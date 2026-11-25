#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>
namespace scope {
using json = nlohmann::json;
using Amount = int64_t;
constexpr Amount MAX_AMOUNT = 1'000'000'000'000'000;
Amount parseAmount(const std::string &value, int decimals);
std::string formatAmount(Amount value, int decimals);
struct Token {
    std::string id, symbol;
    int decimals;
    std::string mint, name;
};
struct Pool {
    std::string id, a, b, dex;
    Amount reserveA, reserveB;
    int feeBps;
};
struct Edge {
    std::string id, pool, from, to;
};
class Graph {
    std::unordered_map<std::string, size_t> tokenIndex_, poolIndex_, edgeIndex_;
    std::unordered_map<std::string, std::vector<size_t>> adjacency_;
    std::vector<Token> tokens_;
    std::vector<Pool> pools_;
    std::vector<Edge> edges_;

  public:
    const std::vector<Token> &tokens() const {
        return tokens_;
    }
    const std::vector<Pool> &pools() const {
        return pools_;
    }
    const std::vector<Edge> &edges() const {
        return edges_;
    }
    std::string snapshot;
    static Graph fromJson(const json &j);
    void addToken(Token token);
    void addPool(Pool pool);
    void updatePool(const std::string &id, Amount a, Amount b, int fee);
    const Token &token(const std::string &id) const;
    const Pool &pool(const std::string &id) const;
    const Edge &edge(const std::string &id) const;
    const std::vector<size_t> &outgoing(const std::string &token) const;
    json toJson() const;
};
struct Quote {
    Amount output, fee;
    double rate, impact;
};
class PoolModel {
  public:
    virtual ~PoolModel() = default;
    virtual Quote quote(const Pool &, bool forward, Amount input) const = 0;
};
class ConstantProduct final : public PoolModel {
  public:
    Quote quote(const Pool &, bool forward, Amount input) const override;
    void apply(Graph &, const Edge &, Amount input, Amount output) const;
};
} // namespace scope
