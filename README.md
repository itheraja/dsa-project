# SolanaScope

**Graph-Based Arbitrage Detection & Simulation System for Solana**

A working C++20 DSA research engine with a React/TypeScript terminal. Tokens are vertices; directed pool swaps are edges in an adjacency-list multigraph. DFS and Bellman–Ford discover candidate cycles, integer CPMM simulation validates them, bounded trade-size search evaluates investment sizes, and a priority queue ranks opportunities.

**SIMULATED — NO REAL FUNDS TRADED.** All supplied markets and replay frames are synthetic. No wallet, RPC connection, transaction signing, or mainnet execution is implemented.

## Quick start on macOS Apple Silicon

Install Xcode Command Line Tools, CMake ≥3.20, Node.js ≥22.12, npm and Python 3 (Python is only used for HTTP integration tests).

```sh
xcode-select --install
brew install cmake node python
```

From this repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 6
ctest --test-dir build --output-on-failure
./build/solanascope
```

In a second terminal:

```sh
cd frontend
npm ci
npm run dev
```

Open **http://127.0.0.1:5173**. Backend: **http://127.0.0.1:8080**. Both bind to loopback. The Vite proxy forwards `/api` to C++. The API is a single shared local simulation session, not a multiuser service. An optional backend port can be passed as `./build/solanascope 8081`; update the frontend proxy if changing it.

For a built frontend:

```sh
cd frontend
npm run build
npm run preview
```

Open http://127.0.0.1:4173 while the backend runs. Preview includes the same API proxy.

### Offline presentation

The **first dependency installation requires internet**. CMake fetches pinned nlohmann/json 3.11.3, cpp-httplib 0.18.3 and GoogleTest 1.15.2; npm installs the committed lockfile. Once installed/built, the executable, frontend, fixtures, fonts and calculations require no internet. Keep the repository, `build/` and `frontend/node_modules/` on the demonstration machine. Do not remove the CMake dependency cache before an offline rebuild. The executable embeds the source data path; moving the repository requires reconfiguring/rebuilding.

## Demonstration

1. Press **Reset Demo**. This restores the profitable fixture, 1,000 USDC capital, DFS, four-hop limit, net-profit ranking, and clears the ledger.
2. Run detection. Select a route, inspect exact pools and swap amounts, and press **Animate**.
3. Open **Algorithm Inspector**. Step through C++ DFS visits, discoveries and backtracking.
4. Return to the graph and **Simulate trade**. Inspect the reserve changes and the ledger.
5. Click a pool edge (or click a token and choose an outgoing edge) to edit reserves and fee basis points.
6. Select **Balanced market** or **Fee shock** to demonstrate zero profitable opportunities.
7. Select Bellman–Ford and rerun detection. Compare extracted candidates and traces with DFS.
8. Select **Snapshot Replay · synthetic** and advance four frames manually or with Start.
9. Run measured benchmarks in the Inspector.

**Start/Pause:** Demo repeatedly recalculates the current, unchanged market every 3.5 seconds; it does not invent price movements or auto-execute trades. Replay advances synthetic frames and stops after frame four. Algorithm trace playback has separate Play/Pause controls.

Selecting a demo scenario resets reserves and clears the simulation ledger. Replay preserves the ledger but disables execution and edits. Reset Demo restores the complete classroom baseline. The ledger is in-memory and is lost on backend restart.

## Verified fixture result

At the default 1,000 USDC capital ceiling, the engine selects input **999.999991 USDC**, returns **1058.562932 USDC**, deducts **0.080000 USDC** in modeled costs and reports **58.482941 USDC** net simulated profit. This emerges from reserve ratios and rounded sequential swaps; it is not injected into the results. Tiny input differences near the ceiling are caused by integer rounding and the tie-break favoring smaller input.

| Scenario | Meaning | Expected result |
|---|---|---|
| `profitable` | JUP/USDC pool has 110,000 USDC against 100,000 JUP | Positive net cycles |
| `balanced` | Cross prices agree | No profitable opportunities |
| `competing` | Parallel SOL pool and RAY pool also differ | Multiple ranked routes |
| `fee-shock` | Same opening imbalance, 500 bps fees | No profitable opportunities |

All DEX labels explicitly refer to research pools. These are not accurate Raydium, Whirlpool or DLMM adapters.

## Repository map

```text
CMakeLists.txt
core/include/{Graph,Algorithms,Engine}.hpp   # public C++ interfaces
core/src/Graph.cpp                          # indexing, integer parsing, CPMM
core/src/Algorithms.cpp                     # DFS, Bellman–Ford, optimizer, benchmark
core/src/Engine.cpp                         # ranking, scenarios, state, ledger
backend/main.cpp                           # loopback HTTP API, synchronized state
frontend/src/{App,GraphView,main}.tsx        # React dashboard and graph
frontend/src/components/                   # chart and pool editor
frontend/src/{types,api}.ts                 # typed contracts and API client
frontend/src/style.css                     # dark responsive design + Tailwind
frontend/package-lock.json                 # pinned frontend dependency graph
data/scenarios/*.json                       # four deterministic market fixtures
data/snapshots/replay.json                  # synthetic replay frames
tests/core_tests.cpp                       # GoogleTest suites
tests/api_integration.py                   # black-box HTTP acceptance checks
benchmarks/{main.cpp,results.json}          # executable and measured example output
docs/                                      # architecture, maths, report, viva guide
```

## Validation commands

```sh
ctest --test-dir build --output-on-failure
python3 tests/api_integration.py ./build/solanascope
./build/scope_benchmark
cd frontend
npm run build
npm audit
```

The HTTP tests start and stop their own backend on a free loopback port and do not change the running demo. `benchmarks/results.json` contains actual machine measurements, not expected constants. See [verification notes](docs/VERIFICATION.md).

## Design limits

- Integer amounts are bounded to 10^15 atomic units; decimals are 0–9. Clang/GCC `__int128` intermediates prevent multiplication overflow under these limits. Boost is unnecessary for this bounded release; an unbounded/general u64 implementation should use wider multiprecision arithmetic. MSVC is not supported by this arithmetic implementation.
- Starting asset is USDC. Capital range is 0.001–100,000 USDC. Costs are explicitly synthetic: 0.05 USDC fixed + 0.01 USDC per hop, with a 0.01 USDC minimum net threshold.
- Trade-size optimization is a bounded numerical search, not a proof of the global integer optimum.
- DFS caps traversal at 20,000 edge examinations and trace storage at 5,000 events. Partial traversal is visibly reported.
- Bellman–Ford extracts predecessor cycles containing USDC; it is not an exhaustive cycle enumerator and can miss profitable routes. One-whole-token rate probes are approximations.
- No CLMM/DLMM pricing, Token-2022 extensions, real gas quotes, durable database, account discovery, live RPC or transaction execution.

## Documentation

- [Architecture and API](docs/ARCHITECTURE.md)
- [Graph algorithms and complexity](docs/GRAPH-ALGORITHMS.md)
- [AMM mathematics and accounting](docs/AMM-MATHEMATICS.md)
- [Academic project report](docs/PROJECT-REPORT.md)
- [10–15 minute presentation and viva guide](docs/PRESENTATION-GUIDE.md)
- [Production roadmap](docs/PRODUCTION-ROADMAP.md)

Implementation references: [React Flow](https://reactflow.dev/api-reference/react-flow), [cpp-httplib](https://github.com/yhirose/cpp-httplib), [GoogleTest/CMake](https://google.github.io/googletest/quickstart-cmake.html), [nlohmann/json](https://json.nlohmann.me/).
