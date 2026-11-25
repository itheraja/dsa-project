export async function api<T>(path: string, body?: unknown): Promise<T> {
  const response = await fetch(`/api${path}`, {
    method: body === undefined ? "GET" : "POST",
    headers: body === undefined ? {} : { "Content-Type": "application/json" },
    body: body === undefined ? undefined : JSON.stringify(body),
    signal: AbortSignal.timeout(30000),
  });
  const envelope = await response.json();
  if (!response.ok || !envelope.ok)
    throw new Error(envelope.error?.message || "Backend request failed");
  return envelope.data;
}
