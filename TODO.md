# DHT22 Thermal Monitoring Plan

## Plan for adding DHT22 on GPIO22

**Information Gathered:**
- No existing DHT/temp code (search no matches)
- platformio.ini no DHT lib, add adafruit/DHT sensor library ^1.4.6
- main.cpp systemInfoTask for logging avg temp/humid
- Pin 22 free

**Plan:**
- [x] Update platformio.ini: add lib_deps adafruit/DHT sensor library @ ^1.4.6
- [x] src/define.h: +#define PIN_DHT22 22; extern float avg_temp_c, avg_humid_pct;
- [x] Create src/Thermal_Manager.h: declares thermalInit(), thermalUpdate(), externs
- [x] Create src/Thermal_Manager.cpp: DHT22 dht(PIN_DHT22, DHT22); rolling avg 10Hz (100ms task), read fail safe
- src/main.cpp: thermalInit(); xTaskCreate thermalTask; systemInfoTask + temp/humid lines

**Dependent Files:**
- platformio.ini, src/define.h, src/main.cpp, src/Thermal_Manager.[ch]pp

**Followup steps:**
- pio lib install/update, pio run to build/test
- Flash, verify serial log avg temp

Approve plan / revise?

