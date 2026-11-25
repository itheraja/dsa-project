#include "Engine.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <queue>
namespace scope {
namespace {
json read(const std::string &path) {
    std::ifstream f(path);
    if (!f)
        throw std::runtime_error("Cannot load fixture: " + path);
    json j;
    f >> j;
    return j;
}
json encode(const Evaluation &e, const std::string &id, const Graph &g) {
    const auto &t = g.token(g.edge(e.route.front()).from);
    std::vector<std::string> tokens{t.id}, pools;
    for (const auto &id : e.route) {
        auto edge = g.edge(id);
        tokens.push_back(edge.to);
        pools.push_back(edge.pool);
    }
    return {{"id", id},
            {"route", tokens},
            {"edges", e.route},
            {"pools", pools},
            {"asset", t.id},
            {"input", formatAmount(e.input, t.decimals)},
            {"output", formatAmount(e.output, t.decimals)},
            {"gross", formatAmount(e.gross, t.decimals)},
            {"cost", formatAmount(e.cost, t.decimals)},
            {"net", formatAmount(e.net, t.decimals)},
            {"roi", e.roi},
            {"hops", e.hops},
            {"snapshot", g.snapshot},
            {"status", e.net >= 10000 ? "profitable" : "rejected"},
            {"simulation", true}};
}
} // namespace
ArbitrageEngine::ArbitrageEngine(std::string dir) : dataDir_(std::move(dir)) {
    select("profitable");
}
json ArbitrageEngine::scenarios() const {
    return read(dataDir_ + "/scenarios/index.json");
}
void ArbitrageEngine::select(const std::string &name, bool clearLedger) {
    bool found = false;
    for (const auto &s : scenarios())
        if (s.at("id") == name)
            found = true;
    if (!found)
        throw std::invalid_argument("Unknown scenario: " + name);
    auto fresh = Graph::fromJson(read(dataDir_ + "/scenarios/" + name + ".json"));
    graph = std::move(fresh);
    scenario = name;
    mode = "demo";
    ++revision_;
    if (clearLedger) {
        trades = json::array();
        pnl_ = 0;
    }
    detect();
}
void ArbitrageEngine::replay(int index) {
    auto frames = read(dataDir_ + "/snapshots/replay.json").at("snapshots");
    if (index < 0 || index >= static_cast<int>(frames.size()))
        throw std::invalid_argument("Snapshot index must be 0–3");
    auto fresh = Graph::fromJson(frames.at(static_cast<size_t>(index)));
    graph = std::move(fresh);
    replayIndex = index;
    mode = "replay";
    scenario = "synthetic-replay";
    ++revision_;
    detect();
}
void ArbitrageEngine::configureAndDetect(const json &body) {
    for (const auto *key : {"maxHops"})
        if (body.contains(key) && !body.at(key).is_number_integer())
            throw std::invalid_argument("maxHops must be an integer");
    auto c =
        body.contains("capital") ? parseAmount(body.at("capital").get<std::string>(), 6) : capital;
    auto h = body.value("maxHops", maxHops);
    auto a = body.value("algorithm", algorithm);
    auto r = body.value("ranking", ranking);
    if (c < 1000 || c > 100000000000LL)
        throw std::invalid_argument("Capital must be 0.001–100000 USDC");
    if (h < 2 || h > 6)
        throw std::invalid_argument("Maximum hops must be 2–6");
    if (a != "dfs" && a != "bellman-ford")
        throw std::invalid_argument("Unknown algorithm");
    if (r != "profit" && r != "roi" && r != "cost" && r != "hops")
        throw std::invalid_argument("Unknown ranking");
    capital = c;
    maxHops = h;
    algorithm = a;
    ranking = r;
    detect();
}
void ArbitrageEngine::detect() {
    auto begin = std::chrono::steady_clock::now();
    ++run_;
    auto discovered = algorithm == "dfs" ? DFSDetector{}.detect(graph, "USDC", maxHops)
                                         : BellmanFord{}.detect(graph, "USDC", maxHops);
    trace = discovered.trace;
    opportunities = json::array();
    candidates = json::array();
    evaluations_.clear();
    struct Item {
        Evaluation e;
        json payload;
        size_t index;
    };
    auto worse = [&](const Item &a, const Item &b) {
        if (ranking == "roi" && a.e.roi != b.e.roi)
            return a.e.roi < b.e.roi;
        if (ranking == "cost" && a.e.cost != b.e.cost)
            return a.e.cost > b.e.cost;
        if (ranking == "hops" && a.e.route.size() != b.e.route.size())
            return a.e.route.size() > b.e.route.size();
        if (a.e.net != b.e.net)
            return a.e.net < b.e.net;
        return a.index > b.index;
    };
    std::priority_queue<Item, std::vector<Item>, decltype(worse)> heap(worse);
    size_t index = 0;
    for (const auto &route : discovered.cycles) {
        auto optimized = TradeOptimizer{}.optimize(graph, route, capital, risk.cost(route.size()));
        if (!optimized.best.input)
            continue;
        auto id = "r" + std::to_string(revision_) + "-d" + std::to_string(run_) + "-" +
                  std::to_string(++index);
        auto payload = encode(optimized.best, id, graph);
        payload["profitCurve"] = optimized.samples;
        payload["revision"] = revision_;
        try {
            payload["atCapital"] =
                encode(SwapSimulator{}.evaluate(graph, route, capital, risk.cost(route.size())),
                       "capital", graph);
        } catch (const std::invalid_argument &ex) {
            payload["capitalError"] = ex.what();
        }
        candidates.push_back(payload);
        evaluations_[id] = optimized.best;
        if (risk.accepts(optimized.best))
            heap.push({optimized.best, payload, index});
    }
    while (!heap.empty()) {
        opportunities.push_back(heap.top().payload);
        heap.pop();
    }
    double ms =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    for (auto &o : opportunities)
        o["detectionMs"] = ms;
    metrics = {{"tokens", graph.tokens().size()},
               {"pools", graph.pools().size()},
               {"candidates", discovered.cycles.size()},
               {"profitable", opportunities.size()},
               {"milliseconds", ms},
               {"expansions", discovered.expansions},
               {"truncated", discovered.truncated},
               {"traceTruncated", trace.size() >= 5000},
               {"bestNet", "0.000000"},
               {"bestRoi", 0.0},
               {"pnl", formatAmount(pnl_, 6)}};
    for (const auto &o : opportunities) {
        if (parseAmount(o.at("net"), 6) > parseAmount(metrics.at("bestNet"), 6))
            metrics["bestNet"] = o.at("net");
        metrics["bestRoi"] =
            std::max(metrics.at("bestRoi").get<double>(), o.at("roi").get<double>());
    }
}
void ArbitrageEngine::updatePool(const json &body) {
    if (mode != "demo")
        throw std::invalid_argument("Pool edits are available in Demo Simulation only");
    if (!body.at("feeBps").is_number_integer())
        throw std::invalid_argument("feeBps must be an integer");
    auto id = body.at("id").get<std::string>();
    const auto p = graph.pool(id);
    auto a = parseAmount(body.at("reserveA"), graph.token(p.a).decimals),
         b = parseAmount(body.at("reserveB"), graph.token(p.b).decimals);
    int fee = body.at("feeBps");
    graph.updatePool(id, a, b, fee);
    ++revision_;
    graph.snapshot = scenario + "-edit-" + std::to_string(revision_);
    detect();
}
json ArbitrageEngine::execute(const std::string &id) {
    if (mode != "demo")
        throw std::invalid_argument("Simulated execution is available in Demo Simulation only");
    auto it = evaluations_.find(id);
    if (it == evaluations_.end())
        throw std::invalid_argument("Unknown or stale opportunity; detect again");
    auto e = SwapSimulator{}.evaluate(graph, it->second.route, it->second.input,
                                      risk.cost(it->second.route.size()));
    if (!risk.accepts(e))
        throw std::invalid_argument("Trade is unprofitable or below the 0.01 USDC threshold");
    Graph updated = graph;
    Amount amount = e.input;
    for (const auto &edgeId : e.route) {
        auto edge = updated.edge(edgeId);
        auto p = updated.pool(edge.pool);
        auto q = ConstantProduct{}.quote(p, edge.from == p.a, amount);
        ConstantProduct{}.apply(updated, edge, amount, q.output);
        amount = q.output;
    }
    if (e.net > MAX_AMOUNT - pnl_)
        throw std::invalid_argument("Ledger limit reached; reset demo");
    auto record = encode(e, "SIM-" + std::to_string(++tradeSequence_), graph);
    record["scenario"] = scenario;
    record["status"] = "SIMULATED — NO REAL FUNDS TRADED";
    record["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count();
    pnl_ += e.net;
    record["cumulativePnl"] = formatAmount(pnl_, 6);
    trades.push_back(record);
    graph = std::move(updated);
    ++revision_;
    graph.snapshot = scenario + "-trade-" + std::to_string(revision_);
    detect();
    return record;
}
json ArbitrageEngine::detail(const std::string &id) const {
    for (const auto &c : candidates)
        if (c.at("id") == id)
            return c;
    throw std::invalid_argument("Unknown opportunity");
}
json ArbitrageEngine::state() const {
    return {{"graph", graph.toJson()},
            {"scenario", scenario},
            {"mode", mode},
            {"algorithm", algorithm},
            {"ranking", ranking},
            {"capital", formatAmount(capital, 6)},
            {"maxHops", maxHops},
            {"opportunities", opportunities},
            {"candidates", candidates},
            {"trades", trades},
            {"metrics", metrics},
            {"revision", revision_},
            {"replayIndex", replayIndex},
            {"live", {{"connected", false}, {"status", "Not Connected"}}},
            {"label", "SIMULATED — NO REAL FUNDS TRADED"}};
}
} // namespace scope
