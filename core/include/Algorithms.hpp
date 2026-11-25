#pragma once
#include "Graph.hpp"
namespace scope {
using Route = std::vector<std::string>;
struct Discovery {
    std::vector<Route> cycles;
    json trace = json::array();
    size_t expansions = 0;
    bool truncated = false;
};
class DFSDetector {
  public:
    Discovery detect(const Graph &, const std::string &start, int maxHops, size_t budget = 20000,
                     bool trace = true) const;
};
class BellmanFord {
  public:
    Discovery detect(const Graph &, const std::string &start, int maxHops, bool trace = true) const;
};
struct Evaluation {
    Route route;
    Amount input = 0, output = 0, gross = 0, cost = 0, net = 0;
    double roi = 0;
    json hops = json::array();
};
class SwapSimulator {
  public:
    Evaluation evaluate(const Graph &, const Route &, Amount input, Amount cost) const;
};
struct Optimization {
    Evaluation best;
    json samples = json::array();
};
class TradeOptimizer {
  public:
    Optimization optimize(const Graph &, const Route &, Amount maxInput, Amount cost) const;
};
struct RiskManager {
    Amount minProfit = 10000;                     // 0.01 USDC
    Amount fixedCost = 50000, perHopCost = 10000; // explicit synthetic cost model
    Amount cost(size_t hops) const {
        return fixedCost + perHopCost * static_cast<Amount>(hops);
    }
    bool accepts(const Evaluation &e) const {
        return e.net >= minProfit;
    }
};
json benchmarks();
} // namespace scope
