#include "Thermal_Manager.h"
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

DHT dht(PIN_DHT22, DHT22);

OneWire oneWire(PIN_DS18B20);
DallasTemperature waterSensors(&oneWire);

float water_temp_c = 0.0f;
float avg_temp_c = 0.0f;
float avg_humid_pct = 0.0f;
bool dhtEnabled = true; // Tracks sensor health
bool ds18b20Enabled = true; // Tracks water sensor health

static int dhtConsecutiveFails = 0;
static int ds18b20ConsecutiveFails = 0;
const int MAX_DHT_FAILS = 5; // Disable after 10 seconds of failure (5 samples * 2s)
const int MAX_DS18B20_FAILS = 5;

const int AVG_SAMPLES = 5; // 10s at 0.5Hz
float temp_samples[AVG_SAMPLES];
float humid_samples[AVG_SAMPLES];
int dht_sample_index = 0;
bool dht_samples_filled = false;

float water_samples[AVG_SAMPLES];
int water_sample_index = 0;
bool water_samples_filled = false;

void thermalInit() {
  dht.begin();
  Serial.printf("Thermal Manager: DHT22 on GPIO%d initialized\n", PIN_DHT22);

  waterSensors.begin();
  Serial.printf("Thermal Manager: DS18B20 on GPIO%d, devices: %d\n", PIN_DS18B20, waterSensors.getDeviceCount());

  // Init samples to 0
  for (int i = 0; i < AVG_SAMPLES; i++) {
    temp_samples[i] = 0.0f;
    humid_samples[i] = 0.0f;
    water_samples[i] = 0.0f;
  }
  dht_sample_index = 0;
  dht_samples_filled = false;
  water_sample_index = 0;
  water_samples_filled = false;
}

void thermalReset() {
  dhtConsecutiveFails = 0;
  ds18b20ConsecutiveFails = 0;
  dht_samples_filled = false;
  water_samples_filled = false;
  dht_sample_index = 0;
  water_sample_index = 0;
  dhtEnabled = true;
  ds18b20Enabled = true;
  Serial.println("Thermal: Sensor flags reset. Attempting to resume monitoring...");
}

void thermalUpdate() {
  // DHT22 air temp/humidity
  float temp = dht.readTemperature();
  float humid = dht.readHumidity();

  if (isnan(temp) || isnan(humid)) {
    dhtConsecutiveFails++;
    Serial.printf("Thermal: DHT read failed (%d/%d)\n", dhtConsecutiveFails, MAX_DHT_FAILS);
    
    if (dhtConsecutiveFails >= MAX_DHT_FAILS) {
      dhtEnabled = false;
      Serial.println("Thermal: CRITICAL - DHT sensor marked as failed. Disabling.");
    }
  } else {
    // Success: Reset failure counter
    dhtConsecutiveFails = 0;
    dhtEnabled = true;

    temp_samples[dht_sample_index] = temp;
    humid_samples[dht_sample_index] = humid;
    dht_sample_index = (dht_sample_index + 1) % AVG_SAMPLES;
    if (dht_sample_index == 0) dht_samples_filled = true;

    float sum_temp = 0.0f;
    float sum_humid = 0.0f;
    int dht_count = dht_samples_filled ? AVG_SAMPLES : dht_sample_index;

    for (int i = 0; i < dht_count; i++) {
      sum_temp += temp_samples[i];
      sum_humid += humid_samples[i];
    }

    avg_temp_c = sum_temp / dht_count;
    avg_humid_pct = sum_humid / dht_count;
  }

  // DS18B20 water temperature
  if (ds18b20Enabled || ds18b20ConsecutiveFails < MAX_DS18B20_FAILS) {
    waterSensors.requestTemperatures();
    delay(10);
    float wtemp = waterSensors.getTempCByIndex(0);

    if (isnan(wtemp) || wtemp == DEVICE_DISCONNECTED_C) {
      ds18b20ConsecutiveFails++;
      Serial.printf("Thermal: DS18B20 read failed (%d/%d)\n", ds18b20ConsecutiveFails, MAX_DS18B20_FAILS);
      if (ds18b20ConsecutiveFails >= MAX_DS18B20_FAILS) {
        ds18b20Enabled = false;
        Serial.println("Thermal: CRITICAL - DS18B20 sensor marked as failed. Disabling.");
      }
    } else {
      // Success
      ds18b20ConsecutiveFails = 0;
      water_samples[water_sample_index] = wtemp;
      water_sample_index = (water_sample_index + 1) % AVG_SAMPLES;
      if (water_sample_index == 0) water_samples_filled = true;

      float sum_water = 0.0f;
      int wcount = water_samples_filled ? AVG_SAMPLES : water_sample_index;
      for (int i = 0; i < wcount; i++) {
        sum_water += water_samples[i];
      }
      water_temp_c = sum_water / wcount;
    }
  }
}

void thermalTask(void *parameter) {
  Serial.println("Thermal Task: 0.5Hz (2s) sampling started");
  for (;;) {
    if (dhtEnabled || ds18b20Enabled) {
      thermalUpdate();
    } else {
      vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
    yield();
    vTaskDelay(pdMS_TO_TICKS(2000)); // 0.5Hz sensors requirement
  }
}
