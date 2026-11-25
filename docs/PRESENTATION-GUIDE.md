# Presentation and viva guide

## Preparation

Build and install dependencies while online. Run `ctest --test-dir build --output-on-failure` and `npm run build` in `frontend/`. Start the backend and frontend, open the dashboard, and press Reset Demo. Keep `core/src/Algorithms.cpp`, `core/src/Graph.cpp` and `core/src/Engine.cpp` ready in your editor. The presentation works without internet once dependencies are installed.

Use a desktop-width browser. If a projector changes the viewport, press the graph's Fit View control. All reserve inputs are token units, not atomic integers. Scenario selection and Reset Demo clear the ledger.

## A 12-minute walkthrough

| Time | Action | Explanation |
|---|---|---|
| 0:00–1:00 | Show simulation banner and liquidity graph | “This is a synthetic research market; no wallets or real funds are involved.” |
| 1:00–2:00 | Click USDC, inspect adjacency, highlight parallel USDC/SOL pools | “Vertices are tokens. Each pool produces two directed edges. Parallel pools make this a multigraph.” |
| 2:00–3:30 | Open Algorithm Inspector, Step Forward, Play, Pause, Step Back | “The C++ recursive DFS emits these events. A visited set prevents repeated interior vertices. Backtracking restores the path.” |
| 3:30–4:30 | Reset Demo and Run detection | “Twelve candidate cycles are evaluated; only those passing exact costs and minimum profit appear in the profitable table.” |
| 4:30–5:30 | Select first route and Animate | “The route stores precise pool edges, so two token-identical paths can be economically different.” |
| 5:30–6:30 | Read each hop and profit curve | “Integer outputs feed the next swap. Fees round up, outputs down. Net subtracts the explicit synthetic execution-cost model.” |
| 6:30–7:30 | Simulate trade; open ledger | “The engine revalidates, changes reserves and records the computed result. It then searches the changed graph.” |
| 7:30–8:30 | Reset, click JUP → gamma pool, set USDC reserve to 100000, Apply | “Removing the price imbalance makes the market unprofitable. This result is calculated, not a frontend toggle.” |
| 8:30–9:30 | Reset; choose Bellman–Ford; Run; open Inspector | “Negative logarithms turn rate products into weight sums. A negative cycle is only a candidate because AMM rates depend on size.” |
| 9:30–10:30 | Run benchmarks and show source | “DFS enumeration is exponential; Bellman–Ford relaxation is O(VE). These times are actual measurements with traces disabled.” |
| 10:30–11:30 | Switch to Snapshot Replay and advance frames | “These four frames are synthetic JSON recordings. Opportunities appear and disappear as reserves or fees change.” |
| 11:30–12:00 | Show passing tests and Reset Demo | “The demo is deterministic, offline and reproducible. Live protocol integration is future work.” |

Optional experiment: set capital to `100000` and rerun. Compare the best sampled investment with the “At capital limit” result. The full investment can lose money because of price impact. Restore default settings before the main presentation.

## Likely viva questions

**Why not a simple graph?** Two DEX pools can connect the same tokens with different reserves and fees. A simple edge would discard pool identity and competing liquidity.

**Why two directed edges per pool?** Each direction has a different input/output reserve ratio. Both edges reference the same underlying pool, so a trade affects both directions.

**What prevents infinite DFS recursion?** Unique visited tokens, a maximum hop limit and an edge-examination budget. Returning to USDC completes a route rather than recursing further.

**Can DFS be O(V+E)?** Reachability DFS can. Enumerating all bounded simple cycles is a different task and can grow exponentially with branching and hop count.

**How are duplicates prevented?** A fixed starting asset removes rotational duplication; exact directed edge sequences distinguish routes. Interior vertices and pool reuse are excluded. Bellman–Ford canonicalizes to USDC and deduplicates sequences.

**Why does −ln(rate) work?** Logarithms transform the product of exchange rates into a sum; a product greater than one yields a negative sum after negation. This holds for the quoted approximation, not every actual investment size.

**Why doesn't Bellman–Ford find every DFS opportunity?** The predecessor method extracts selected negative cycles, not all simple cycles. Probe-size assumptions and the required starting asset also affect discovery.

**Why integers?** Token balances are discrete atomic units. Floating-point arithmetic can silently change boundary rounding and profit decisions. Wide intermediates avoid overflow; floating-point values are reserved for display/approximate discovery.

**Why no Boost?** Inputs are explicitly limited to 10^15 atomic units. A signed 128-bit intermediate safely holds the at-most-10^30 swap product. Supporting unrestricted protocol integer ranges would require revisiting that proof, potentially with Boost.Multiprecision.

**Where are fees kept?** All input, including fee, enters the input reserve. Only the effective input contributes to output quotation. The reserve product therefore does not decrease under this adapter.

**Is the optimized input mathematically exact?** Every quote is exact under the model. The choice of input is a bounded coarse/refined numerical search; it is not a certified global integer optimum.

**Does simulated execution alter the market?** Yes. It commits reserve changes and reruns detection. It rejects stale opportunity IDs and unprofitable routes.

**Why can a reset produce new IDs?** IDs include revision/run counters to prevent stale execution. Financial outputs and traversal order remain reproducible; timings and timestamps naturally differ.

**Can the application trade on Solana?** No. Live data is explicitly disconnected, and no signing or transaction submission code exists. Synthetic CPMM assumptions must not be confused with Whirlpool or DLMM mathematics.

## Recovery

If the backend indicator is disconnected, restart `./build/solanascope` and press Reconnect (or reload the page). If port 8080 is occupied, stop the old backend you started rather than running two sessions. If the page does not load, run `npm run dev` in `frontend/`. Reset Demo is the fastest way to restore a known classroom state. Do not reinstall dependencies during an offline presentation.
