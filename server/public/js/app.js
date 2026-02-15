const tVal = document.getElementById("tVal");
const rhVal = document.getElementById("rhVal");
const coVal = document.getElementById("coVal");
const tBadge = document.getElementById("tBadge");
const rhBadge = document.getElementById("rhBadge");
const coBadge = document.getElementById("coBadge");
const calBadge = document.getElementById("calBadge");
const coMeta = document.getElementById("coMeta");
const coCard = document.getElementById("coCard");
const navStatus = document.getElementById("navStatus");
const lastUpdate = document.getElementById("lastUpdate");

const minutesEl = document.getElementById("minutes");
const reloadBtn = document.getElementById("reload");
const RATIO_WARN_LT = 0.85;
const RATIO_DANGER_LT = 0.70;
const OFFLINE_TIMEOUT_MS = 15000;

function fmtTs(ts) {
  if (!ts) return "-";
  return new Date(ts * 1000).toLocaleString();
}

function setStatus(text, ok) {
  navStatus.textContent = text;
  navStatus.classList.remove("is-online", "is-error");
  if (ok) {
    navStatus.classList.add("is-online");
  } else {
    navStatus.classList.add("is-error");
  }
}

function renderOfflineState() {
  tVal.textContent = "--.-";
  rhVal.textContent = "--";
  coVal.textContent = "--";
  tBadge.textContent = "—";
  rhBadge.textContent = "—";
  coBadge.textContent = "—";
  calBadge.textContent = "R0: --";
  coMeta.textContent = "raw: -- | ratio: --";

  tBadge.className = "pill pill-neutral";
  rhBadge.className = "pill pill-neutral";
  coBadge.className = "pill pill-neutral";
  calBadge.className = "pill pill-neutral";
  coCard.classList.remove("co-warn", "co-danger");
}

async function fetchLatest() {
  try {
    const r = await fetch("/api/v1/latest", { cache: "no-store" });
    const data = await r.json();

    if (!data || data.ok === false) {
      setStatus("Offline", false);
      lastUpdate.textContent = "Ultimo aggiornamento: --";
      renderOfflineState();
      return;
    }

    const ageMs = Date.now() - Number(data.receivedAt || 0);
    if (!Number.isFinite(ageMs) || ageMs > OFFLINE_TIMEOUT_MS) {
      setStatus("Offline", false);
      lastUpdate.textContent = "Ultimo aggiornamento: --";
      renderOfflineState();
      return;
    }

    if (data.dhtOk === false) {
      setStatus("DHT fail", false);
      renderOfflineState();
      return;
    }

    setStatus("Online", true);
    lastUpdate.textContent = "Ultimo aggiornamento: " + fmtTs(data.ts);

    if (Number.isFinite(data.t)) {
      tVal.textContent = data.t.toFixed(1);
      tBadge.textContent = "OK";
      tBadge.className = "pill pill-ok";
    } else {
      tVal.textContent = "--.-";
      tBadge.textContent = "—";
      tBadge.className = "pill pill-neutral";
    }

    if (Number.isFinite(data.rh)) {
      rhVal.textContent = String(Math.round(data.rh));
      rhBadge.textContent = "OK";
      rhBadge.className = "pill pill-ok";
    } else {
      rhVal.textContent = "--";
      rhBadge.textContent = "—";
      rhBadge.className = "pill pill-neutral";
    }

    if (Number.isFinite(data.mq7Ppm)) {
      coVal.textContent = data.mq7Ppm.toFixed(0);
    } else {
      coVal.textContent = "--";
    }

    const rawTxt = Number.isFinite(data.mq7Raw) ? String(Math.round(data.mq7Raw)) : "--";
    const ratioTxt = Number.isFinite(data.mq7Ratio) ? data.mq7Ratio.toFixed(3) : "--";
    coMeta.textContent = `raw: ${rawTxt} | ratio: ${ratioTxt}`;

    if (data.mq7Calibrated) {
      const r0Txt = Number.isFinite(data.mq7R0) ? data.mq7R0.toFixed(0) : "--";
      calBadge.textContent = `R0: ${r0Txt}`;
      calBadge.className = "pill pill-ok";
    } else {
      calBadge.textContent = "R0: non calibrato";
      calBadge.className = "pill pill-warn";
    }

    const ratio = Number.isFinite(data.mq7Ratio) ? data.mq7Ratio : NaN;
    const levelFromServer = Number.isFinite(data.mq7Level) ? data.mq7Level : 0;
    let level = levelFromServer;
    if (level === 0 && Number.isFinite(ratio)) {
      if (ratio < RATIO_DANGER_LT) level = 3;
      else if (ratio < RATIO_WARN_LT) level = 2;
      else level = 1;
    }

    coCard.classList.remove("co-warn", "co-danger");
    if (!data.mq7WarmupDone) {
      coBadge.textContent = "WARMUP";
      coBadge.className = "pill pill-neutral";
    } else if (!data.mq7Calibrated) {
      coBadge.textContent = "CALIBRA";
      coBadge.className = "pill pill-warn";
    } else if (level === 3) {
      coBadge.textContent = "DANGER";
      coBadge.className = "pill pill-danger";
      coCard.classList.add("co-danger");
    } else if (level === 2) {
      coBadge.textContent = "WARN";
      coBadge.className = "pill pill-warn";
      coCard.classList.add("co-warn");
    } else if (level === 1) {
      coBadge.textContent = "OK";
      coBadge.className = "pill pill-ok";
    } else {
      coBadge.textContent = "N/D";
      coBadge.className = "pill pill-neutral";
    }
  } catch {
    setStatus("Offline", false);
    lastUpdate.textContent = "Ultimo aggiornamento: --";
    renderOfflineState();
  }
}

