# Constant-product AMM mathematics

## Units and exactness

Amounts are signed 64-bit integers in token atomic units, bounded to 10^15. Token decimal precision is 0–9. A USDC amount of `1.000001` parses to 1,000,001 units; SOL has nine decimals. Parsing rejects negatives, scientific notation, too many decimal places and out-of-range values. API strings preserve exact decimals.

All multiplications affecting balances use signed 128-bit intermediates. The largest reserve-times-effective-input product is at most 10^30, below 2^127−1. Reserves plus input are checked before mutation. Fee products are at most 10^19. The model does not use `double` as the source of accounting truth. Derived rate, ROI, impact and elapsed time are floating-point display values.

## CPMM quotation

For input-side reserve x, output-side reserve y, raw input a and fee f in basis points:

```text
fee        = ceil(a × f / 10,000)
effective  = a − fee
output     = floor(y × effective / (x + effective))
new_x      = x + a
new_y      = y − output
```

Fee rounding up is an explicit conservative synthetic adapter rule. It is not a claim about every Solana protocol's rounding conventions. Fees remain in the input reserve, so the post-swap reserve product is at least the prior product, subject to integer rounding. Both directions of a pool observe the same updated reserves.

For x=1,000,000, y=2,000,000, a=10,000 and f=30: fee=30, effective=9,970 and output=19,743. A one-unit trade at a positive fee can round to zero and is rejected as an executable swap.

The displayed effective rate is output/input, normalized for differing token decimals. Price impact is `100 × (1 − output / (effective × y/x))`; it excludes the fee itself but includes output rounding. It is an explanatory metric, not a reserve/accounting input.

## Sequential route evaluation

The first hop receives the chosen investment. Each subsequent hop receives exactly the prior integer output. A graph copy is mutated during quotation so repeated economic state cannot be priced independently. The validator requires continuous edges, at least two and at most six hops, unique pools, unique internal vertices and a return to the starting token.

```text
gross = final USDC − initial USDC
cost  = 0.05 USDC + number_of_hops × 0.01 USDC
net   = gross − cost
ROI   = 100 × net / initial USDC
```

Costs are explicitly modeled assumptions denominated in USDC. There is no claim that these equal current Solana network fees. A candidate passes only if net ≥0.01 USDC. The ledger records gross final output, costs and net separately; cost is deducted in P&L, not from AMM reserves.

## Worked synthetic result

Default three-hop route, discovered from the profitable fixture:

| Hop | Exact pool | Input | Fee | Output |
|---|---|---:|---:|---:|
| USDC → SOL | alpha-usdc-sol | 999.999991 USDC | 3.000000 USDC | 9.871580255 SOL |
| SOL → JUP | beta-sol-jup | 9.871580255 SOL | 0.029614741 SOL | 974.604527 JUP |
| JUP → USDC | gamma-jup-usdc | 974.604527 JUP | 2.923814 JUP | 1058.562932 USDC |

Gross = 58.562941 USDC; modeled cost = 0.080000; net = 58.482941. These values are reproducible C++ outputs. The fixture's final pool has 100,000 JUP and 110,000 USDC; all other baseline cross prices align. Changing its USDC reserve to 100,000 removes the opening.

## Size and feasibility

The optimizer evaluates a coarse grid and refines near the best result. At a 100,000 USDC ceiling, the best executable input is below the ceiling; spending the entire amount causes sufficient price impact to lose money on the same route. The UI displays both the optimized result and the direct capital-limit evaluation. No feasible sampled input means no executable candidate is reported. No global-optimality guarantee is made.

This model omits concentrated ticks, bins, transfer taxes, protocol fee splitting, slippage between snapshots, transaction failure, rent, priority-fee estimation and adversarial ordering. Separate verified adapters and execution controls are required before any production use.
