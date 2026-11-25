#include "Engine.hpp"
#include <cstdlib>
#include <httplib.h>
#include <iostream>
#include <mutex>
int main(int argc, char **argv) {
    try {
        scope::ArbitrageEngine engine;
        std::mutex mutex;
        httplib::Server server;
        server.set_payload_max_length(16384);
        auto endpoint = [&](auto fn) {
            return [&, fn](const httplib::Request &req, httplib::Response &res) {
                std::lock_guard lock(mutex);
                try {
                    scope::json body =
                        req.body.empty() ? scope::json::object() : scope::json::parse(req.body);
                    auto result = fn(req, body);
                    res.set_content(scope::json{{"ok", true}, {"data", result}}.dump(),
                                    "application/json");
                } catch (const scope::json::exception &e) {
                    res.status = 400;
                    res.set_content(
                        scope::json{{"ok", false},
                                    {"error", {{"code", "INVALID_JSON"}, {"message", e.what()}}}}
                            .dump(),
                        "application/json");
                } catch (const std::invalid_argument &e) {
                    res.status = 400;
                    res.set_content(
                        scope::json{{"ok", false},
                                    {"error", {{"code", "INVALID_REQUEST"}, {"message", e.what()}}}}
                            .dump(),
                        "application/json");
                } catch (const std::exception &e) {
                    res.status = 500;
                    std::cerr << e.what() << '\n';
                    res.set_content(
                        R"({"ok":false,"error":{"code":"INTERNAL_ERROR","message":"Internal server error"}})",
                        "application/json");
                }
            };
        };
        server.Get("/api/health", endpoint([&](auto &, auto &) {
                       return scope::json{
                           {"status", "ok"}, {"engine", "C++20"}, {"liveConnected", false}};
                   }));
        server.Get("/api/state", endpoint([&](auto &, auto &) { return engine.state(); }));
        server.Get("/api/scenarios", endpoint([&](auto &, auto &) { return engine.scenarios(); }));
        server.Post("/api/scenario/select", endpoint([&](auto &, auto &b) {
                        engine.select(b.at("scenario"));
                        return engine.state();
                    }));
        server.Post("/api/reset", endpoint([&](auto &, auto &) {
                        engine.capital = 1000000000;
                        engine.algorithm = "dfs";
                        engine.maxHops = 4;
                        engine.ranking = "profit";
                        engine.select("profitable");
                        return engine.state();
                    }));
        server.Get("/api/graph", endpoint([&](auto &, auto &) { return engine.graph.toJson(); }));
        server.Get("/api/opportunities",
                   endpoint([&](auto &, auto &) { return engine.opportunities; }));
        server.Get(R"(/api/opportunities/([^/]+))",
                   endpoint([&](auto &r, auto &) { return engine.detail(r.matches[1]); }));
        server.Post("/api/detect", endpoint([&](auto &, auto &b) {
                        engine.configureAndDetect(b);
                        return engine.state();
                    }));
        server.Post("/api/pools/update", endpoint([&](auto &, auto &b) {
                        engine.updatePool(b);
                        return engine.state();
                    }));
        server.Post("/api/simulate-trade",
                    endpoint([&](auto &, auto &b) { return engine.execute(b.at("id")); }));
        server.Get("/api/trades", endpoint([&](auto &, auto &) { return engine.trades; }));
        server.Get("/api/algorithm/trace", endpoint([&](auto &, auto &) {
                       return scope::json{{"algorithm", engine.algorithm}, {"steps", engine.trace}};
                   }));
        server.Get("/api/benchmarks",
                   endpoint([&](auto &, auto &) { return scope::benchmarks(); }));
        server.Post("/api/replay", endpoint([&](auto &, auto &b) {
                        if (!b.at("index").is_number_integer())
                            throw std::invalid_argument("Snapshot index must be an integer");
                        engine.replay(b.at("index"));
                        return engine.state();
                    }));
        server.Post("/api/live/connect", endpoint([&](auto &, auto &) -> scope::json {
                        throw std::invalid_argument(
                            "Live connector disabled. No RPC or protocol decoder configured.");
                    }));
        server.set_error_handler([](const auto &, auto &res) {
            if (res.body.empty())
                res.set_content(
                    R"({"ok":false,"error":{"code":"NOT_FOUND","message":"Unknown endpoint"}})",
                    "application/json");
        });
        int port = argc > 1 ? std::stoi(argv[1]) : 8080;
        std::cout << "SolanaScope C++ API http://127.0.0.1:" << port
                  << " — synthetic simulation only\n"
                  << std::flush;
        if (!server.listen("127.0.0.1", port)) {
            std::cerr << "Unable to bind port\n";
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
