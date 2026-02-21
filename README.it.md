# EnvMonitor (Italiano)

Monitor ambientale basato su ESP32 con DHT11 + MQ-7, telemetria in tempo reale verso server Node.js, output OLED, allarmi buzzer e logging CSV locale su scheda SD (opzionale).

## Funzionalita

- Firmware ESP32 (PlatformIO, framework Arduino)
- Misura temperatura/umidita con DHT11
- Workflow MQ-7 con:
  - campionamento ADC periodico (`MQ7_PERIOD_MS`)
  - calibrazione `R0` persistente (`Preferences`)
  - `ratio = Rs/R0` per allarmi piu stabili
  - ppm stimati come valore indicativo
- Livelli allarme basati su ratio:
  - `OK`: ratio `>= 0.85`
  - `WARN`: `0.70 <= ratio < 0.85`
  - `DANGER`: ratio `< 0.70`
- Warm-up MQ-7 (prime letture ignorate per 10 minuti)
- Buzzer passivo con pattern diversi per WARN/DANGER
- Display OLED riassuntivo
- Logger SD (`src/source/sd_logger.*`) con header CSV automatico
- Backend Node.js + dashboard web live

## Stack Tecnologico

- Firmware: C++ / Arduino su ESP32
- Build: PlatformIO
- Backend: Node.js + Express
- Frontend: HTML/CSS/JS + Chart.js

## Hardware

- ESP32 DevKit
- DHT11
- MQ-7 (uscita AO su ADC1)
- OLED SSD1306 (I2C)
- Buzzer passivo
- Modulo microSD SPI (opzionale)

## Mappatura Pin (config attuale)

- `DHT11` -> `GPIO4`
- `MQ-7 AO` -> `GPIO32` (ADC1)
- `Buzzer` -> `GPIO26`
- `OLED I2C` -> `SDA GPIO21`, `SCL GPIO22`
- `SD CS` -> `GPIO27`
- `SD SCK` -> `GPIO18`
- `SD MISO` -> `GPIO19`
- `SD MOSI` -> `GPIO23`

Nota: nel firmware attuale non viene usato controllo heater/MOSFET del MQ-7.

## Avvio Rapido

### 1) Firmware ESP32

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

Server su `http://0.0.0.0:3001`.
Apri la dashboard in LAN su `http://<IP_PC>:3001`.

## Configurazione

Le impostazioni principali sono in `include/config.h`:

- credenziali Wi-Fi
- URL telemetria (`TELEMETRY_URL`)
- soglie MQ-7
- durata warm-up
- parametri buzzer
- vincoli calibrazione
- pin/percorso/periodo SD (`PIN_SD_*`, `SD_FILE_PATH`, `SD_PERIOD_MS`)

## Calibrazione MQ-7

1. Accendi e attendi il warm-up (almeno 10 minuti).
2. Tieni il sensore in aria pulita.
3. Apri il monitor seriale e premi `c`.
4. Condizione di successo:
   - 20 campioni
   - deviazione standard relativa `< 5%`
5. Per reset calibrazione premi `r`.

## Comportamento Runtime

- Le letture MQ-7 sono valide solo dopo warm-up.
- Gli allarmi usano `ratio` (piu stabile dei ppm).
- La telemetria viene inviata ogni `SEND_PERIOD_MS`.
- Il logger SD scrive righe CSV tramite `SdLogger` (nel flusso attuale, con cadenza telemetria).
- La dashboard passa a `Offline` se i dati sono stantii (>15s).

## API

- `POST /api/v1/telemetry`
- `GET /api/v1/latest`
- `GET /api/v1/history?minutes=30`

## Note

- I ppm del MQ-7 sono stime non certificate.
- Per la logica decisionale usa preferibilmente trend e soglie su `ratio`.
- Per maggiore accuratezza va fatta una taratura su sensore/referenza reale.
