# Production roadmap

This milestone is a local research simulator. The following work is **not implemented** and must not be represented as a deployable trading system.

## 1. Verified read-only market data

Implement `MarketDataConnector` with configured RPC endpoints and account subscriptions. Proposed future configuration names are `SOLANA_RPC_HTTP_URL`, `SOLANA_RPC_WS_URL`, `SOLANA_COMMITMENT` and a program/pool allowlist. These are design proposals; setting them in this release will not enable anything. Keep provider credentials outside source control.

The connector should validate account owners, data layouts, program versions, token mints/decimals, reserve sources, slots, freshness and commitment. Batch related accounts into coherent immutable snapshots, handle reconnects and out-of-order updates, and make gaps visible. Discover accounts only through documented program interfaces and allowlists. Persist raw captured data with source/slot metadata; label it separately from synthetic fixtures.

Acceptance gate: replay independently captured snapshots, compare decoded states with authoritative on-chain account data, and ensure stale or mixed-slot snapshots cannot produce executable results.

## 2. Protocol-specific pool adapters

Implement actual protocol fee conventions and rounding. Constant-product, concentrated-liquidity ticks and bin-based liquidity require distinct state and mathematics. Add Token-2022 transfer-fee/extension handling where supported, explicit rejection elsewhere, and account for liquidity crossing, protocol fees and reserve constraints. Do not rename the current CPMM adapter to imply protocol accuracy.

Acceptance gate: differential tests against official SDK/on-chain behavior, edge-case vectors, invariant/property tests and overflow analysis over supported protocol ranges.

## 3. Research and risk infrastructure

Add persistent snapshot/trade storage, replay clocks, deterministic seeds, profiling, route-to-pool dependency indexes, bounded caches and affected-route recomputation. Expand benchmarks by graph density and hop depth. Separate data ingestion from immutable search snapshots so slow clients cannot block updates. Use explicit state/version checks, stale-quote expiry, liquidity limits, conservative slippage, capital exposure limits, modeled failure costs and circuit breakers.

Acceptance gate: paper trading on recorded data with realistic latency, costs, failed execution and survivorship controls. Favor reproducible accounting and risk outcomes over headline opportunity counts.

## 4. Transaction construction without real capital

Build and simulate atomic multi-hop transactions on appropriate test environments. Validate account metas, compute budgets, blockhash validity, address lookup tables and token program behavior. Require minimum final balance and per-hop bounds so a partially unfavorable sequence cannot settle. Separate signing from strategy processes and use hardware-backed or otherwise isolated key management. A Rust on-chain executor can enforce atomic route constraints if needed; it is outside this release.

Acceptance gate: adversarial test cases, transaction simulation parity, permission boundaries, recovery procedures and independent security review. Simulated success must never be labeled a confirmed on-chain trade.

## 5. Delivery, monitoring and capital protection

Evaluate transaction delivery, including Jito interfaces, only after checking current official documentation and applicable operational constraints. Measure actual inclusion, contention, ordering risk and delivery cost. Record submitted/simulated/included/finalized/failed states distinctly, reconcile balances independently and monitor inventory, exposures, latency, RPC divergence and rejected routes. Add emergency stop, per-strategy caps, loss limits, audit logs and manual recovery controls.

Acceptance gate: reviewed secure deployment, independent assessment, clearly bounded trial capital and explicit operator authorization. No component in the current repository is a real-money transaction sender, and no production-readiness claim is made.
