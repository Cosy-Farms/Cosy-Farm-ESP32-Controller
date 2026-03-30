# RTD Sensor (DS18B20 Water Temp Probe) Development TODO

Branch: blackboxai/rtd-sensor-development

## Steps:

- [x] 1. Update platformio.ini: Add OneWire@^2.3.7 and DallasTemperature@^3.9.0 libs
- [x] 2. Update src/define.h: Add `#define PIN_DS18B20 7`
- [x] 3. Update src/Thermal_Manager.h: Add `extern float water_temp_c; extern bool ds18b20Enabled;`
- [x] 4. Update src/Thermal_Manager.cpp: Include libs, define objects, init, reset, update logic with averaging & failure handling (mirror DHT logic)
- [ ] 5. Update src/main.cpp: Add water temp reporting in systemInfoTask
- [ ] 6. Commit: `git add . &amp;&amp; git commit -m "feat: DS18B20 water temperature sensor support"`
- [ ] 7. Build/Upload: `pio run -t upload`
- [ ] 8. Hardware test & verify Serial output

Progress will be marked here after each step.