let chart = null;

async function loadHistory() {
  const minutes = Math.max(1, Math.min(720, Number(minutesEl.value || 30)));

  const r = await fetch(`/api/v1/history?minutes=${minutes}`, { cache: "no-store" });
  const data = await r.json();

  const labels = data.points.map(p => new Date(p.receivedAt).toLocaleTimeString());
  const temps = data.points.map(p => p.t);
  const hums  = data.points.map(p => p.rh);
  const coPpm = data.points.map(p => p.mq7Ppm);

  const ctx = document.getElementById("chart");

  if (chart) chart.destroy();

  chart = new Chart(ctx, {
    type: "line",
    data: {
      labels,
      datasets: [
        {
          label: "Temp (°C)",
          data: temps,
          tension: 0.25,
          borderColor: "#ff7f50",
          backgroundColor: "rgba(255,127,80,0.18)",
          pointRadius: 0,
          borderWidth: 2.2
        },
        {
          label: "Umidità (%)",
          data: hums,
          tension: 0.25,
          borderColor: "#2f9bd9",
          backgroundColor: "rgba(47,155,217,0.16)",
          pointRadius: 0,
          borderWidth: 2.2
        },
        {
          label: "CO stimato (ppm)",
          data: coPpm,
          tension: 0.25,
          borderColor: "#14a06f",
          backgroundColor: "rgba(20,160,111,0.15)",
          pointRadius: 0,
          borderWidth: 2.4
        }
      ]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      animation: false,
      scales: {
        x: {
          ticks: { maxTicksLimit: 8, color: "#5d6a6d", font: { family: "IBM Plex Mono" } },
          grid: { color: "rgba(21,32,33,0.08)" }
        },
        y: {
          ticks: { color: "#5d6a6d", font: { family: "IBM Plex Mono" } },
          grid: { color: "rgba(21,32,33,0.08)" }
        }
      },
      plugins: {
        legend: {
          display: true,
          labels: {
            color: "#223033",
            boxWidth: 14,
            boxHeight: 14,
            borderRadius: 4,
            font: { family: "Sora", weight: "600" }
          }
        }
      }
    }
  });
}

reloadBtn.addEventListener("click", loadHistory);

// init
loadHistory();
fetchLatest();
setInterval(fetchLatest, 1000);
