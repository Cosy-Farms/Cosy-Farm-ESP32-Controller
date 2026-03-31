# ESP32 Cosy Farm Controller - Architecture (Updated to match current codebase incl. CO2 v1.1.0)

## Overview
ESP32-S3 WiFi controller for Cosy Farm IoT system. Manages thermal monitoring (DHT22), water tank (HC-SR04 + DS18B20), AC condensate drainage (float + relay pump), **CO2 (MH-Z19E)**, with RGB LED status, NTP/RTC time sync, OTA from GitHub, serial CLI commands. Uses FreeRTOS tasks, LittleFS logging with RTC-persistent buffer, NVS prefs, auto sensor failure recovery. PlatformIO Arduino framework.

Repo: https://github.com/Cosy-Farms/Cosy-Farm-ESP32-Controller

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
| 10 | MH-Z19 RX | CO2 Sensor UART2 | 9600 baud |
| 11 | MH-Z19 TX | CO2 Sensor UART2 | 9600 baud |

## Software Stack

### Core Components
- **main.cpp**: Serial/10s delay → g_deviceId=eFuse MAC→NVS → LittleFS/RTC log → hw info → ledInit/wifiInit → tasks → sensor inits. 100ms loop(commandUpdate). systemInfoTask (60s reports: uptime/sensors/CO2/WiFi RSSI/IP/heap/PSRAM/state).
- **define.h**: Pins, globals (extern), states (0=NTP_SYNC ... 6=OTA_UPDATE), timings (AC_PUMP_RUN_TIME_MS=90s etc.), FIRMWARE_VERSION=\"1.0.0\".
- **Logging**: LittleFS `/system_log.txt` (128kB trunc), RTC_DATA_ATTR 2kB buffer (persist resets/brownouts, flush overflow/10min/critical/60s reports).
- **Storage**: NVS prefs (\"device\"/\"wifi-creds\"/\"ota\"), LittleFS mounted in setup.
- **ID**: g_deviceId = \"COSYFARM-\" + eFuse MAC.

### FreeRTOS Tasks (all pri=1)

| Manager | Purpose | Update Freq | Stack Size | Globals/Key Logic |
|---------|---------|-------------|------------|-------------------|
| **Thermal_Manager** | DHT22 air T/RH (avg5) | 2s | 16kB | avg_temp_c, avg_humid_pct, dhtEnabled (fail≥5→10min recovery). |
| **Tank_Manager** | HC-SR04 level avg10/DS18B20 T avg10 | 5s | 4kB | g_waterLevelPct (10-150cm→0-100%), g_waterDistanceCm, water_temp_c, tankSensorEnabled/ds18b20Enabled. |
| **CO2_Manager (NEW)** | MH-Z19E ppm avg10 / int T | 5s | 4kB | g_co2Ppm, g_co2Temp, co2Enabled (fail≥5→10min), co2WarmedUp (3min). |
| **ACWater_Manager** | Float→pump FSM | 200ms | 2kB | g_acWaterPumpedToday (daily reset), g_acPumpRunning. States: IDLE/PUMPING/COOLDOWN/FAULT. |
| **WiFi_Manager** | STA (NVS/\"COSYFARM\") monitor+rollback | Async/100ms | 8kB | wifiConnected; AP SSID=g_deviceId. |
| **systemInfoTask** | Aggregate Serial/LittleFS reports | 60s | 4kB | Uptime, sensors/CO2, WiFi, mem/heap/PSRAM, state/OTA prog. |
| **wifiMonitorTask** | Events+NTP/OTA/LED | 100ms | 8kB | Reconnect, ntpAttempt/otaCheckAfterNtp, ledBlink(state). |

**Other Modules** (non-task/init-only):
- **NTP_Manager**: GeoIP/tz JSON sync on connect, g_lat/lon/g_epochTime/g_localTime/g_timezone (rtcUpdate daily).
- **Command_Manager**: Serial CLI (W/w/C/c/S/s/F/f/T/t).
- **LED_Manager**: PWM RGB blink patterns per state (e.g. solid green=CONNECTED).
- **RTC_Manager**: ESP RTC + NTP drift correction, day-change→ntp+acWaterResetDaily.
- **OTA_Manager**: GitHub check post-NTP/24h, rollback if WiFi fail<5min.

### UART Serial Commands (115200 baud, single char trigger in commandUpdate())

| Command | Case | Description | Effect |
|---------|------|-------------|--------|
| **W/w** | Both | Wipe WiFi Credentials | Clear NVS \"wifi-creds\", restart to DEFAULT_SSID/PASS |
| **C/c** | Both | Manual WiFi Config | Prompt SSID → PASS → save NVS → restart |
| **S/s** | Both | Sync WiFi Defaults | Copy define.h DEFAULT_SSID/PASS to NVS → restart |
| **F/f** | Both | Factory Reset | nvs_flash_erase() all → restart |
| **T/t** | Both | Reset Sensors | thermalReset() + tankReset() |

### Key Globals (define.h extern)
- Sensors: avg_temp_c/avg_humid_pct, water_temp_c, g_waterLevelPct/g_waterDistanceCm, g_co2Ppm/g_co2Temp, g_acWaterPumpedToday.
- System: currentState (0=NTP_SYNC..6=OTA_UPDATE), wifiConnected/ntpRetryCount, g_deviceId=\"COSYFARM-MAC\"/g_*time, dhtEnabled/tankSensorEnabled/ds18b20Enabled/co2Enabled/g_acPumpRunning/co2WarmedUp.

## Data Flow
1. setup(): Serial(115200)/10s delay, g_deviceId=eFuse MAC→NVS, LittleFS/RTC log buffer, hw info print, ledInit/wifiInit → tasks(wifiMonitor/systemInfo/co2/acWater/tank/thermal) → sensor inits.
2. Tasks update globals (sensors/logic).
3. systemInfoTask aggregates → Serial + logStatusToFile (RTC-buffered).
4. RTC day-change → NTP resync + daily resets (acWater).
5. Loop: commandUpdate() → CLI actions (e.g., sensor reset).

## Build & OTA
**platformio.ini**:
```
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
lib_deps = 
    bblanchon/ArduinoJson @ ^7.0.0
    adafruit/DHT sensor library @ ^1.4.6
    paulstoffregen/OneWire @ ^2.3.7
    milesburton/DallasTemperature @ ^3.9.0
    https://github.com/WifWaf/MH-Z19.git
build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DCONFIG_ESP_TASK_WDT_INIT=1
    -DCONFIG_ESP_TASK_WDT_TIMEOUT_S=5
```

**OTA URLs**:
- Version: https://raw.githubusercontent.com/Cosy-Farms/Cosy-Farm-ESP32-Controller/main/OTA Files/version.txt
- Firmware: https://raw.githubusercontent.com/Cosy-Farms/Cosy-Farm-ESP32-Controller/main/OTA Files/firmware.bin

## Failure Handling
- **Sensors**: Consecutive fail threshold (5 reads) → disable + 10min auto-recovery/re-init (separate HC-SR04/DS18B20/CO2/DHT).
- **Logging**: RTC buffer survives brownout/reset, auto-flush.
- **WDT**: 5s task timeout.
- **OTA Rollback**: If new FW WiFi fail >5min.

Firmware fully matches this architecture (incl. CO2). Flash & monitor with `pio run -t upload -t monitor`.

