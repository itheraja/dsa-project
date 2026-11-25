#pragma once
#include "Algorithms.hpp"
namespace scope {
// Live adapters must provide validated snapshots. No wallet or submission interface exists.
class MarketDataConnector {
  public:
    virtual ~MarketDataConnector() = default;
    virtual bool connected() const = 0;
    virtual Graph snapshot() const = 0;
};
class DisabledSolanaConnector final : public MarketDataConnector {
  public:
    bool connected() const override {
        return false;
    }
    Graph snapshot() const override {
        throw std::runtime_error("Live Solana data is not connected");
    }
};
class ArbitrageEngine {
    std::string dataDir_;
    uint64_t revision_ = 0, run_ = 0, tradeSequence_ = 0;
    std::unordered_map<std::string, Evaluation> evaluations_;
    Amount pnl_ = 0;

  public:
    Graph graph;
    std::string scenario = "profitable", mode = "demo", algorithm = "dfs", ranking = "profit";
    Amount capital = 1'000'000'000;
    int maxHops = 4, replayIndex = 0;
    RiskManager risk;
    json opportunities = json::array(), candidates = json::array(), trades = json::array(),
         trace = json::array();
    json metrics;
    explicit ArbitrageEngine(std::string dataDir = SCOPE_DATA_DIR);
    json scenarios() const;
    void select(const std::string &name, bool clearLedger = true);
    void replay(int index);
    void configureAndDetect(const json &body);
    void detect();
    void updatePool(const json &body);
    json execute(const std::string &id);
    json state() const;
    json detail(const std::string &id) const;
};
} // namespace scope
