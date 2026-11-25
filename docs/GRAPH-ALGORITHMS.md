# Graph data structures and algorithms

## Directed multigraph

A token is a vertex. A pool connects two tokens with two directed swap edges. Different pools between the same pair are distinct edges, so `USDC → SOL` alone does not identify an executable swap. Every path therefore stores edge IDs, not merely token symbols. A simple graph would discard competing pool prices.

Vectors preserve insertion order for deterministic traversal; `unordered_map` indexes resolve token/pool/edge IDs in expected O(1). The adjacency map stores outgoing edge indexes and traverses in O(outdegree). Both directions share the authoritative pool object. Pool changes need no topology rebuild; candidate routes are rediscovered after each mutation. This initial release deliberately recalculates all candidates rather than maintaining an incremental route cache.

## DFS with backtracking

Implementation: `DFSDetector::detect` in `core/src/Algorithms.cpp`.

1. Insert starting token USDC in the visited set.
2. Traverse each outgoing edge in fixture order, retaining its exact ID.
3. If it returns to USDC with at least two edges, record the cycle.
4. Otherwise recurse only if the destination has not been visited and the hop budget remains.
5. Remove the edge, visited token and pool on return; emit a backtrack event.

A path may not reuse a pool or repeat an interior vertex. Two-hop cycles must therefore use distinct pools. Starting at USDC eliminates rotational duplicates. Parallel-pool routes with the same token sequence are **different valid opportunities**, not duplicates. Reverse routes are different directed cycles.

DFS records current node, depth, visited set, edge path and cycle events. The visited set is sorted for deterministic serialization; traversal order is represented by the ordered visit events, not the set order. Step Back in the browser rewinds this trace; it does not run a new algorithm.

### Complexity

Let V be vertices, E directed edges, H the hop limit, b maximum outdegree, C discovered candidates and S investment samples. Enumerating simple cycles is exponential in the worst case. A loose bounded estimate is O(b^H · H), including path copying and small set operations; it is not O(V+E) as for a single reachability DFS. Auxiliary recursion/visited memory is O(H); graph space is O(V+E); storing cycles is O(CH). Trace storage adds O(T H), capped at 5,000 events. A 20,000-edge-examination budget bounds traversal work and marks partial results.

## Bellman–Ford

For every directed edge, quote one **whole source token** using CPMM reserves and fees. Convert atomic-unit rate to human token units and assign `w = -ln(rate)`. Zero-output edges receive infinite weight. Set the starting distance to zero and others to infinity. Relax edges in deterministic order for at most V passes. An update on pass V indicates a reachable negative cycle under this approximation.

For each final-pass updated vertex, walk V predecessor edges to enter a cycle, then recover the predecessor loop and reverse it to forward traversal order. Rotate to USDC; reject loops outside the hop limit or repeating pools; deduplicate edge sequences. Every retained candidate is subjected to the **same** sequential integer simulation and optimization as DFS candidates.

The relaxation phase costs O(VE), with O(V+E) auxiliary data including edge weights. Early exit can be faster. Predecessor extraction can add O(EV) work in this implementation. Traces copy distance maps for display and can add O(V) per recorded relaxation; benchmarks turn traces off.

A negative cycle is not proof of executable profit: per-edge rates were quoted independently at different token-unit probe sizes. Actual capital changes at each hop, liquidity is finite, rounding matters and modeled costs remain. Conversely, the predecessor method is not an exhaustive negative-cycle enumerator; some routes may be missed, particularly cycles not containing the chosen starting asset.

## Optimization and ranking

For each candidate, evaluate 48 uniformly spaced inputs and a small baseline input, then perform seven local refinement rounds of at most 17 samples around the best investment. Repeated inputs are skipped. Inputs are bounded, dust outputs are infeasible, and a larger trade may have negative return despite a positive small trade.

The smooth CPMM objective motivates local refinement; atomic rounding means the routine is only a measured bounded search, not a global integer-optimality guarantee. Every sample uses exact rounded financial accounting. ROI and plots are derived descriptive floating-point values.

Simulation currently copies the graph before each evaluation for transactional isolation. Thus evaluation costs O(V+E+H), and all candidate optimization costs O(C S (V+E+H)). Up to 168 inputs are attempted per route before deduplication (48 + 1 + 7×17). Sampling order is deterministic. Sample sorting adds O(S log S) per route.

A `std::priority_queue` ranks accepted results by net profit, ROI, lower cost or fewer hops. Ties use net profit then deterministic discovery index. Heap insertion/extraction costs O(C log C) with O(C) storage. Ranking by ROI selects among net-profit-optimized sizes; it does not separately optimize ROI over investment sizes.

## Benchmarks

`scope_benchmark` builds ring-like synthetic graphs with 6, 12, 24 and 48 tokens, three pools per token and two edges per pool. Every measurement averages five actual runs with hop limit four and traces disabled. Stored sample results are in `benchmarks/results.json`; rerun them on the presentation machine.

These datasets keep local degree approximately constant. DFS therefore levels off when its four-hop neighborhood stops growing, even as total V increases. This does not establish polynomial cycle-enumeration complexity. Bellman–Ford benefits from early exit on these balanced graphs, so measured times are not worst-case bounds. The dashboard's detection latency additionally includes optimization, trace construction, JSON materialization and ranking; it should not be compared directly with traversal-only benchmark time.
