# ESP32 Cosy Farm Controller - Architecture

## Overview
ESP32-S3 based WiFi controller for Cosy Farm with RGB LED status, NTP sync, OTA updates from GitHub Raw (firmware.bin/version.txt). PlatformIO Arduino framework.

## Hardware
- **Board**: ESP32-S3-DevKitC-1 (8MB Flash)
- **RGB LED**: GPIO2 (R PWM ch0), GPIO1 (G ch1), GPIO3 (B ch2) + GND
- **Power**: USB/3.3V

## Software Stack
```
PlatformIO + Arduino-ESP32 core 3.x
├── WiFi (stored creds Preferences)
├── NTP (ip-api.com geo/tz)
├── OTA (GitHub Raw)
├── FreeRTOS tasks
├── ArduinoJson 7
└── Native ledc PWM
```

## Components

### 1. main.cpp
- Globals def (prefs, currentState, wifiConnected, ntpRetryCount)
- setup(): Serial, ledInit(), wifiInit(), xTaskCreate(wifiMonitorTask)
- loop(): Idle

### 2. define.h
- **States**: NTP_SYNC(0), AP(1), CONNECTING(2), CONNECTED(3), ERROR(4), OTA_CHECK(5), OTA_UPDATE(6)
- **Pins/PWM**: R2/G1/B3, 1kHz 8bit
- **OTA**: VERSION_URL, FIRMWARE_URL (GitHub raw), INTERVAL 24h
- Externals: globals

### 3. WiFi_Manager.cpp/h
- wifiInit(): Load/save SSID/pass prefs ("wifi-creds"), connect/reconnect
- wifiMonitorTask(): 5s WiFi check, ledBlink(), ntpAttempt() on connect, otaCheckAfterNtp()

### 4. NTP_Manager.cpp/h
- ntpUpdateOnConnect(): HTTP ip-api.com/json → geo/tz/offset, configTime, cache prefs "geo-cache"
- g_epochTime/g_lat/g_lon/g_timezone/g_localTime

### 5. LED_Manager.cpp/h
- ledInit(): PWM setup
- ledSetColor(r,g,b)
- ledBlink(state, now): Switch patterns (bright/dim blink)

### 6. OTA_Manager.cpp/h
- Globals: localOtaVersion, lastOtaCheck
- otaCheckAfterNtp(): Post-NTP, load "ota" prefs, HTTP version.txt stream read/compare (daily), task if new
- otaUpdateTask(): HTTP firmware.bin chunk 512B to Update.end(true) → restart

## Flow
```
Boot → WiFi connect (blue blink) → NTP sync (magenta fast) → OTA check (cyan slow) → Connected (green)
Error (red fast)
Daily OTA if new version.txt → yellow fast download → restart
```

## OTA Setup
1. Update define.h URLs to repo/raw/version.txt & firmware.bin
2. `pio run` → copy .pio/build/esp32-s3-devkitc-1/firmware.bin
3. Push bin/txt to GitHub main → device checks/pulls

## Build/Upload
```
pio run --target upload --upload-port COMX
monitor: 115200
```

## Partition/Flash
Default 8MB OK for OTA. Flash: min-size for large bin if needed.

Repo: https://github.com/Cosy-Farms/Cosy-Farm-ESP32-Controller

