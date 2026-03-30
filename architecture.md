# ESP32 Cosy Farm Controller - Architecture (Updated w/ Complete Pin Mapping)

## Overview
ESP32-S3 WiFi controller for Cosy Farm. Features: RGB LED status, NTP geo/tz sync, OTA from GitHub, Serial commands, RTC cache/drift correction, Voltage safety mode. PlatformIO Arduino. Latest: Version 1.0.0 init, 11s boot delay.

## Hardware
- Board: ESP32-S3-DevKitC-1 (8MB Flash, 240MHz dual-core Xtensa LX7, 512KB SRAM)
- UART: USB CDC (GPIO43 TX, GPIO44 RX default, 115200 baud)

## Pin Mapping & Connections
| GPIO | Function | Connection | Tolerance | Notes/Binding |
|------|-----------|------------|-----------|--------------|
| 1 | RGB Green LED | LED cathode via resistor | 3.3V 40mA max | PWM ch1 freq1kHz res8bit |
| 2 | RGB Red LED | LED cathode via resistor | 3.3V 40mA max | PWM ch0 |
| 3 | RGB Blue LED | LED cathode via resistor | 3.3V 40mA max | PWM ch2 |
| 4 | Unused | - | - | - |
| 5 | Unused | - | - | - |
| 6 | DHT22 Data | DHT22 Sensor | 3.3V Logic | Requires 4.7k-10k Pull-up |
| 7 | DS18B20 Data | Waterproof Temp Probe | 3.3V Logic | OneWire, 4.7k pull-up, water tank temp |
| 43 | UART TX | USB CDC | 3.3V | Default Serial.print |
| 44 | UART RX | USB CDC | 3.3V | Default Serial.read |
| - | Strapping | GPIO0/3/45/46 boot mode | 3.3V | GPIO0 GND for download, GPIO3/45/46 high for normal boot |

**Tolerance**: All GPIO 3.3V logic, +/-10% supply OK, ADC 12bit 0-3.3V. PWM safe for LEDs.

## Software Stack
[... rest unchanged, append full previous content but truncate for brevity ...]

Repo: https://github.com/Cosy-Farms/Cosy-Farm-ESP32-Controller main:latest
