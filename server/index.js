import express from "express";
import path from "path";
import { fileURLToPath } from "url";

const app = express();
app.use(express.json({ limit: "64kb" }));

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// static dashboard
app.use(express.static(path.join(__dirname, "public")));

// ====== In-memory store (per ora) ======
let latest = null;
const history = [];
const MAX_POINTS = 5000;

function toFiniteOrNull(v) {
  if (v === null || v === undefined) return null;
  const n = Number(v);
  return Number.isFinite(n) ? n : null;
}

function pruneHistory() {
  if (history.length > MAX_POINTS) {
    history.splice(0, history.length - MAX_POINTS);
  }
}

app.post("/api/v1/telemetry", (req, res) => {
  const b = req.body || {};
  const point = {
    deviceId: String(b.deviceId || "unknown"),
    ts: Number(b.ts || Math.floor(Date.now() / 1000)), // fallback
    t: toFiniteOrNull(b.t),
    rh: toFiniteOrNull(b.rh),
    dhtOk: Boolean(b.dhtOk ?? true),
    mq7Raw: toFiniteOrNull(b.mq7Raw),
    mq7Ratio: toFiniteOrNull(b.mq7Ratio),
    mq7Ppm: toFiniteOrNull(b.mq7Ppm),
    mq7R0: toFiniteOrNull(b.mq7R0),
    mq7Ok: Boolean(b.mq7Ok ?? false),
    mq7Calibrated: Boolean(b.mq7Calibrated ?? false),
    mq7WarmupDone: Boolean(b.mq7WarmupDone ?? false),
    mq7Level: Number.isFinite(Number(b.mq7Level)) ? Number(b.mq7Level) : 0,
    receivedAt: Date.now()
  };

  latest = point;
  history.push(point);
  pruneHistory();

  res.status(200).json({ ok: true });
});

app.get("/api/v1/latest", (req, res) => {
  res.json(latest ?? { ok: false });
});

// /api/v1/history?minutes=30
app.get("/api/v1/history", (req, res) => {
  const minutes = Math.max(1, Math.min(12 * 60, Number(req.query.minutes || 30)));
  const sinceMs = Date.now() - minutes * 60 * 1000;
  const out = history.filter(p => p.receivedAt >= sinceMs);
  res.json({ minutes, count: out.length, points: out });
});

const PORT = process.env.PORT ? Number(process.env.PORT) : 3001;
app.listen(PORT, "0.0.0.0", () => {
  console.log(`Listening on http://0.0.0.0:${PORT}`);
});
