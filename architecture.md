# ESP32 Cosy Farm Controller - Architecture (Updated w/ Tank_Manager & DS18B20)

## Overview
ESP32-S3 WiFi controller for Cosy Farm. Features: RGB LED status, NTP geo/tz sync, OTA GitHub, Serial commands, RTC cache, safety mode. PlatformIO Arduino. v1.0.0+.

## Hardware
- Board: ESP32-S3-DevKitC-1 (8MB Flash, 240MHz dual-core, 512KB SRAM)
- UART: USB CDC (GPIO43 TX/RX, 115200 baud)

## Pin Mapping
| GPIO | Function | Connection | Notes |
|------|----------|------------|-------|
| 1 | RGB G | LED | PWM ch1 |
| 2 | RGB R | LED | PWM ch0 |
| 3 | RGB B | LED | PWM ch2 |
| 6 | DHT22 | Air temp/humid | 4.7k pull-up |
| 7 | DS18B20 | Water temp probe | OneWire, 4.7k pull-up |
| 4 | HC-SR04 Trig | Tank ultrasonic level | PIN_TANK_TRIG |
| 5 | HC-SR04 Echo | Tank ultrasonic level | PIN_TANK_ECHO |
| 43/44 | UART | USB CDC | Serial |

## Software Stack
**Managers (FreeRTOS tasks):**
- **Thermal_Manager**: DHT22 air temp/humid (GPIO6), avg5 2s, fail-disable
- **Tank_Manager** (new): HC-SR04 ultrasonic water level (Trig/Echo pins), DS18B20 water temp (GPIO7), avg10 5s, recovery logic, g_waterLevelPct, g_waterDistanceCm, water_temp_c
- WiFi_Manager: AP/STA, NVS creds
- NTP_Manager: Geo/tz sync
- OTA_Manager: GitHub firmware check/update
- LED_Manager: RGB PWM status
- RTC_Manager: Local time cache
- Command_Manager: Serial CLI

**Core:**
- main.cpp: Init tasks, systemInfoTask (60s report: uptime, WiFi, sensors)
- define.h: Pins (PIN_DHT22=6, PIN_DS18B20=7, PIN_TANK_TRIG/ECHO), globals (avg_temp_c, water_temp_c, g_waterLevelPct)
- LittleFS logs, NVS prefs

**Libs:** ArduinoJson, DHT, OneWire, DallasTemperature, Preferences, LittleFS

Repo: https://github.com/Cosy-Farms/Cosy-Farm-ESP32-Controller main:9d8fa8e (Tank_Manager + docs)
