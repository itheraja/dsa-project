# Verification record

Verified locally on macOS arm64 with AppleClang 17, CMake 4.2.3 and Node 25.2.1. Commands and results below were actually run during implementation.

| Check | Result |
|---|---|
| Release CMake configure/build | Passed: core, backend, benchmark, GoogleTest executable |
| `ctest --test-dir build --output-on-failure` | **19/19 CTest entries passed** (18 GoogleTest cases + HTTP integration suite) |
| HTTP integration suite | Four black-box tests passed, including the full detect/inspect/simulate/reset flow |
| Frontend `npm run build` | Passed TypeScript checking and Vite production compilation |
| `npm audit` | **0 vulnerabilities** in the installed lockfile at verification time |
| Benchmarks | Real measurements produced for 6/12/24/48-token datasets, both algorithms |
| Browser integrated workflow | Passed the interactions listed below |

The final CTest run completed in 1.45 seconds. Durations are machine-specific and vary. Financial outputs below are deterministic; IDs, timestamps and execution timings are not.

## Deterministic acceptance result

- Six tokens, eight pools, sixteen directed edges.
- Default DFS, max four hops: twelve candidate cycles, three profitable routes.
- Selected route: USDC → SOL → JUP → USDC.
- Exact pools: alpha-usdc-sol, beta-sol-jup, gamma-jup-usdc.
- Investment: 999.999991 USDC.
- Final output: 1058.562932 USDC.
- Gross: 58.562941 USDC; modeled cost: 0.080000 USDC.
- Net simulated profit: **58.482941 USDC**.
- Balanced and fee-shock fixtures: no profitable opportunities.

## Browser checks

Using the actual dashboard against the C++ server:

1. Loaded graph, metrics, opportunity rows and exact per-hop amounts.
2. Executed a simulated trade and verified the matching 58.482941 USDC ledger result and changed opportunity rankings.
3. Reset the demo and verified restored amounts and empty ledger.
4. Stepped DFS from USDC to SOL, with C++ visited set and exact edge path visible.
5. Selected Bellman–Ford, reran detection, viewed the −ln(rate) weights and loaded actual benchmark rows/charts.
6. Opened JUP token inspection and its outgoing adjacency list, then opened the gamma pool editor.
7. Changed the USDC reserve from 110000 to 100000; the UI recalculated to zero profitable opportunities.
8. Entered synthetic replay, advanced from balanced frame to profitable frame and verified simulated execution remained disabled.
9. Entered live mode and verified the explicit Not Connected view.
10. Reloaded the final graph implementation, verified all six nodes were visible, and verified a single route edge animated at the current hop.

Testing caught and fixed a controlled-node sizing issue that left React Flow nodes hidden. The final graph specifies stable node dimensions and handle geometry. A single-point ledger chart now renders its point, and the investment chart uses a numeric x-axis. A dependency audit finding in the initial Vite pin was resolved by updating to 7.3.7.

## Scope of evidence

The suite exercises synthetic CPMM behavior and local HTTP integration. It does not validate real Solana pool decoding, current network costs, market profitability, transaction delivery or production deployment. Browser checks are interactive verification, not a committed automated browser runner. The backend ledger remains in-memory. Dependency audit results reflect the verification time and should be rerun before future releases.
