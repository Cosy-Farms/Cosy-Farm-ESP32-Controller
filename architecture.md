# ESP32 Cosy Farm Controller - Architecture (Updated to match current codebase v1.0.0)

## Overview
ESP32-S3 WiFi controller for Cosy Farm IoT system. Manages thermal monitoring (DHT22), water tank (HC-SR04 + DS18B20), AC condensate drainage (float + relay pump), with RGB LED status, NTP/RTC time sync, OTA from GitHub, serial CLI commands. Uses FreeRTOS tasks, LittleFS logging with RTC-persistent buffer, NVS prefs, auto sensor failure recovery. PlatformIO Arduino framework.

Repo: https://github.com/PrathameshMestry/CosyFarm-ESP32-Controller

## Hardware
- **Board**: ESP32-S3-DevKitC-1 (8MB Flash, 512KB SRAM, dual-core 240MHz)
- **UART**: USB CDC (115200 baud)

### Pin Mapping & Configs
| GPIO | Function | Sensor/Device | Notes/Calibration |
|------|----------|---------------|-------------------|
| 1 | RGB G | WS2812/LED | PWM ch1, 1kHz/8bit |
| 2 | RGB R | WS2812/LED | PWM ch0 |
| 3 | RGB B | WS2812/LED | PWM ch2 |
| 6 | DHT22 Data | Air T/RH | 4.7kΩ pull-up, 2s samples, avg5 (10s) |
| 7 | DS18B20 DQ | Water T (tank) | OneWire, 4.7kΩ pull-up, avg10 |
| 4 | HC-SR04 Trig | Tank level ultrasonic | 5s samples, avg10 |
| 5 | HC-SR04 Echo | Tank level ultrasonic | Full=10cm, Empty=150cm → 0-100% |
| 8 | Float Switch | AC Condensate Level | Pull-up, LOW=full, 3s debounce |
| 9 | Relay | AC Water Pump | Active HIGH, 90s cycles (~2L @1.5L/min), 10s cooldown |

## Software Stack

### Core Components
- **main.cpp**: Sequential init (LED/WiFi → tasks → sensors), 100ms loop (commandUpdate). systemInfoTask (60s reports: uptime, sensors, WiFi RSSI/IP, heap/PSRAM, state).
- **define.h**: Pins, globals (extern), states (0=NTP_SYNC ... 6=OTA_UPDATE), timings (AC_PUMP_RUN_TIME_MS=90s, etc.), FIRMWARE_VERSION="1.0.0".
- **Logging**: LittleFS `/system_log.txt` (128kB trunc), RTC_DATA_ATTR 2kB buffer (persist resets, flush overflow/10min/critical changes).
- **Storage**: NVS prefs ("device"), LittleFS mounted in setup.
- **ID**: g_deviceId = "COSYFARM-" + eFuse MAC.



### FreeRTOS Tasks (all pri=1)

## UART Serial Commands (115200 baud, single char trigger in commandUpdate())

| Command | Case | Description | Effect |
|---------|------|-------------|--------|
| **W/w** | Both | Wipe WiFi Credentials | Clear NVS "wifi-creds", restart to DEFAULT_SSID/PASS from define.h |
| **C/c** | Both | Manual WiFi Config | Prompt SSID → PASS → save NVS → restart |
| **S/s** | Both | Sync WiFi Defaults | Copy define.h DEFAULT_SSID/PASS to NVS → restart |
| **F/f** | Both | Factory Reset | nvs_flash_erase() all partitions → init → restart |
| **T/t** | Both | Reset Sensors | thermalReset() + tankReset() (re-init enabled) |

| Manager | Purpose | Update Freq | Stack Size | Globals/Key Logic |
|---------|---------|-------------|------------|-------------------|
| **Thermal_Manager** | DHT22 air T/RH | 2s | 16kB | avg_temp_c, avg_humid_pct, dhtEnabled. Fail≥5→disable 10min recovery. |
| **Tank_Manager** | HC-SR04 level %, DS18B20 water T | 5s | 4kB | g_waterLevelPct, g_waterDistanceCm, water_temp_c, tankSensorEnabled/ds18b20Enabled. Separate fail/recovery. |
| **ACWater_Manager** | Float→pump cycles | 200ms | 2kB | g_acWaterPumpedToday (daily 2L increments), g_acPumpRunning. States: IDLE/PUMPING/COOLDOWN/FAULT. |
| **WiFi_Manager** | AP/STA/NVS creds | Monitor | 8kB | wifiConnected, currentState. Defaults: SSID="COSYFARM"/"12345678". |
| **systemInfoTask** | 60s Serial/LittleFS reports | 60s | 4kB | Uptime d/h/m, sensors, mem, OTA prog. |
| **wifiMonitorTask** | Connection watch | Async | 8kB | Updates wifiConnected/state. |

**Other Modules** (non-task/init-only):
- **NTP_Manager**: GeoIP/tz JSON sync, g_lat/lon/epoch/localTime/timezone.
- **OTA_Manager**: GitHub check 24h (version.txt/firmware.bin), isOtaInProgress().
- **LED_Manager**: RGB PWM status (incl. define.h PWM_FREQ=1kHz etc.).
- **RTC_Manager**: ESP RTC + NTP, day-change drift correction → ntpUpdate + acWaterResetDaily.
- **Command_Manager**: Serial CLI (wifi reset, thermal/tank status).

### Key Globals (define.h extern)
- Sensors: avg_temp_c/pct, water_temp_c, g_waterLevelPct/DistanceCm, g_acWaterPumpedToday.
- System: currentState, wifiConnected, ntpRetryCount, g_deviceId, g_* time vars.
- Flags: dhtEnabled/tankSensorEnabled/ds18b20Enabled/g_acPumpRunning.

## Data Flow
1. setup(): Hardware init → create tasks → sensor inits.
2. Tasks update globals (sensors/logic).
3. systemInfoTask aggregates → Serial + logStatusToFile (RTC-buffered).
4. RTC day-change → NTP resync + daily resets.
5. Loop: commandUpdate() → CLI actions (e.g., sensor reset).

## Build & OTA
**platformio.ini**:
```
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
lib_deps = 
    bblanchon/ArduinoJson@^7.0.0
    adafruit/DHT sensor library@^1.4.6
    paulstoffregen/OneWire@^2.3.7
    milesburton/DallasTemperature@^3.9.0
build_flags = -DCORE_DEBUG_LEVEL=3 -DARDUINO_USB_CDC_ON_BOOT=1 -DCONFIG_ESP_TASK_WDT_INIT=1 -DCONFIG_ESP_TASK_WDT_TIMEOUT_S=5
```

**OTA URLs**:
- Version: https://raw.githubusercontent.com/PrathameshMestry/CosyFarm-ESP32/main/version.txt
- Firmware: https://raw.githubusercontent.com/PrathameshMestry/CosyFarm-ESP32/main/firmware.bin

## Failure Handling
- **Sensors**: Consecutive fail threshold (5 reads) → disable + 10min auto-recovery/re-init.
- **Logging**: RTC buffer survives brownout/reset, auto-flush.
- **WDT**: 5s task timeout.

Firmware fully matches this architecture. Flash & monitor with `pio run -t upload -t monitor`.

