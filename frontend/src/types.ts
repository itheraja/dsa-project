export interface Token {
  id: string;
  symbol: string;
  decimals: number;
  mint: string;
  name: string;
}
export interface Pool {
  id: string;
  a: string;
  b: string;
  dex: string;
  type: string;
  reserveA: string;
  reserveB: string;
  feeBps: number;
  snapshot: string;
}
export interface Edge {
  id: string;
  pool: string;
  from: string;
  to: string;
}
export interface Graph {
  tokens: Token[];
  pools: Pool[];
  edges: Edge[];
  snapshot: string;
  synthetic: boolean;
}
export interface Hop {
  edge: string;
  pool: string;
  dex: string;
  from: string;
  to: string;
  input: string;
  output: string;
  fee: string;
  feeBps: number;
  rate: number;
  impact: number;
  reserveIn: string;
  reserveOut: string;
}
export interface Opportunity {
  id: string;
  route: string[];
  edges: string[];
  pools: string[];
  asset: string;
  input: string;
  output: string;
  gross: string;
  cost: string;
  net: string;
  roi: number;
  hops: Hop[];
  snapshot: string;
  status: string;
  detectionMs?: number;
  profitCurve: { input: string; net: string }[];
  atCapital?: Opportunity;
  capitalError?: string;
}
export interface Trade extends Opportunity {
  timestamp: number;
  scenario: string;
  cumulativePnl: string;
}
export interface State {
  graph: Graph;
  scenario: string;
  mode: string;
  algorithm: string;
  ranking: string;
  capital: string;
  maxHops: number;
  opportunities: Opportunity[];
  candidates: Opportunity[];
  trades: Trade[];
  metrics: {
    tokens: number;
    pools: number;
    candidates: number;
    profitable: number;
    milliseconds: number;
    expansions: number;
    truncated: boolean;
    traceTruncated: boolean;
    bestNet: string;
    bestRoi: number;
    pnl: string;
  };
  revision: number;
  replayIndex: number;
}
export interface Trace {
  action: string;
  start?: string;
  current?: string;
  depth?: number;
  visited?: string[];
  path?: string[];
  edge?: string;
  weight?: number;
  iteration?: number;
  distances?: Record<string, number | null>;
  negativeCycle?: boolean;
  probe?: string;
}
export interface Benchmark {
  tokens: number;
  pools: number;
  edges: number;
  algorithm: string;
  milliseconds: number;
  cycles: number;
  expansions: number;
  truncated: boolean;
}
export interface Scenario {
  id: string;
  name: string;
  description: string;
}
