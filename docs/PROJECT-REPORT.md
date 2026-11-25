# SolanaScope — Academic Project Report

## Abstract

SolanaScope demonstrates graph-based arbitrage discovery and sequential liquidity simulation in C++20. Synthetic token markets are modeled as directed weighted multigraphs with shared pool reserve state. Depth-first search enumerates bounded simple cycles; Bellman–Ford extracts candidate negative cycles from approximate logarithmic exchange weights. Integer constant-product simulation, a bounded trade-size search and heap ranking evaluate practical profitability under explicit modeled execution costs. A React dashboard visualizes the graph, algorithm traces and simulated ledger. Reproducible fixtures and automated unit/integration tests support an offline university demonstration without representing synthetic results as live market opportunities.

## Introduction

Decentralized exchanges maintain separate liquidity pools. Two pools can quote different prices for the same pair, and a sequence of swaps can return to its original asset. This makes arbitrage a useful application of multigraph traversal, shortest-path reasoning and numerical evaluation. However, the existence of a favorable product of fixed exchange rates is insufficient: swap size, pool fees, token precision and reserve depletion affect the final result.

## Problem statement and objectives

The project investigates how to identify and explain cyclic exchange routes while preserving the identity and state of each liquidity pool. Its objectives are to implement genuine C++ graph structures, demonstrate DFS and Bellman–Ford, validate candidates using exact accounting, optimize a bounded investment and provide a visual interface suitable for academic evaluation. It also establishes explicit boundaries for future blockchain integration.

## Methodology

The engine loads deterministic JSON tokens and pools, validates them and constructs two directed edges per pool. DFS starts at USDC, tracks visited tokens and pool IDs, and emits structured visit/cycle/backtrack events. Bellman–Ford transforms one-token swap quotes with negative logarithms, relaxes distances and extracts predecessor cycles. Every candidate is evaluated by a common simulator, which passes integer outputs to the next hop. A coarse investment grid with local refinement finds a high-net-profit feasible input. A priority queue orders accepted candidates. The API exposes immutable results to the frontend.

Four controlled scenarios isolate the effects of imbalance, aligned prices, competition and fee shock. Additional tests examine high price impact by increasing the capital ceiling. Simulated execution revalidates and updates pool reserves, allowing the next detection to observe the economic effects of the trade.

## Data structures and algorithms

The graph combines vectors, hash maps and adjacency lists. Vectors provide deterministic iteration; hash maps support expected constant-time ID lookup; adjacency lists avoid scanning every edge for each DFS step. Sets enforce unique vertices and pools. Recursive DFS uses an implicit stack. Bellman–Ford stores distance and predecessor maps. A priority queue ranks opportunities. JSON vectors provide trace and ledger serialization.

Theoretical and measured costs are separated. Cycle enumeration is exponential in hop depth and branching, while Bellman–Ford relaxation is O(VE). Optimization multiplies candidate cost by the number of sampled sizes. This implementation copies the graph for simulation isolation, an intentional overhead disclosed in the complexity analysis.

## System design

The C++ core is independent of the HTTP server. The server serializes access to a single local research session. The React frontend owns only display, controls and animation state. Financial data is transported as decimal strings; JavaScript converts numbers solely for formatting and plotting. Demo and replay use the same algorithms. Live Solana mode explicitly reports Not Connected.

## Results and evaluation

The default fixture contains six tokens, eight liquidity pools and sixteen directed edges. DFS at a four-hop limit discovers twelve candidate cycles, of which three pass the default net threshold. The best route returns 1058.562932 USDC from 999.999991 USDC; after 0.080000 USDC modeled costs its net result is 58.482941 USDC. The two parallel USDC/SOL pools yield separately identifiable routes. A third route uses RAY.

Aligned-price and fee-shock fixtures produce no accepted opportunities. A large capital-limit experiment demonstrates that spending more can lose money because of price impact. Simulated execution changes reserves and candidate rankings; a reset restores exact quote values. Bellman–Ford discovers a profitable candidate but fewer routes than exhaustive bounded DFS, illustrating its different purpose.

GoogleTest covers parsing, graph mutation, parallel pools, traversal determinism, route uniqueness, negative cycles, integer quotations, fees, reserve invariants, optimization, scenario behavior, ranking, ledger updates, stale execution, replay and disabled live mode. Black-box HTTP tests exercise success/error envelopes and the full detection/execution/reset workflow. Frontend TypeScript and production builds are checked, and browser interaction verifies the integrated demonstration. Exact run evidence is recorded in `VERIFICATION.md`.

Traversal benchmark datasets vary from six to forty-eight tokens and eighteen to 144 pools. The stored measured sample reports DFS around 0.052–0.056 ms and Bellman–Ford around 0.006–0.074 ms on these small balanced graphs. These are machine-specific measurements, not general performance guarantees. Fixed local degree and bounded depth explain nearly flat DFS time; balanced rates permit early Bellman–Ford exit.

## Limitations

The market is entirely synthetic. The system has no production protocol decoder, CLMM or DLMM implementation, real execution-cost feed, wallet, atomic transaction builder or transaction sender. Accounting is bounded to supported atomic-unit ranges. Optimized sizes are best among sampled inputs and are not proven global integer optima. Bellman–Ford may miss cycles, particularly those outside the starting asset. The in-memory ledger is session-scoped, and the local HTTP service has no multiuser isolation. Graph updates trigger a full detection rather than incremental affected-route recomputation.

## Future enhancements

Future work should first add immutable slot-tagged snapshots, verified protocol-specific decoders and recorded historical fixtures. Differential pricing tests and risk validation should precede any executor. A durable ledger, incremental route indexes, profiling, expanded graph datasets and property-based tests would strengthen the research implementation. Transaction simulation, secure signing, atomic execution controls and external assessment are prerequisites for real capital.

## Conclusion

SolanaScope connects textbook data structures to a stateful financial simulation while keeping mathematical assumptions observable. Its primary contribution is an integrated, reproducible classroom demonstration of multigraph representation, cycle discovery, exact sequential simulation and opportunity ranking. It is a research release, not a production trading system.
