import express from "express";
import path from "path";
import { fileURLToPath } from "url";
import https from "https";

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
const TELEGRAM_BOT_TOKEN = process.env.TELEGRAM_BOT_TOKEN || "";
const TELEGRAM_CHAT_ID = process.env.TELEGRAM_CHAT_ID || "";
const TELEGRAM_PERIOD_MS = Number(process.env.TELEGRAM_PERIOD_MS || 10 * 60 * 1000);

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

function val(v, suffix = "") {
  return v === null || v === undefined ? "-" : `${v}${suffix}`;
}

function sendTelegramMessage(text) {
  return new Promise((resolve, reject) => {
    const payload = JSON.stringify({
      chat_id: TELEGRAM_CHAT_ID,
      text
    });

    const req = https.request(
      {
        hostname: "api.telegram.org",
        path: `/bot${TELEGRAM_BOT_TOKEN}/sendMessage`,
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          "Content-Length": Buffer.byteLength(payload)
        }
      },
      (res) => {
        let body = "";
        res.on("data", (chunk) => {
          body += chunk;
        });
        res.on("end", () => {
          if (res.statusCode >= 200 && res.statusCode < 300) {
            resolve();
            return;
          }
          reject(new Error(`Telegram API ${res.statusCode}: ${body}`));
        });
      }
    );

    req.on("error", reject);
    req.write(payload);
    req.end();
  });
}

function buildTelegramMessage(point) {
  return [
    `EnvMonitor (${new Date().toLocaleString("it-IT")})`,
    `Device: ${point.deviceId}`,
    `T: ${val(point.t, " C")}`,
    `RH: ${val(point.rh, " %")}`,
    `MQ7 ratio: ${val(point.mq7Ratio)}`,
    `MQ7 ppm: ${val(point.mq7Ppm)}`,
    `MQ7 level: ${val(point.mq7Level)}`,
    `Warmup: ${point.mq7WarmupDone ? "Y" : "N"}`,
    `Calibrated: ${point.mq7Calibrated ? "Y" : "N"}`
  ].join("\n");
}

function startTelegramNotifier() {
  if (!TELEGRAM_BOT_TOKEN || !TELEGRAM_CHAT_ID) {
    console.log("Telegram disabled: set TELEGRAM_BOT_TOKEN and TELEGRAM_CHAT_ID");
    return;
  }

  console.log(`Telegram notifier enabled (every ${Math.round(TELEGRAM_PERIOD_MS / 1000)}s)`);

  setInterval(async () => {
    if (!latest) return;

    try {
      await sendTelegramMessage(buildTelegramMessage(latest));
      console.log("Telegram notification sent");
    } catch (err) {
      console.error(`Telegram send failed: ${err.message}`);
    }
  }, TELEGRAM_PERIOD_MS);
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
  startTelegramNotifier();
});
