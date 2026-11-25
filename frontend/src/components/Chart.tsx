import {
  ResponsiveContainer,
  LineChart,
  Line,
  XAxis,
  YAxis,
  Tooltip,
  CartesianGrid,
} from "recharts";
export function Chart({
  data,
  x,
  y,
}: {
  data: Record<string, unknown>[];
  x: string;
  y: string;
}) {
  return (
    <div className="chart">
      <ResponsiveContainer width="100%" height="100%">
        <LineChart data={data}>
          <CartesianGrid stroke="#222a35" strokeDasharray="3 3" />
          <XAxis
            dataKey={x}
            type={x === "input" ? "number" : "category"}
            domain={x === "input" ? ["dataMin", "dataMax"] : undefined}
            tick={{ fill: "#8a96a8", fontSize: 10 }}
            minTickGap={40}
          />
          <YAxis tick={{ fill: "#8a96a8", fontSize: 10 }} width={60} />
          <Tooltip
            contentStyle={{
              background: "#141b24",
              border: "1px solid #334155",
            }}
          />
          <Line
            type="monotone"
            dataKey={y}
            stroke="#58d9a6"
            dot={data.length === 1}
            strokeWidth={2}
            isAnimationActive={false}
          />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
