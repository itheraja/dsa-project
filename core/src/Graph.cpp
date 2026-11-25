#include "Graph.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace scope {
Amount parseAmount(const std::string &s, int decimals) {
    if (s.empty() || decimals < 0 || decimals > 9)
        throw std::invalid_argument("Invalid token amount");
    Amount result = 0;
    int fraction = -1;
    for (char c : s) {
        if (c == '.' && fraction == -1) {
            fraction = 0;
            continue;
        }
        if (c < '0' || c > '9' || (fraction >= decimals && fraction >= 0))
            throw std::invalid_argument("Invalid amount or excess decimal precision");
        if (result > (MAX_AMOUNT - (c - '0')) / 10)
            throw std::invalid_argument("Amount exceeds safe bound");
        result = result * 10 + (c - '0');
        if (fraction >= 0)
            ++fraction;
    }
    if (s == "." || s.back() == '.')
        throw std::invalid_argument("Invalid amount");
    int padding = decimals - (fraction < 0 ? 0 : fraction);
    for (int i = 0; i < padding; ++i) {
        if (result > MAX_AMOUNT / 10)
            throw std::invalid_argument("Amount exceeds safe bound");
        result *= 10;
    }
    return result;
}
std::string formatAmount(Amount value, int decimals) {
    bool neg = value < 0;
    uint64_t n = static_cast<uint64_t>(neg ? -value : value);
    auto s = std::to_string(n);
    if (decimals) {
        if (s.size() <= static_cast<size_t>(decimals))
            s.insert(0, static_cast<size_t>(decimals) + 1 - s.size(), '0');
        s.insert(s.size() - static_cast<size_t>(decimals), ".");
    }
    return (neg ? "-" : "") + s;
}
void Graph::addToken(Token t) {
    if (t.id.empty() || tokenIndex_.contains(t.id) || t.decimals < 0 || t.decimals > 9)
        throw std::invalid_argument("Invalid or duplicate token");
    tokenIndex_[t.id] = tokens_.size();
    adjacency_[t.id] = {};
    tokens_.push_back(std::move(t));
}
void Graph::addPool(Pool p) {
    token(p.a);
    token(p.b);
    if (p.id.empty() || poolIndex_.contains(p.id) || p.a == p.b || p.reserveA <= 0 ||
        p.reserveB <= 0 || p.reserveA > MAX_AMOUNT || p.reserveB > MAX_AMOUNT || p.feeBps < 0 ||
        p.feeBps >= 10000)
        throw std::invalid_argument("Invalid or duplicate pool");
    poolIndex_[p.id] = pools_.size();
    pools_.push_back(p);
    for (bool forward : {true, false}) {
        Edge e{p.id + (forward ? ":ab" : ":ba"), p.id, forward ? p.a : p.b, forward ? p.b : p.a};
        edgeIndex_[e.id] = edges_.size();
        adjacency_[e.from].push_back(edges_.size());
        edges_.push_back(e);
    }
}
void Graph::updatePool(const std::string &id, Amount a, Amount b, int fee) {
    const auto &old = pool(id);
    (void)old;
    if (a <= 0 || b <= 0 || a > MAX_AMOUNT || b > MAX_AMOUNT || fee < 0 || fee >= 10000)
        throw std::invalid_argument("Invalid reserves or fee (0–9999 bps)");
    auto &p = pools_.at(poolIndex_.at(id));
    p.reserveA = a;
    p.reserveB = b;
    p.feeBps = fee;
}
const Token &Graph::token(const std::string &id) const {
    auto i = tokenIndex_.find(id);
    if (i == tokenIndex_.end())
        throw std::invalid_argument("Unknown token: " + id);
    return tokens_.at(i->second);
}
const Pool &Graph::pool(const std::string &id) const {
    auto i = poolIndex_.find(id);
    if (i == poolIndex_.end())
        throw std::invalid_argument("Unknown pool: " + id);
    return pools_.at(i->second);
}
const Edge &Graph::edge(const std::string &id) const {
    auto i = edgeIndex_.find(id);
    if (i == edgeIndex_.end())
        throw std::invalid_argument("Unknown edge: " + id);
    return edges_.at(i->second);
}
const std::vector<size_t> &Graph::outgoing(const std::string &id) const {
    token(id);
    return adjacency_.at(id);
}
Graph Graph::fromJson(const json &j) {
    Graph g;
    g.snapshot = j.at("id");
    for (const auto &t : j.at("tokens"))
        g.addToken({t.at("id"), t.at("symbol"), t.at("decimals"), t.value("mint", ""),
                    t.value("name", t.at("symbol").get<std::string>())});
    for (const auto &p : j.at("pools")) {
        if (p.value("type", "CPMM") != "CPMM")
            throw std::invalid_argument("Unsupported pool model");
        auto a = p.at("a").get<std::string>(), b = p.at("b").get<std::string>();
        g.addPool({p.at("id"), a, b, p.at("dex"),
                   parseAmount(p.at("reserveA"), g.token(a).decimals),
                   parseAmount(p.at("reserveB"), g.token(b).decimals), p.at("feeBps")});
    }
    return g;
}
json Graph::toJson() const {
    json ts = json::array(), ps = json::array(), es = json::array();
    for (const auto &t : tokens_)
        ts.push_back({{"id", t.id},
                      {"symbol", t.symbol},
                      {"decimals", t.decimals},
                      {"mint", t.mint},
                      {"name", t.name}});
    for (const auto &p : pools_)
        ps.push_back({{"id", p.id},
                      {"a", p.a},
                      {"b", p.b},
                      {"dex", p.dex},
                      {"type", "Synthetic CPMM"},
                      {"reserveA", formatAmount(p.reserveA, token(p.a).decimals)},
                      {"reserveB", formatAmount(p.reserveB, token(p.b).decimals)},
                      {"feeBps", p.feeBps},
                      {"snapshot", snapshot}});
    for (const auto &e : edges_)
        es.push_back({{"id", e.id}, {"pool", e.pool}, {"from", e.from}, {"to", e.to}});
    return {
        {"tokens", ts}, {"pools", ps}, {"edges", es}, {"snapshot", snapshot}, {"synthetic", true}};
}
Quote ConstantProduct::quote(const Pool &p, bool forward, Amount input) const {
    if (input <= 0 || input > MAX_AMOUNT)
        throw std::invalid_argument("Input must be positive and bounded");
    Amount x = forward ? p.reserveA : p.reserveB, y = forward ? p.reserveB : p.reserveA;
    if (x <= 0 || y <= 0 || x > MAX_AMOUNT || y > MAX_AMOUNT || p.feeBps < 0 || p.feeBps >= 10000)
        throw std::invalid_argument("Invalid CPMM state");
    // Fee is rounded UP to atomic units. All fee remains in pool reserves.
    using Wide = __int128_t;
    Amount fee = static_cast<Amount>((Wide(input) * p.feeBps + 9999) / 10000);
    Amount effective = input - fee;
    Amount output = static_cast<Amount>(Wide(y) * effective / (Wide(x) + effective));
    double rate = static_cast<double>(output) / static_cast<double>(input);
    double spot = static_cast<double>(y) / static_cast<double>(x);
    double impact =
        effective
            ? 100.0 * (1.0 - static_cast<double>(output) / (static_cast<double>(effective) * spot))
            : 100.0;
    return {output, fee, rate, std::max(0.0, impact)};
}
void ConstantProduct::apply(Graph &g, const Edge &e, Amount input, Amount output) const {
    auto p = g.pool(e.pool);
    bool f = e.from == p.a;
    Amount x = f ? p.reserveA : p.reserveB, y = f ? p.reserveB : p.reserveA;
    if (input > MAX_AMOUNT - x || output <= 0 || output >= y)
        throw std::invalid_argument("Swap violates reserve bounds or rounds to zero");
    if (quote(p, f, input).output != output)
        throw std::invalid_argument("Quote mismatch");
    g.updatePool(p.id, f ? x + input : y - output, f ? y - output : x + input, p.feeBps);
}
} // namespace scope
