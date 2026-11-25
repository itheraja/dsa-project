import { Chart } from "./components/Chart";
import { PoolEditor } from "./components/PoolEditor";
import { useEffect, useRef, useState } from "react";
import {
  Activity,
  ArrowRight,
  BarChart3,
  ChevronLeft,
  ChevronRight,
  Code2,
  GitBranch,
  Layers,
  Play,
  Pause,
  RotateCcw,
  ScanLine,
  WifiOff,
  X,
  CheckCircle2,
} from "lucide-react";
import {
  ResponsiveContainer,
  XAxis,
  YAxis,
  Tooltip,
  BarChart,
  Bar,
  Legend,
} from "recharts";
import { api } from "./api";
import { GraphView } from "./GraphView";
import type { State, Scenario, Opportunity, Trace, Benchmark } from "./types";
const money = (s: string | number) =>
  Number(s).toLocaleString("en-US", {
    minimumFractionDigits: 2,
    maximumFractionDigits: 4,
  });
export default function App() {
  const [state, setState] = useState<State>(),
    [scenarios, setScenarios] = useState<Scenario[]>([]),
    [selectedId, setSelectedId] = useState(""),
    [tab, setTab] = useState("market"),
    [mode, setMode] = useState("demo"),
    [busy, setBusy] = useState(false),
    [error, setError] = useState(""),
    [notice, setNotice] = useState(""),
    [connected, setConnected] = useState(false),
    [updated, setUpdated] = useState("");
  const [capital, setCapital] = useState("1000"),
    [algorithm, setAlgorithm] = useState("dfs"),
    [maxHops, setMaxHops] = useState(4),
    [ranking, setRanking] = useState("profit"),
    [running, setRunning] = useState(false),
    [hop, setHop] = useState(-1),
    [poolId, setPoolId] = useState(""),
    [tokenId, setTokenId] = useState("");
  const [steps, setSteps] = useState<Trace[]>([]),
    [step, setStep] = useState(0),
    [playing, setPlaying] = useState(false),
    [bench, setBench] = useState<Benchmark[]>([]),
    [showRejected, setShowRejected] = useState(false);
  const requestBusy = useRef(false);
  const selected = state?.candidates.find((o) => o.id === selectedId);
  const pool = state?.graph.pools.find((p) => p.id === poolId);
  const token = state?.graph.tokens.find((t) => t.id === tokenId);
  async function accept(next: State) {
    setState(next);
    setMode(next.mode);
    setConnected(true);
    setUpdated(new Date().toLocaleTimeString());
    setSelectedId((old) =>
      next.candidates.some((o) => o.id === old)
        ? old
        : next.opportunities[0]?.id || "",
    );
    setCapital(next.capital);
    setAlgorithm(next.algorithm);
    setMaxHops(next.maxHops);
    setRanking(next.ranking);
    setPlaying(false);
    setStep(0);
    setHop(-1);
    const trace = await api<{ steps: Trace[] }>("/algorithm/trace");
    setSteps(trace.steps);
  }
  async function action(fn: () => Promise<void>) {
    if (requestBusy.current) return;
    requestBusy.current = true;
    setBusy(true);
    setError("");
    try {
      await fn();
    } catch (e) {
      setError(e instanceof Error ? e.message : "Request failed");
    } finally {
      setBusy(false);
      requestBusy.current = false;
    }
  }
  async function mutate(path: string, body: unknown) {
    setNotice("");
    await action(async () => accept(await api<State>(path, body)));
  }
  useEffect(() => {
    void action(async () => {
      const [s, ss] = await Promise.all([
        api<State>("/state"),
        api<Scenario[]>("/scenarios"),
      ]);
      setScenarios(ss);
      await accept(s);
    });
    const id = window.setInterval(() => {
      void api("/health")
        .then(() => setConnected(true))
        .catch(() => setConnected(false));
    }, 5000);
    return () => clearInterval(id);
  }, []);
  useEffect(() => {
    if (!running) return;
    const id = window.setInterval(() => {
      if (mode === "replay") {
        if ((state?.replayIndex || 0) >= 3) {
          setRunning(false);
          return;
        }
        void mutate("/replay", { index: (state?.replayIndex || 0) + 1 });
      } else if (mode === "demo")
        void mutate("/detect", { capital, algorithm, maxHops, ranking });
    }, 3500);
    return () => clearInterval(id);
  }, [running, mode, state?.replayIndex, capital, algorithm, maxHops, ranking]);
  useEffect(() => {
    if (!playing) return;
    const id = window.setInterval(
      () =>
        setStep((s) => {
          if (s >= steps.length - 1) {
            setPlaying(false);
            return s;
          }
          return s + 1;
        }),
      650,
    );
    return () => clearInterval(id);
  }, [playing, steps.length]);
  useEffect(() => {
    if (hop < 0 || !selected) return;
    const id = window.setTimeout(
      () => setHop((h) => (h + 1 < selected.hops.length ? h + 1 : -1)),
      1500,
    );
    return () => clearTimeout(id);
  }, [hop, selected]);
  async function changeMode(m: string) {
    setRunning(false);
    setPlaying(false);
    if (m === "live") {
      setMode(m);
      return;
    }
    await mutate(
      m === "replay" ? "/replay" : "/scenario/select",
      m === "replay"
        ? { index: 0 }
        : {
            scenario:
              state?.scenario === "synthetic-replay"
                ? "profitable"
                : state?.scenario || "profitable",
          },
    );
  }
  const rows = showRejected ? state?.candidates : state?.opportunities;
  return (
    <div className="app-shell">
      <aside className="sidebar">
        <div className="brand-mark">
          <GitBranch size={25} />
        </div>
        <button
          aria-label="Market workspace"
          className={tab === "market" ? "active" : ""}
          onClick={() => setTab("market")}
        >
          <Layers />
        </button>
        <button
          aria-label="Algorithm Inspector"
          className={tab === "algorithms" ? "active" : ""}
          onClick={() => setTab("algorithms")}
        >
          <Code2 />
        </button>
        <button
          aria-label="Simulation ledger"
          className={tab === "ledger" ? "active" : ""}
          onClick={() => setTab("ledger")}
        >
          <BarChart3 />
        </button>
        <div className="sidebar-bottom">
          SS
          <br />
          <span>01</span>
        </div>
      </aside>
      <div className="workspace">
        <header>
          <div className="brand">
            Solana<span>Scope</span>
            <span className="version">RESEARCH / V1.0</span>
          </div>
          <div className="connection">
            <i className={connected ? "online" : ""} />
            {connected ? "C++ engine connected" : "Backend unavailable"}
          </div>
        </header>
        <div className="simulation-banner">
          <span>
            <span className="square-dot" /> SIMULATED — NO REAL FUNDS TRADED
          </span>
          <span>
            Synthetic CPMM fixtures · Offline-ready research environment
          </span>
        </div>
        <main>
          <div className="title-row">
            <div>
              <div className="eyebrow">QUANTITATIVE RESEARCH WORKSPACE</div>
              <h1>
                {tab === "algorithms"
                  ? "Algorithm Inspector"
                  : tab === "ledger"
                    ? "Simulation ledger"
                    : "Market overview"}
              </h1>
              <p className="muted">
                Explore liquidity. Trace cycles. Validate every swap.
              </p>
            </div>
            <div className="actions">
              <button
                disabled={busy || mode === "live"}
                onClick={() => setRunning(!running)}
              >
                {running ? <Pause size={15} /> : <Play size={15} />}{" "}
                {running ? "Pause" : "Start"}
              </button>
              <button
                disabled={busy}
                onClick={() => {
                  setRunning(false);
                  void mutate("/reset", {});
                }}
              >
                <RotateCcw size={15} />
                Reset Demo
              </button>
            </div>
          </div>
          <div className="control-bar panel">
            <label>
              Operating mode
              <select
                value={mode}
                onChange={(e) => void changeMode(e.target.value)}
                disabled={busy}
              >
                <option value="demo">Demo Simulation</option>
                <option value="replay">Snapshot Replay · synthetic</option>
                <option value="live">Live Solana · Not Connected</option>
              </select>
            </label>
            <label>
              Market scenario
              <select
                value={
                  state?.scenario === "synthetic-replay"
                    ? "profitable"
                    : state?.scenario || "profitable"
                }
                disabled={busy || mode !== "demo"}
                onChange={(e) => {
                  setRunning(false);
                  void mutate("/scenario/select", { scenario: e.target.value });
                }}
              >
                {scenarios.map((s) => (
                  <option key={s.id} value={s.id}>
                    {s.name}
                  </option>
                ))}
              </select>
            </label>
            <div className="freshness">
              <span>INPUT SNAPSHOT</span>
              <strong>{state?.graph.snapshot || "Awaiting engine"}</strong>
              <small>Last calculation {updated || "—"}</small>
            </div>
            {mode === "replay" && (
              <div className="actions">
                <button
                  aria-label="Previous snapshot"
                  disabled={busy || state?.replayIndex === 0}
                  onClick={() =>
                    void mutate("/replay", {
                      index: (state?.replayIndex || 0) - 1,
                    })
                  }
                >
                  <ChevronLeft size={15} />
                </button>
                <span>Frame {(state?.replayIndex || 0) + 1}/4</span>
                <button
                  aria-label="Next snapshot"
                  disabled={busy || state?.replayIndex === 3}
                  onClick={() =>
                    void mutate("/replay", {
                      index: (state?.replayIndex || 0) + 1,
                    })
                  }
                >
                  <ChevronRight size={15} />
                </button>
              </div>
            )}
          </div>
          {error && (
            <div className="error" role="alert">
              {error}
              <button onClick={() => setError("")} aria-label="Dismiss error">
                <X size={14} />
              </button>
            </div>
          )}
          {notice && (
            <div className="notice" role="status">
              <CheckCircle2 size={16} />
              {notice}
              <button
                onClick={() => setNotice("")}
                aria-label="Dismiss notification"
              >
                <X size={14} />
              </button>
            </div>
          )}
          {mode === "live" ? (
            <section className="panel live-state">
              <WifiOff size={40} />
              <h2>Live Solana · Not Connected</h2>
              <p>No RPC source or protocol decoder is configured.</p>
              <p className="muted">
                This release supports synthetic CPMM research. A typed C++
                connector boundary is ready for validated snapshots; live
                pricing and real-money execution are unavailable.
              </p>
              <button onClick={() => void changeMode("demo")}>
                Return to Demo Simulation
              </button>
            </section>
          ) : state ? (
            <>
              <div className="metrics">
                {[
                  [
                    "TOKENS / POOLS",
                    `${state.metrics.tokens} / ${state.metrics.pools}`,
                    "Directed liquidity multigraph",
                  ],
                  [
                    "CANDIDATE CYCLES",
                    state.metrics.candidates,
                    `${state.metrics.profitable} pass profitability threshold`,
                  ],
                  [
                    "BEST NET PROFIT",
                    `$${money(state.metrics.bestNet)}`,
                    `${state.metrics.bestRoi.toFixed(3)}% best estimated ROI`,
                  ],
                  [
                    "ENGINE LATENCY",
                    `${state.metrics.milliseconds.toFixed(2)} ms`,
                    `${state.metrics.expansions} edge examinations`,
                  ],
                  [
                    "SIMULATED P&L",
                    `$${money(state.metrics.pnl)}`,
                    `${state.trades.length} simulated trades`,
                  ],
                ].map(([label, value, sub]) => (
                  <div className="metric panel" key={label}>
                    <span>{label}</span>
                    <strong
                      className={
                        String(label).includes("PROFIT") ||
                        String(label).includes("P&L")
                          ? "positive"
                          : ""
                      }
                    >
                      {value}
                    </strong>
                    <small>{sub}</small>
                  </div>
                ))}
              </div>
              <nav className="tabs">
                <button
                  className={tab === "market" ? "active" : ""}
                  onClick={() => setTab("market")}
                >
                  <GitBranch size={15} />
                  Liquidity graph
                </button>
                <button
                  className={tab === "algorithms" ? "active" : ""}
                  onClick={() => setTab("algorithms")}
                >
                  <Code2 size={15} />
                  Algorithm Inspector
                </button>
                <button
                  className={tab === "ledger" ? "active" : ""}
                  onClick={() => setTab("ledger")}
                >
                  <Activity size={15} />
                  Simulation ledger<span>{state.trades.length}</span>
                </button>
              </nav>
              {tab !== "ledger" && (
                <div className="detect-bar">
                  <label>
                    Capital limit · USDC
                    <input
                      aria-label="Capital limit"
                      value={capital}
                      onChange={(e) => setCapital(e.target.value)}
                    />
                  </label>
                  <label>
                    Algorithm
                    <select
                      value={algorithm}
                      onChange={(e) => setAlgorithm(e.target.value)}
                    >
                      <option value="dfs">DFS + backtracking</option>
                      <option value="bellman-ford">Bellman–Ford</option>
                    </select>
                  </label>
                  <label>
                    Max hops
                    <select
                      value={maxHops}
                      onChange={(e) => setMaxHops(Number(e.target.value))}
                    >
                      {[2, 3, 4, 5, 6].map((n) => (
                        <option key={n}>{n}</option>
                      ))}
                    </select>
                  </label>
                  <label>
                    Rank by
                    <select
                      value={ranking}
                      onChange={(e) => setRanking(e.target.value)}
                    >
                      <option value="profit">Net profit</option>
                      <option value="roi">Return on capital</option>
                      <option value="cost">Execution cost</option>
                      <option value="hops">Route length</option>
                    </select>
                  </label>
                  <button
                    className="primary"
                    disabled={busy}
                    onClick={() =>
                      void mutate("/detect", {
                        capital,
                        algorithm,
                        maxHops,
                        ranking,
                      })
                    }
                  >
                    <ScanLine size={16} />
                    {busy ? "Calculating…" : "Run detection"}
                  </button>
                </div>
              )}
              {state.metrics.truncated && (
                <div className="error">
                  Traversal budget reached. Results are partial; lower the hop
                  limit.
                </div>
              )}
              {tab !== "ledger" && (
                <div className="research-grid">
                  <section className="panel graph-panel">
                    <div className="section-heading">
                      <div>
                        <h2>
                          Liquidity network{" "}
                          <span className="badge">DIRECTED MULTIGRAPH</span>
                        </h2>
                        <p>Click a token or pool to inspect its state</p>
                      </div>
                      <span className="graph-legend">
                        <i />
                        Selected route
                      </span>
                    </div>
                    <div className="graph-canvas">
                      <GraphView
                        graph={state.graph}
                        selected={tab === "market" ? selected : undefined}
                        hop={hop}
                        trace={tab === "algorithms" ? steps[step] : undefined}
                        onPool={setPoolId}
                        onToken={setTokenId}
                      />
                    </div>
                    <div className="graph-footer">
                      <span>
                        {state.graph.edges.length} directed edges · parallel
                        pools enabled
                      </span>
                      <span>
                        {tab === "algorithms"
                          ? `Trace step ${step + 1} / ${steps.length}`
                          : "Pan to explore · Scroll to zoom"}
                      </span>
                    </div>
                  </section>
                  {tab === "market" ? (
                    <section className="panel detail-panel">
                      <div className="section-heading">
                        <h2>Route analysis</h2>
                        <span className="badge">CPMM</span>
                      </div>
                      {selected ? (
                        <>
                          <div className="route-title">
                            {selected.route.join(" → ")}
                          </div>
                          <div className="profit-hero">
                            <span>Estimated net profit</span>
                            <strong
                              className={
                                Number(selected.net) > 0
                                  ? "positive"
                                  : "negative"
                              }
                            >
                              ${money(selected.net)}
                            </strong>
                            <small>
                              {selected.roi.toFixed(4)}% return ·{" "}
                              {selected.hops.length} swaps
                            </small>
                          </div>
                          <div className="breakdown">
                            <div>
                              <span>Optimized investment</span>
                              <b>{selected.input} USDC</b>
                            </div>
                            <div>
                              <span>Final amount</span>
                              <b>{selected.output} USDC</b>
                            </div>
                            <div>
                              <span>Gross profit</span>
                              <b>{selected.gross}</b>
                            </div>
                            <div>
                              <span>Modeled execution costs</span>
                              <b>−{selected.cost}</b>
                            </div>
                          </div>
                          <div className="hop-list">
                            {selected.hops.map((h, i) => (
                              <div
                                className={`hop ${hop === i ? "current" : ""}`}
                                key={h.edge}
                              >
                                <span className="hop-number">{i + 1}</span>
                                <div>
                                  <strong>
                                    {h.from}
                                    <ArrowRight size={12} />
                                    {h.to}
                                  </strong>
                                  <small>{h.pool}</small>
                                  <p>
                                    {h.input} → {h.output}
                                  </p>
                                  <small>
                                    Fee {h.fee} {h.from} ({h.feeBps} bps) ·
                                    Impact {h.impact.toFixed(3)}%
                                  </small>
                                  <small>
                                    Effective rate {h.rate.toPrecision(8)}{" "}
                                    {h.to}/{h.from}
                                  </small>
                                </div>
                              </div>
                            ))}
                          </div>
                          <div className="actions">
                            <button onClick={() => setHop(hop < 0 ? 0 : -1)}>
                              {hop < 0 ? (
                                <Play size={14} />
                              ) : (
                                <Pause size={14} />
                              )}
                              Animate
                            </button>
                            <button
                              className="primary"
                              disabled={
                                busy ||
                                mode !== "demo" ||
                                selected.status !== "profitable"
                              }
                              onClick={() =>
                                void action(async () => {
                                  const trade = await api<Opportunity>(
                                    "/simulate-trade",
                                    { id: selected.id },
                                  );
                                  await accept(await api<State>("/state"));
                                  setNotice(
                                    `Simulated trade recorded: ${trade.net} USDC net profit. No real funds traded.`,
                                  );
                                })
                              }
                            >
                              Simulate trade
                            </button>
                          </div>
                          <p className="muted small">
                            Snapshot {selected.snapshot}. Costs: 0.05 USDC +
                            0.01 per hop. Minimum net: 0.01 USDC.
                          </p>
                        </>
                      ) : (
                        <div className="empty">
                          <ScanLine size={32} />
                          <h3>No profitable opportunities</h3>
                          <p>
                            Fees, price impact and execution costs exceed the
                            return. Inspect rejected candidates below.
                          </p>
                        </div>
                      )}
                    </section>
                  ) : (
                    <section className="panel inspector">
                      <div className="section-heading">
                        <h2>
                          {state.algorithm === "dfs"
                            ? "DFS traversal"
                            : "Bellman–Ford relaxations"}
                        </h2>
                        <span className="badge">C++ TRACE</span>
                      </div>
                      <p className="muted">
                        {state.algorithm === "dfs"
                          ? "Visited tokens and exact pool edges are emitted by the recursive C++ traversal."
                          : "Weights use −ln(rate), quoted at one whole source token. Negative cycles are candidates; exact AMM validation decides profit."}
                      </p>
                      <div className="trace-controls">
                        <button
                          aria-label="Reset trace"
                          onClick={() => {
                            setStep(0);
                            setPlaying(false);
                          }}
                        >
                          <RotateCcw size={15} />
                        </button>
                        <button
                          aria-label="Step back"
                          disabled={step === 0}
                          onClick={() => {
                            setPlaying(false);
                            setStep((s) => Math.max(0, s - 1));
                          }}
                        >
                          <ChevronLeft size={16} />
                        </button>
                        <button onClick={() => setPlaying(!playing)}>
                          {playing ? <Pause size={15} /> : <Play size={15} />}{" "}
                          {playing ? "Pause" : "Play"}
                        </button>
                        <button
                          aria-label="Step forward"
                          disabled={step >= steps.length - 1}
                          onClick={() => {
                            setPlaying(false);
                            setStep((s) => Math.min(steps.length - 1, s + 1));
                          }}
                        >
                          <ChevronRight size={16} />
                        </button>
                      </div>
                      <input
                        aria-label="Trace position"
                        type="range"
                        min="0"
                        max={Math.max(0, steps.length - 1)}
                        value={step}
                        onChange={(e) => {
                          setPlaying(false);
                          setStep(Number(e.target.value));
                        }}
                      />
                      <div className="trace-event">
                        {steps[step]?.action || "No steps"}
                        <span>
                          {step + 1}/{steps.length}
                        </span>
                      </div>
                      <dl className="trace-details">
                        {Object.entries(steps[step] || {})
                          .filter(([k]) => k !== "action")
                          .map(([k, v]) => (
                            <div key={k}>
                              <dt>{k}</dt>
                              <dd>
                                {Array.isArray(v)
                                  ? v.join(" → ")
                                  : typeof v === "object"
                                    ? JSON.stringify(v)
                                    : String(v)}
                              </dd>
                            </div>
                          ))}
                      </dl>
                      {state.metrics.traceTruncated && (
                        <p className="negative">
                          Trace display capped at 5,000 events.
                        </p>
                      )}
                      <div className="trace-history">
                        {steps
                          .slice(Math.max(0, step - 5), step + 1)
                          .map((s, i) => (
                            <div key={i}>
                              <span>{Math.max(0, step - 5) + i + 1}</span>
                              {s.action} <b>{s.current || s.edge || ""}</b>
                            </div>
                          ))}
                      </div>
                    </section>
                  )}
                </div>
              )}
              {tab === "market" && (
                <>
                  <section className="panel opportunities">
                    <div className="section-heading">
                      <div>
                        <h2>
                          Arbitrage opportunities{" "}
                          <span className="badge">
                            {state.opportunities.length} PROFITABLE
                          </span>
                        </h2>
                        <p>
                          Ranked by the C++ priority queue · exact sequential
                          swap validation
                        </p>
                      </div>
                      <label className="checkbox">
                        <input
                          type="checkbox"
                          checked={showRejected}
                          onChange={(e) => setShowRejected(e.target.checked)}
                        />
                        Include rejected candidates
                      </label>
                    </div>
                    <div className="table-scroll">
                      <table>
                        <thead>
                          <tr>
                            {[
                              "ID / route",
                              "Exact pools",
                              "Investment · USDC",
                              "Net · USDC",
                              "ROI",
                              "Hops / time",
                              "Status",
                              "",
                            ].map((h, i) => (
                              <th key={i}>{h}</th>
                            ))}
                          </tr>
                        </thead>
                        <tbody>
                          {rows?.map((o) => (
                            <tr
                              key={o.id}
                              className={o.id === selectedId ? "selected" : ""}
                              onClick={() => {
                                setSelectedId(o.id);
                                setHop(-1);
                              }}
                            >
                              <td>
                                <small>{o.id}</small>
                                <strong>{o.route.join(" → ")}</strong>
                              </td>
                              <td className="pool-cell">
                                {o.pools.join(" / ")}
                              </td>
                              <td>{money(o.input)}</td>
                              <td
                                className={
                                  Number(o.net) > 0 ? "positive" : "negative"
                                }
                              >
                                {money(o.net)}
                              </td>
                              <td>{o.roi.toFixed(3)}%</td>
                              <td>
                                {o.hops.length} /{" "}
                                {state.metrics.milliseconds.toFixed(2)}ms
                              </td>
                              <td>
                                <span className={`status ${o.status}`}>
                                  {o.status}
                                </span>
                              </td>
                              <td>
                                <button
                                  aria-label={`Inspect ${o.id}`}
                                  onClick={() => setSelectedId(o.id)}
                                >
                                  <ChevronRight size={16} />
                                </button>
                              </td>
                            </tr>
                          ))}
                        </tbody>
                      </table>
                      {!rows?.length && (
                        <div className="empty compact">
                          No routes pass the profitability threshold in this
                          snapshot.
                        </div>
                      )}
                    </div>
                  </section>
                  {selected && (
                    <section className="panel curve">
                      <div className="section-heading">
                        <div>
                          <h2>Profit vs. trade size</h2>
                          <p>
                            Measured C++ AMM samples · x: USDC input · y: net
                            USDC
                          </p>
                        </div>
                        <span className="muted">
                          At capital limit:{" "}
                          {selected.atCapital
                            ? `${selected.atCapital.net} USDC`
                            : selected.capitalError}
                        </span>
                      </div>
                      <Chart
                        data={selected.profitCurve.map((p) => ({
                          input: Number(p.input),
                          net: Number(p.net),
                        }))}
                        x="input"
                        y="net"
                      />
                    </section>
                  )}
                </>
              )}
              {tab === "algorithms" && (
                <section className="panel benchmarks">
                  <div className="section-heading">
                    <div>
                      <h2>Complexity & measured performance</h2>
                      <p>
                        Actual C++ measurements · five repetitions per dataset ·
                        trace serialization disabled
                      </p>
                    </div>
                    <button
                      disabled={busy}
                      onClick={() =>
                        void action(async () =>
                          setBench(await api<Benchmark[]>("/benchmarks")),
                        )
                      }
                    >
                      Run benchmarks
                    </button>
                  </div>
                  <div className="complexity">
                    <div>
                      <strong>Adjacency list</strong>
                      <code>O(V + E) space</code>
                      <p>
                        Hash maps index tokens, pools and directed edges.
                        Multiple pools may connect the same pair.
                      </p>
                    </div>
                    <div>
                      <strong>Bounded DFS</strong>
                      <code>O(bᴴ · H), exponential</code>
                      <p>
                        Backtracking enumerates simple cycles, capped at 20,000
                        edge examinations. H ≤ 6.
                      </p>
                    </div>
                    <div>
                      <strong>Bellman–Ford</strong>
                      <code>O(VE) relaxation</code>
                      <p>
                        Predecessor cycles are extracted, then simulated. It
                        does not enumerate every cycle.
                      </p>
                    </div>
                    <div>
                      <strong>Optimization + heap</strong>
                      <code>O(C · S · (V + E + H) + C log C)</code>
                      <p>
                        S bounded trade-size samples per candidate; this
                        implementation copies the graph per evaluation.
                      </p>
                    </div>
                  </div>
                  {bench.length > 0 && (
                    <>
                      <div className="chart">
                        <ResponsiveContainer width="100%" height="100%">
                          <BarChart
                            data={[6, 12, 24, 48].map((n) => ({
                              tokens: n,
                              dfs: bench.find(
                                (b) => b.tokens === n && b.algorithm === "dfs",
                              )?.milliseconds,
                              bellmanFord: bench.find(
                                (b) =>
                                  b.tokens === n &&
                                  b.algorithm === "bellman-ford",
                              )?.milliseconds,
                            }))}
                          >
                            <XAxis dataKey="tokens" />
                            <YAxis />
                            <Tooltip contentStyle={{ background: "#141b24" }} />
                            <Legend />
                            <Bar dataKey="dfs" fill="#58d9a6" />
                            <Bar dataKey="bellmanFord" fill="#8099eb" />
                          </BarChart>
                        </ResponsiveContainer>
                      </div>
                      <div className="table-scroll">
                        <table>
                          <thead>
                            <tr>
                              <th>Algorithm</th>
                              <th>Tokens</th>
                              <th>Pools / edges</th>
                              <th>Cycles</th>
                              <th>Edge examinations</th>
                              <th>Mean ms</th>
                            </tr>
                          </thead>
                          <tbody>
                            {bench.map((b) => (
                              <tr key={`${b.tokens}-${b.algorithm}`}>
                                <td>{b.algorithm}</td>
                                <td>{b.tokens}</td>
                                <td>
                                  {b.pools} / {b.edges}
                                </td>
                                <td>{b.cycles}</td>
                                <td>{b.expansions}</td>
                                <td>{b.milliseconds.toFixed(4)}</td>
                              </tr>
                            ))}
                          </tbody>
                        </table>
                      </div>
                    </>
                  )}
                </section>
              )}
              {tab === "ledger" && (
                <section className="panel ledger">
                  <div className="section-heading">
                    <div>
                      <h2>Simulated execution history</h2>
                      <p>
                        In-memory session ledger · actual reserve mutations · no
                        on-chain transactions
                      </p>
                    </div>
                    <span className="badge">SIMULATION ONLY</span>
                  </div>
                  {state.trades.length > 0 ? (
                    <>
                      <Chart
                        data={state.trades.map((t) => ({
                          trade: t.id,
                          pnl: Number(t.cumulativePnl),
                        }))}
                        x="trade"
                        y="pnl"
                      />
                      <div className="table-scroll">
                        <table>
                          <thead>
                            <tr>
                              {[
                                "Trade / time",
                                "Route",
                                "Scenario",
                                "Starting USDC",
                                "Final USDC",
                                "Net USDC",
                                "Status",
                              ].map((h) => (
                                <th key={h}>{h}</th>
                              ))}
                            </tr>
                          </thead>
                          <tbody>
                            {state.trades.map((t) => (
                              <tr key={t.id}>
                                <td>
                                  {t.id}
                                  <small>
                                    {new Date(t.timestamp).toLocaleString()}
                                  </small>
                                </td>
                                <td>{t.route.join(" → ")}</td>
                                <td>{t.scenario}</td>
                                <td>{t.input}</td>
                                <td>{t.output}</td>
                                <td className="positive">{t.net}</td>
                                <td className="positive">SIMULATED</td>
                              </tr>
                            ))}
                          </tbody>
                        </table>
                      </div>
                    </>
                  ) : (
                    <div className="empty">
                      <Activity size={36} />
                      <h3>Your research ledger is empty</h3>
                      <p>
                        Select an opportunity and execute a simulated trade to
                        record its outcome here.
                      </p>
                      <button onClick={() => setTab("market")}>
                        Explore opportunities
                      </button>
                    </div>
                  )}
                </section>
              )}
            </>
          ) : (
            <div className="panel empty">
              <h2>
                {busy
                  ? "Connecting to the C++ engine…"
                  : "Waiting for the C++ engine"}
              </h2>
              <p>Start the backend on 127.0.0.1:8080, then reconnect.</p>
              <button
                disabled={busy}
                onClick={() =>
                  void action(async () => {
                    setScenarios(await api<Scenario[]>("/scenarios"));
                    await accept(await api<State>("/state"));
                  })
                }
              >
                Reconnect
              </button>
            </div>
          )}
          <footer>
            <span>SolanaScope / Graph-based arbitrage research</span>
            <span>C++20 engine · Integer accounting · Synthetic data</span>
          </footer>
        </main>
      </div>
      {pool && (
        <PoolEditor
          key={pool.id}
          pool={pool}
          busy={busy}
          editable={mode === "demo"}
          onClose={() => setPoolId("")}
          onSave={(p) =>
            void action(async () => {
              await accept(await api<State>("/pools/update", p));
              setPoolId("");
            })
          }
        />
      )}
      {token && (
        <div className="modal-backdrop" onClick={() => setTokenId("")}>
          <section className="panel modal" onClick={(e) => e.stopPropagation()}>
            <div className="section-heading">
              <h2>
                {token.symbol} · {token.name}
              </h2>
              <button
                aria-label="Close token inspector"
                onClick={() => setTokenId("")}
              >
                <X size={16} />
              </button>
            </div>
            <p>Precision: {token.decimals} decimal places</p>
            <p className="muted">
              Synthetic token metadata. No mainnet mint is assigned.
            </p>
            <h3>Outgoing adjacency list</h3>
            {state?.graph.edges
              .filter((e) => e.from === token.id)
              .map((e) => (
                <button
                  className="adjacency"
                  key={e.id}
                  onClick={() => {
                    setTokenId("");
                    setPoolId(e.pool);
                  }}
                >
                  {e.to}
                  <span>{e.id}</span>
                </button>
              ))}
          </section>
        </div>
      )}
    </div>
  );
}
