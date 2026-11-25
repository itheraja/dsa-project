#include "Algorithms.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <set>
#include <unordered_set>
namespace scope {
Discovery DFSDetector::detect(const Graph &g, const std::string &start, int maxHops, size_t budget,
                              bool tracing) const {
    g.token(start);
    if (maxHops < 2 || maxHops > 6)
        throw std::invalid_argument("Maximum hops must be 2–6");
    Discovery d;
    Route path;
    std::set<std::string> visited{start}, usedPools;
    auto emit = [&](const std::string &action, const std::string &current) {
        if (tracing && d.trace.size() < 5000)
            d.trace.push_back({{"action", action},
                               {"start", start},
                               {"current", current},
                               {"depth", path.size()},
                               {"visited", visited},
                               {"path", path}});
    };
    std::function<void(const std::string &)> visit = [&](const std::string &u) {
        emit("visit", u);
        for (auto idx : g.outgoing(u)) {
            if (d.expansions >= budget) {
                d.truncated = true;
                break;
            }
            ++d.expansions;
            const auto &e = g.edges()[idx];
            if (usedPools.contains(e.pool))
                continue;
            path.push_back(e.id);
            if (e.to == start && path.size() >= 2) {
                d.cycles.push_back(path);
                emit("cycle", start);
            } else if (!visited.contains(e.to) && path.size() < static_cast<size_t>(maxHops)) {
                visited.insert(e.to);
                usedPools.insert(e.pool);
                visit(e.to);
                usedPools.erase(e.pool);
                visited.erase(e.to);
            }
            path.pop_back();
            emit("backtrack", u);
            if (d.truncated)
                break;
        }
    };
    visit(start);
    return d;
}
Discovery BellmanFord::detect(const Graph &g, const std::string &start, int maxHops,
                              bool tracing) const {
    g.token(start);
    Discovery d;
    std::unordered_map<std::string, double> dist;
    std::unordered_map<std::string, std::string> predecessor;
    for (const auto &t : g.tokens())
        dist[t.id] = std::numeric_limits<double>::infinity();
    dist[start] = 0;
    std::vector<double> weights;
    for (const auto &e : g.edges()) {
        // Quote one whole source token; normalized human-unit rate. Approximation only.
        Amount probe = parseAmount("1", g.token(e.from).decimals);
        auto q = ConstantProduct{}.quote(g.pool(e.pool), e.from == g.pool(e.pool).a, probe);
        double rate = q.rate * std::pow(10.0, g.token(e.from).decimals - g.token(e.to).decimals);
        double w = rate > 0 ? -std::log(rate) : std::numeric_limits<double>::infinity();
        weights.push_back(w);
        if (tracing)
            d.trace.push_back({{"action", "weight"},
                               {"edge", e.id},
                               {"weight", std::isfinite(w) ? json(w) : json(nullptr)},
                               {"probe", "1 " + e.from}});
    }
    std::vector<std::string> changed;
    for (size_t iteration = 0; iteration < g.tokens().size(); ++iteration) {
        changed.clear();
        for (size_t i = 0; i < g.edges().size(); ++i) {
            ++d.expansions;
            const auto &e = g.edges()[i];
            double candidate = dist[e.from] + weights[i];
            if (candidate < dist[e.to] - 1e-12) {
                dist[e.to] = candidate;
                predecessor[e.to] = e.id;
                changed.push_back(e.to);
                if (tracing && d.trace.size() < 5000) {
                    json distances = json::object();
                    for (const auto &t : g.tokens())
                        distances[t.id] =
                            std::isfinite(dist[t.id]) ? json(dist[t.id]) : json(nullptr);
                    d.trace.push_back({{"action", "relax"},
                                       {"iteration", iteration + 1},
                                       {"edge", e.id},
                                       {"weight", weights[i]},
                                       {"distances", distances},
                                       {"negativeCycle", iteration + 1 == g.tokens().size()}});
                }
            }
        }
        if (changed.empty())
            break;
    }
    std::set<Route> unique;
    for (auto v : changed) {
        bool valid = true;
        for (size_t i = 0; i < g.tokens().size(); ++i) {
            if (!predecessor.contains(v)) {
                valid = false;
                break;
            }
            v = g.edge(predecessor.at(v)).from;
        }
        if (!valid)
            continue;
        auto root = v;
        Route cycle;
        do {
            if (!predecessor.contains(v)) {
                valid = false;
                break;
            }
            auto id = predecessor.at(v);
            cycle.push_back(id);
            v = g.edge(id).from;
        } while (v != root && cycle.size() <= g.tokens().size());
        if (!valid || v != root || cycle.size() < 2 || cycle.size() > static_cast<size_t>(maxHops))
            continue;
        std::reverse(cycle.begin(), cycle.end());
        auto it = std::find_if(cycle.begin(), cycle.end(),
                               [&](const auto &id) { return g.edge(id).from == start; });
        if (it == cycle.end())
            continue;
        std::rotate(cycle.begin(), it, cycle.end());
        std::set<std::string> pools;
        for (const auto &id : cycle)
            pools.insert(g.edge(id).pool);
        if (pools.size() != cycle.size())
            continue;
        if (unique.insert(cycle).second) {
            d.cycles.push_back(cycle);
            if (tracing)
                d.trace.push_back({{"action", "negative-cycle"}, {"path", cycle}});
        }
    }
    return d;
}
Evaluation SwapSimulator::evaluate(const Graph &original, const Route &route, Amount input,
                                   Amount cost) const {
    if (route.size() < 2 || route.size() > 6 || cost < 0)
        throw std::invalid_argument("Invalid route or cost");
    Graph g = original;
    Evaluation e;
    e.route = route;
    e.input = input;
    e.cost = cost;
    auto start = g.edge(route.front()).from, current = start;
    Amount amount = input;
    std::set<std::string> visited{start}, pools;
    for (size_t i = 0; i < route.size(); ++i) {
        const auto &edge = g.edge(route[i]);
        if (edge.from != current || !pools.insert(edge.pool).second)
            throw std::invalid_argument("Discontinuous route or repeated pool");
        if (i + 1 < route.size() && !visited.insert(edge.to).second)
            throw std::invalid_argument("Repeated vertex");
        const auto p = g.pool(edge.pool);
        auto q = ConstantProduct{}.quote(p, edge.from == p.a, amount);
        if (!q.output)
            throw std::invalid_argument("Swap output rounds to zero");
        auto di = g.token(edge.from).decimals, doo = g.token(edge.to).decimals;
        e.hops.push_back(
            {{"edge", edge.id},
             {"pool", p.id},
             {"dex", p.dex},
             {"from", edge.from},
             {"to", edge.to},
             {"input", formatAmount(amount, di)},
             {"output", formatAmount(q.output, doo)},
             {"fee", formatAmount(q.fee, di)},
             {"feeBps", p.feeBps},
             {"rate", q.rate * std::pow(10.0, di - doo)},
             {"impact", q.impact},
             {"reserveIn", formatAmount(edge.from == p.a ? p.reserveA : p.reserveB, di)},
             {"reserveOut", formatAmount(edge.from == p.a ? p.reserveB : p.reserveA, doo)}});
        ConstantProduct{}.apply(g, edge, amount, q.output);
        amount = q.output;
        current = edge.to;
    }
    if (current != start)
        throw std::invalid_argument("Route must return to starting token");
    e.output = amount;
    e.gross = amount - input;
    e.net = e.gross - cost;
    e.roi = 100.0 * static_cast<double>(e.net) / static_cast<double>(input);
    return e;
}
Optimization TradeOptimizer::optimize(const Graph &g, const Route &route, Amount maxInput,
                                      Amount cost) const {
    if (maxInput < 1000 || maxInput > MAX_AMOUNT)
        throw std::invalid_argument("Capital out of bounds");
    Optimization o;
    o.best.net = std::numeric_limits<Amount>::min();
    std::set<Amount> tested;
    auto sample = [&](Amount amount) {
        if (amount < 1 || !tested.insert(amount).second)
            return;
        try {
            auto e = SwapSimulator{}.evaluate(g, route, amount, cost);
            o.samples.push_back(
                {{"input", formatAmount(amount, g.token(g.edge(route.front()).from).decimals)},
                 {"net", formatAmount(e.net, g.token(g.edge(route.front()).from).decimals)}});
            if (e.net > o.best.net || (e.net == o.best.net && e.input < o.best.input))
                o.best = e;
        } catch (const std::invalid_argument &) { /* Dust or reserve bound: infeasible sample. */
        }
    };
    // Coarse grid + bounded local refinement. Not a global integer-optimality proof.
    for (int i = 1; i <= 48; ++i)
        sample(maxInput * i / 48);
    sample(std::min<Amount>(maxInput, 1000));
    Amount span = maxInput / 48;
    for (int round = 0; round < 7 && o.best.input > 0; ++round) {
        Amount center = o.best.input, step = std::max<Amount>(1, span / 8);
        for (int k = -8; k <= 8; ++k)
            sample(std::clamp(center + k * step, Amount(1), maxInput));
        span = step;
    }
    std::sort(o.samples.begin(), o.samples.end(), [](const json &a, const json &b) {
        return std::stod(a.at("input").get<std::string>()) <
               std::stod(b.at("input").get<std::string>());
    });
    return o;
}
json benchmarks() {
    json rows = json::array();
    for (int n : {6, 12, 24, 48}) {
        Graph g;
        for (int i = 0; i < n; ++i)
            g.addToken({"T" + std::to_string(i), "T" + std::to_string(i), 6, "", "Synthetic"});
        for (int i = 0; i < n; ++i)
            for (int k = 1; k <= 3; ++k)
                g.addPool({"p" + std::to_string(i) + "-" + std::to_string(k),
                           "T" + std::to_string(i), "T" + std::to_string((i + k) % n),
                           "Benchmark CPMM", 10000000000, 10000000000, 30});
        for (const std::string method : {"dfs", "bellman-ford"}) {
            auto begin = std::chrono::steady_clock::now();
            Discovery d;
            for (int r = 0; r < 5; ++r)
                d = method == "dfs" ? DFSDetector{}.detect(g, "T0", 4, 20000, false)
                                    : BellmanFord{}.detect(g, "T0", 4, false);
            double ms =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin)
                    .count() /
                5;
            rows.push_back({{"tokens", n},
                            {"pools", g.pools().size()},
                            {"edges", g.edges().size()},
                            {"algorithm", method},
                            {"milliseconds", ms},
                            {"cycles", d.cycles.size()},
                            {"expansions", d.expansions},
                            {"truncated", d.truncated},
                            {"maxHops", 4},
                            {"repetitions", 5}});
        }
    }
    return rows;
}
} // namespace scope
