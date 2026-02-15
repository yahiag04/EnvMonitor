# EnvMonitor

ESP32 environmental monitor with DHT11 + MQ-7, real-time telemetry to a Node.js server, OLED output, passive-buzzer alarms, and a modern live web dashboard.
<img width="1454" height="742" alt="Screenshot 2026-02-15 alle 18 42 05" src="https://github.com/user-attachments/assets/3e89a11c-5f93-4212-963e-bebe2429e168" />

## Features

- ESP32 firmware (PlatformIO, Arduino framework)
- DHT11 temperature/humidity monitoring
- MQ-7 CO workflow with:
  - LOW-phase sampling
  - persistent `R0` calibration (`Preferences`)
  - `ratio = Rs/R0` for stable alerting
  - estimated ppm as indicative value
- Ratio-based alarm levels:
  - `OK`: ratio `>= 0.85`
  - `WARN`: `0.70 <= ratio < 0.85`
  - `DANGER`: ratio `< 0.70`
- Warm-up protection (first 10 minutes ignored)
- Passive buzzer alarm patterns (PWM tone)
- OLED summary screen (T, RH, CO estimate, ratio, state)
- Node.js backend with telemetry history
- Live dashboard with:
  - current metrics
  - calibration/warm-up status
  - chart history
  - offline timeout handling (data hidden when stale)

## Tech Stack

- Firmware: C++ / Arduino on ESP32
- Build system: PlatformIO
- Backend: Node.js + Express
- Frontend: HTML/CSS/JS + Chart.js

## Hardware

- Board: ESP32 DevKit (AZ-Delivery)
- Sensors:
  - DHT11
  - MQ-7 (AO read on ADC1)
- Display: SSD1306 OLED (I2C)
- Buzzer: passive buzzer

### Pin Mapping (current config)

- `DHT11` -> `GPIO4`
- `MQ-7 AO` -> `GPIO32` (ADC1)
- `MQ-7 heater control` -> `GPIO25` (reserved for MOSFET-driven heater control)
- `Buzzer (passive)` -> `GPIO26`
- `OLED I2C` -> `SDA GPIO21`, `SCL GPIO22`

> Note: if your MQ-7 module does not expose separate heater control, keep current code as-is and treat ppm as indicative.

## Project Structure

```text
EnvMonitor/
├─ include/
│  ├─ app/app_state.h
│  ├─ config.h
│  ├─ net/telemetry_client.h
│  ├─ net/wifi_manager.h
│  ├─ sensors/dht_sensor.h
│  ├─ sensors/mq7_types.h
│  └─ time/time_sync.h
├─ src/
│  ├─ main.cpp
│  ├─ display/oled_display.cpp
│  ├─ display/oled_display.h
│  ├─ drivers/heater_controller.cpp
│  ├─ drivers/heater_controller.h
│  ├─ net/telemetry_client.cpp
│  ├─ net/wifi_manager.cpp
│  ├─ sensors/dht_sensor.cpp
│  ├─ sensors/mq7_sensor.cpp
│  ├─ sensors/mq7_sensor.h
│  ├─ sensors/mq7_types.cpp
│  ├─ sensors/mq7_types.h
│  ├─ storage/r0_store.cpp
│  ├─ storage/r0_store.h
│  └─ time/time_sync.cpp
├─ server/
│  ├─ index.js
│  ├─ package.json
│  └─ public/
│     ├─ index.html
│     ├─ css/styles.css
│     └─ js/app.js
└─ platformio.ini
```

## Quick Start

### 1) Firmware (ESP32)

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

### 2) Server + Dashboard

```bash
cd server
npm install
npm start
```

Server runs on `http://0.0.0.0:3001`.
Open dashboard from your LAN browser at `http://<YOUR_PC_IP>:3001`.

## Configuration

Main runtime settings are in `include/config.h`:

- Wi-Fi credentials
- telemetry URL (`TELEMETRY_URL`)
- MQ-7 ratio thresholds
- warm-up duration
- buzzer tone/timing parameters
- calibration constraints

## Calibration Workflow (MQ-7)

1. Power on and wait warm-up (10+ minutes).
2. Ensure clean air and LOW sampling phase.
3. Open serial monitor and press `c`.
4. Success condition:
   - 20-sample calibration
   - relative stddev `< 5%`
5. To reset calibration, press `r`.

## Runtime Behavior

- MQ-7 readings are considered valid only after warm-up and valid LOW-phase data.
- Alerts and buzzer are driven by `ratio` (more stable than ppm estimate).
- Dashboard switches to `Offline` and replaces values with `--` if data is stale (>15s).

## API Endpoints

- `POST /api/v1/telemetry`
- `GET /api/v1/latest`
- `GET /api/v1/history?minutes=30`

## Notes

- MQ-7 ppm values are estimates, not certified measurements.
- For decision logic, prefer ratio-based trend + thresholds.
- For production-grade accuracy, derive curve fit from your specific sensor/datasheet and validate with reference gas.

## Roadmap

- Better ppm fitting (`A`, `B`) or LUT/interpolation
- Heater hardware verification with MOSFET and measured LOW equivalent
- Optional persistence for backend history (DB)
