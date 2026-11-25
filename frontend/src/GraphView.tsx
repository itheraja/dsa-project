import { useMemo } from "react";
import {
  ReactFlow,
  Background,
  Controls,
  MarkerType,
  BaseEdge,
  EdgeLabelRenderer,
  Handle,
  Position,
  type EdgeProps,
  type NodeProps,
} from "@xyflow/react";
import "@xyflow/react/dist/style.css";
import type { Graph, Opportunity, Trace } from "./types";
function TokenNode({ data }: NodeProps) {
  return (
    <div className={`token-node ${data.active ? "active" : ""}`}>
      <Handle type="target" position={Position.Left} />
      <span className="token-emblem">{String(data.symbol).slice(0, 1)}</span>
      <strong>{String(data.symbol)}</strong>
      <small>{String(data.name)}</small>
      <Handle type="source" position={Position.Right} />
    </div>
  );
}
function PoolEdge(props: EdgeProps) {
  const {
    sourceX: x1,
    sourceY: y1,
    targetX: x2,
    targetY: y2,
    data,
    style,
    markerEnd,
    id,
  } = props;
  const dx = x2 - x1,
    dy = y2 - y1,
    len = Math.hypot(dx, dy) || 1;
  const offset = Number(data?.offset || 0);
  const cx = (x1 + x2) / 2 - (dy / len) * offset,
    cy = (y1 + y2) / 2 + (dx / len) * offset;
  const path = `M ${x1},${y1} Q ${cx},${cy} ${x2},${y2}`;
  return (
    <>
      <BaseEdge
        id={id}
        path={path}
        style={style}
        markerEnd={markerEnd}
        interactionWidth={18}
      />
      <EdgeLabelRenderer>
        <div
          className="pool-edge-label"
          style={{
            transform: `translate(-50%,-50%) translate(${(x1 + 2 * cx + x2) / 4}px,${(y1 + 2 * cy + y2) / 4}px)`,
            opacity: style?.opacity,
          }}
        >
          {String(data?.label || "")}
        </div>
      </EdgeLabelRenderer>
    </>
  );
}
const nodeTypes = { token: TokenNode },
  edgeTypes = { pool: PoolEdge };
export function GraphView({
  graph,
  selected,
  hop,
  trace,
  onPool,
  onToken,
}: {
  graph: Graph;
  selected?: Opportunity;
  hop: number;
  trace?: Trace;
  onPool: (id: string) => void;
  onToken: (id: string) => void;
}) {
  const nodes = useMemo(
    () =>
      graph.tokens.map((t, i) => ({
        id: t.id,
        type: "token",
        width: 115,
        height: 84,
        handles: [
          {
            type: "source" as const,
            position: Position.Right,
            x: 113,
            y: 40,
            width: 4,
            height: 4,
          },
          {
            type: "target" as const,
            position: Position.Left,
            x: -2,
            y: 40,
            width: 4,
            height: 4,
          },
        ],
        position: {
          x:
            370 +
            300 * Math.cos((i * 2 * Math.PI) / graph.tokens.length - Math.PI),
          y:
            220 +
            190 * Math.sin((i * 2 * Math.PI) / graph.tokens.length - Math.PI),
        },
        data: {
          symbol: t.symbol,
          name: t.name,
          active: trace?.current === t.id || selected?.route.includes(t.id),
        },
      })),
    [graph.tokens, selected, trace],
  );
  const edges = useMemo(
    () =>
      graph.edges.map((e) => {
        const siblings = graph.edges.filter(
          (x) =>
            [x.from, x.to].sort().join(":") === [e.from, e.to].sort().join(":"),
        );
        const ordinal = siblings.findIndex((x) => x.id === e.id);
        const canonical = e.from < e.to ? 1 : -1;
        const active = trace
          ? trace.edge === e.id || trace.path?.includes(e.id)
          : selected?.edges.includes(e.id);
        const current = trace?.edge === e.id || selected?.edges[hop] === e.id;
        const pool = graph.pools.find((p) => p.id === e.pool)!;
        return {
          id: e.id,
          source: e.from,
          target: e.to,
          type: "pool",
          animated: !!current,
          markerEnd: {
            type: MarkerType.ArrowClosed,
            color: active ? "#58d9a6" : "#667184",
            width: 16,
            height: 16,
          },
          data: {
            offset: canonical * (ordinal - (siblings.length - 1) / 2) * 65,
            label: `${pool.id.split("-")[0]} · ${pool.feeBps} bps`,
          },
          style: {
            stroke: current
              ? "#a6f5d2"
              : active
                ? selected?.status === "rejected"
                  ? "#e98989"
                  : "#58d9a6"
                : "#536074",
            strokeWidth: current ? 3 : active ? 2 : 1.3,
            opacity: (selected || trace) && !active ? 0.19 : 1,
          },
        };
      }),
    [graph, selected, hop, trace],
  );
  return (
    <ReactFlow
      nodes={nodes}
      edges={edges}
      nodeTypes={nodeTypes}
      edgeTypes={edgeTypes}
      fitView
      fitViewOptions={{ padding: 0.18 }}
      minZoom={0.3}
      maxZoom={2}
      nodesDraggable={false}
      nodesConnectable={false}
      colorMode="dark"
      onEdgeClick={(_, e) =>
        onPool(graph.edges.find((x) => x.id === e.id)!.pool)
      }
      onNodeClick={(_, n) => onToken(n.id)}
    >
      <Background color="#26303b" gap={24} size={1} />
      <Controls showInteractive={false} />
    </ReactFlow>
  );
}
