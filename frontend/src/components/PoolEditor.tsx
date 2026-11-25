import { useState } from "react";
import { X } from "lucide-react";
import type { Pool } from "../types";
export function PoolEditor({
  pool,
  busy,
  editable,
  onClose,
  onSave,
}: {
  pool: Pool;
  busy: boolean;
  editable: boolean;
  onClose: () => void;
  onSave: (p: Pool) => void;
}) {
  const [draft, setDraft] = useState(pool);
  return (
    <div className="modal-backdrop" onClick={onClose}>
      <section className="modal panel" onClick={(e) => e.stopPropagation()}>
        <div className="section-heading">
          <h2>Pool inspector</h2>
          <button aria-label="Close pool inspector" onClick={onClose}>
            <X size={17} />
          </button>
        </div>
        <h3>{pool.id}</h3>
        <p className="muted">
          {pool.dex} · {pool.type}
        </p>
        <label>
          {pool.a} reserve
          <input
            value={draft.reserveA}
            onChange={(e) => setDraft({ ...draft, reserveA: e.target.value })}
          />
        </label>
        <label>
          {pool.b} reserve
          <input
            value={draft.reserveB}
            onChange={(e) => setDraft({ ...draft, reserveB: e.target.value })}
          />
        </label>
        <label>
          Trading fee · basis points
          <input
            type="number"
            min="0"
            max="9999"
            value={draft.feeBps}
            onChange={(e) =>
              setDraft({ ...draft, feeBps: Number(e.target.value) })
            }
          />
        </label>
        <p className="muted">
          Snapshot: {pool.snapshot}. Changes invalidate previous quotes.
        </p>
        <button
          className="primary"
          disabled={busy || !editable}
          onClick={() => onSave(draft)}
        >
          Apply & recalculate
        </button>
      </section>
    </div>
  );
}
