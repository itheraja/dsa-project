#include "Engine.hpp"
#include <gtest/gtest.h>
#include <set>
using namespace scope;
TEST(Amounts, ExactParsingAndFormatting) {
    EXPECT_EQ(parseAmount("1.000001", 6), 1000001);
    EXPECT_EQ(parseAmount("2", 9), 2000000000);
    EXPECT_EQ(formatAmount(-123, 6), "-0.000123");
    EXPECT_THROW(parseAmount("-1", 6), std::invalid_argument);
    EXPECT_THROW(parseAmount("1.0000001", 6), std::invalid_argument);
    EXPECT_THROW(parseAmount("1e9", 6), std::invalid_argument);
    EXPECT_THROW(parseAmount("1000000000000", 6), std::invalid_argument);
}
TEST(Graph, ParallelPoolsAndAtomicMutation) {
    ArbitrageEngine e;
    EXPECT_EQ(e.graph.tokens().size(), 6);
    EXPECT_EQ(e.graph.edges().size(), 16);
    int count = 0;
    for (auto i : e.graph.outgoing("USDC"))
        if (e.graph.edges()[i].to == "SOL")
            ++count;
    EXPECT_EQ(count, 2);
    auto old = e.graph.pool("alpha-usdc-sol");
    EXPECT_THROW(e.graph.updatePool(old.id, -1, 1, 30), std::invalid_argument);
    EXPECT_EQ(e.graph.pool(old.id).reserveA, old.reserveA);
    e.graph.updatePool(old.id, old.reserveA + 1, old.reserveB, 40);
    EXPECT_EQ(e.graph.pool(old.id).feeBps, 40);
    EXPECT_THROW(e.graph.pool("missing"), std::invalid_argument);
    EXPECT_THROW(e.graph.addPool(old), std::invalid_argument);
}
TEST(Pricing, HandCalculatedQuote) {
    Pool p{"p", "a", "b", "test", 1000000, 2000000, 30};
    auto q = ConstantProduct{}.quote(p, true, 10000);
    EXPECT_EQ(q.fee, 30);
    EXPECT_EQ(q.output, 19743);
    EXPECT_GT(q.impact, 0);
    EXPECT_EQ(ConstantProduct{}.quote(p, true, 1).output, 0);
    EXPECT_EQ(ConstantProduct{}.quote(p, true, 101).fee, 1);
}
TEST(Pricing, WideIntermediates) {
    Pool p{"p", "a", "b", "test", MAX_AMOUNT, MAX_AMOUNT, 0};
    EXPECT_EQ(ConstantProduct{}.quote(p, true, MAX_AMOUNT).output, MAX_AMOUNT / 2);
    EXPECT_THROW(ConstantProduct{}.quote(p, true, MAX_AMOUNT + 1), std::invalid_argument);
}
TEST(Pricing, ReservesAndInvariant) {
    Graph g;
    g.addToken({"a", "a", 6, "", ""});
    g.addToken({"b", "b", 6, "", ""});
    g.addPool({"p", "a", "b", "test", 1000000, 2000000, 30});
    auto q = ConstantProduct{}.quote(g.pool("p"), true, 10000);
    ConstantProduct{}.apply(g, g.edge("p:ab"), 10000, q.output);
    EXPECT_EQ(g.pool("p").reserveA, 1010000);
    EXPECT_EQ(g.pool("p").reserveB, 2000000 - q.output);
    EXPECT_GE(__int128_t(g.pool("p").reserveA) * g.pool("p").reserveB,
              __int128_t(1000000) * 2000000);
}
TEST(DFS, DeterministicUniqueValidCycles) {
    ArbitrageEngine e;
    auto a = DFSDetector{}.detect(e.graph, "USDC", 4), b = DFSDetector{}.detect(e.graph, "USDC", 4);
    EXPECT_EQ(a.cycles, b.cycles);
    EXPECT_EQ(a.trace, b.trace);
    ASSERT_FALSE(a.cycles.empty());
    std::set<Route> unique;
    bool two = false, three = false;
    for (auto &r : a.cycles) {
        EXPECT_TRUE(unique.insert(r).second);
        std::set<std::string> vertices, pools;
        auto current = std::string("USDC");
        for (auto &id : r) {
            auto edge = e.graph.edge(id);
            EXPECT_EQ(edge.from, current);
            EXPECT_TRUE(vertices.insert(edge.from).second);
            EXPECT_TRUE(pools.insert(edge.pool).second);
            current = edge.to;
        }
        EXPECT_EQ(current, "USDC");
        two |= r.size() == 2;
        three |= r.size() == 3;
    }
    EXPECT_TRUE(two);
    EXPECT_TRUE(three);
}
TEST(DFS, TraversalBudget) {
    ArbitrageEngine e;
    auto d = DFSDetector{}.detect(e.graph, "USDC", 6, 3);
    EXPECT_TRUE(d.truncated);
    EXPECT_EQ(d.expansions, 3);
}
TEST(BellmanFord, NegativeAndBalancedMarkets) {
    ArbitrageEngine e;
    auto d = BellmanFord{}.detect(e.graph, "USDC", 4);
    ASSERT_FALSE(d.cycles.empty());
    bool profitable = false;
    for (auto &r : d.cycles)
        profitable |=
            TradeOptimizer{}.optimize(e.graph, r, e.capital, e.risk.cost(r.size())).best.net > 0;
    EXPECT_TRUE(profitable);
    e.select("balanced");
    EXPECT_TRUE(BellmanFord{}.detect(e.graph, "USDC", 4).cycles.empty());
}
TEST(Engine, ProfitableFixtureAndAccounting) {
    ArbitrageEngine e;
    ASSERT_FALSE(e.opportunities.empty());
    auto o = e.opportunities.at(0);
    auto input = parseAmount(o.at("input"), 6), output = parseAmount(o.at("output"), 6),
         gross = parseAmount(o.at("gross"), 6), net = parseAmount(o.at("net"), 6),
         cost = parseAmount(o.at("cost"), 6);
    EXPECT_EQ(output - input, gross);
    EXPECT_EQ(gross - cost, net);
    EXPECT_GT(net, 0);
    EXPECT_NEAR(o.at("roi").get<double>(), 100.0 * double(net) / double(input), 1e-9);
    auto &hops = o.at("hops");
    for (size_t i = 1; i < hops.size(); ++i)
        EXPECT_EQ(hops[i - 1].at("output"), hops[i].at("input"));
}
TEST(Engine, BalancedAndFeeShock) {
    ArbitrageEngine e;
    e.select("balanced");
    EXPECT_TRUE(e.opportunities.empty());
    e.select("fee-shock");
    EXPECT_TRUE(e.opportunities.empty());
}
TEST(Optimizer, ImpactAndBoundedOptimum) {
    ArbitrageEngine e;
    auto r = DFSDetector{}.detect(e.graph, "USDC", 3).cycles;
    Route selected;
    for (auto &p : r) {
        auto o = TradeOptimizer{}.optimize(e.graph, p, 100000000000, e.risk.cost(p.size()));
        if (o.best.net > 0) {
            selected = p;
            EXPECT_LT(o.best.input, 100000000000);
            auto large = SwapSimulator{}.evaluate(e.graph, p, 100000000000, e.risk.cost(p.size()));
            EXPECT_LT(large.net, 0);
            EXPECT_GT(o.best.net, large.net);
            break;
        }
    }
    EXPECT_FALSE(selected.empty());
}
TEST(Engine, HeapRanking) {
    ArbitrageEngine e;
    e.select("competing");
    ASSERT_GT(e.opportunities.size(), 1);
    for (size_t i = 1; i < e.opportunities.size(); ++i)
        EXPECT_GE(parseAmount(e.opportunities[i - 1].at("net"), 6),
                  parseAmount(e.opportunities[i].at("net"), 6));
    e.configureAndDetect({{"ranking", "roi"}});
    for (size_t i = 1; i < e.opportunities.size(); ++i)
        EXPECT_GE(e.opportunities[i - 1].at("roi").get<double>(),
                  e.opportunities[i].at("roi").get<double>());
}
TEST(Execution, LedgerReserveUpdatesAndStaleRejection) {
    ArbitrageEngine e;
    auto o = e.opportunities.at(0);
    auto before = e.graph.toJson();
    auto trade = e.execute(o.at("id"));
    EXPECT_EQ(trade.at("net"), o.at("net"));
    EXPECT_EQ(e.trades.size(), 1);
    EXPECT_EQ(e.metrics.at("pnl"), trade.at("net"));
    EXPECT_NE(before, e.graph.toJson());
    auto after = e.graph.toJson();
    EXPECT_THROW(e.execute(o.at("id")), std::invalid_argument);
    EXPECT_EQ(after, e.graph.toJson());
}
TEST(Engine, ResetReproducibility) {
    ArbitrageEngine e;
    auto old = e.opportunities.at(0);
    e.execute(old.at("id"));
    e.select("profitable");
    EXPECT_TRUE(e.trades.empty());
    EXPECT_EQ(e.metrics.at("pnl"), "0.000000");
    EXPECT_EQ(old.at("input"), e.opportunities.at(0).at("input"));
    EXPECT_EQ(old.at("net"), e.opportunities.at(0).at("net"));
    EXPECT_EQ(old.at("edges"), e.opportunities.at(0).at("edges"));
}
TEST(Engine, InvalidConfigurationIsAtomic) {
    ArbitrageEngine e;
    auto before = e.state();
    EXPECT_THROW(e.configureAndDetect({{"capital", "500"}, {"maxHops", 99}}),
                 std::invalid_argument);
    EXPECT_EQ(before, e.state());
    EXPECT_THROW(e.select("../../bad"), std::invalid_argument);
    EXPECT_EQ(before, e.state());
    EXPECT_THROW(
        e.updatePool({{"id", "missing"}, {"reserveA", "1"}, {"reserveB", "1"}, {"feeBps", 30}}),
        std::invalid_argument);
}
TEST(Replay, CreationAndDisappearance) {
    ArbitrageEngine e;
    e.replay(0);
    EXPECT_TRUE(e.opportunities.empty());
    e.replay(1);
    EXPECT_FALSE(e.opportunities.empty());
    EXPECT_THROW(e.execute(e.opportunities.at(0).at("id")), std::invalid_argument);
    e.replay(3);
    EXPECT_TRUE(e.opportunities.empty());
    EXPECT_THROW(e.replay(4), std::invalid_argument);
}
TEST(Execution, RejectsLossAndMalformedRoute) {
    ArbitrageEngine e;
    e.select("balanced");
    ASSERT_FALSE(e.candidates.empty());
    EXPECT_THROW(e.execute(e.candidates.at(0).at("id")), std::invalid_argument);
    EXPECT_TRUE(e.trades.empty());
    EXPECT_THROW(
        SwapSimulator{}.evaluate(e.graph, {"alpha-usdc-sol:ab", "gamma-jup-usdc:ab"}, 1000000, 0),
        std::invalid_argument);
}
TEST(Live, ExplicitlyDisabled) {
    DisabledSolanaConnector c;
    EXPECT_FALSE(c.connected());
    EXPECT_THROW(c.snapshot(), std::runtime_error);
}
