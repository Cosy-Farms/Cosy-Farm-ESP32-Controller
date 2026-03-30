#include "Thermal_Manager.h"
#include <DHT.h>

DHT dht(PIN_DHT22, DHT22);

float avg_temp_c = 0.0f;
float avg_humid_pct = 0.0f;
bool dhtEnabled = true;     // Tracks sensor health

static int dhtConsecutiveFails = 0;
const int MAX_DHT_FAILS = 5; // Disable after 10 seconds of failure (5 samples * 2s)

static unsigned long lastRecoveryAttempt = 0;
const unsigned long RECOVERY_INTERVAL_MS = 600000; // 10 minutes (600,000 ms)

const int AVG_SAMPLES = 5; // 10s at 0.5Hz
float temp_samples[AVG_SAMPLES];
float humid_samples[AVG_SAMPLES];
int dht_sample_index = 0;
bool dht_samples_filled = false;

void thermalInit()
{
  dht.begin();
  Serial.printf("Thermal Manager: DHT22 on GPIO%d initialized\n", PIN_DHT22);

  lastRecoveryAttempt = millis();
  // Init samples to 0
  for (int i = 0; i < AVG_SAMPLES; i++)
  {
    temp_samples[i] = 0.0f;
    humid_samples[i] = 0.0f;
  }
  dht_sample_index = 0;
  dht_samples_filled = false;
}

void thermalReset()
{
  dhtConsecutiveFails = 0;
  dht_samples_filled = false;
  dht_sample_index = 0;
  dhtEnabled = true;
  Serial.println("Thermal: Sensor flags reset. Attempting to resume monitoring...");
}

void thermalUpdate()
{
  unsigned long now = millis();
  // Check if it's time to try re-initializing failed sensors
  bool isRecoveryTime = (now - lastRecoveryAttempt >= RECOVERY_INTERVAL_MS);
  bool dhtRecoveryActive = isRecoveryTime && !dhtEnabled;

  if (isRecoveryTime) {
    lastRecoveryAttempt = now;
    if (dhtRecoveryActive) {
      Serial.println("Thermal: 10-minute recovery attempt for DHT22...");
      dht.begin();
    }
  }

  // DHT22 air temp/humidity
  if (dhtEnabled || dhtRecoveryActive) 
  {
    float temp = dht.readTemperature();
    float humid = dht.readHumidity();

    if (isnan(temp) || isnan(humid))
    {
      if (dhtEnabled) {
        dhtConsecutiveFails++;
        Serial.printf("Thermal: DHT read failed (%d/%d)\n", dhtConsecutiveFails, MAX_DHT_FAILS);

        if (dhtConsecutiveFails >= MAX_DHT_FAILS)
        {
          dhtEnabled = false;
          Serial.println("Thermal: CRITICAL - DHT sensor marked as failed. Disabling for 10 minutes.");
        }
      } else if (dhtRecoveryActive) {
        Serial.println("Thermal: DHT22 recovery read failed. Sensor still unresponsive.");
      }
    }
    else
    {
      // Success: Reset failure counter and re-enable if it was disabled
      if (!dhtEnabled) Serial.println("Thermal: DHT22 Self-Healed!");
      dhtConsecutiveFails = 0;
      dhtEnabled = true;

      temp_samples[dht_sample_index] = temp;
      humid_samples[dht_sample_index] = humid;
      dht_sample_index = (dht_sample_index + 1) % AVG_SAMPLES;
      if (dht_sample_index == 0)
        dht_samples_filled = true;

      float sum_temp = 0.0f;
      float sum_humid = 0.0f;
      int dht_count = dht_samples_filled ? AVG_SAMPLES : dht_sample_index;

      for (int i = 0; i < dht_count; i++)
      {
        sum_temp += temp_samples[i];
        sum_humid += humid_samples[i];
      }

      avg_temp_c = sum_temp / dht_count;
      avg_humid_pct = sum_humid / dht_count;
    }
  }
}

void thermalTask(void *parameter)
{
  Serial.println("Thermal Task: 0.5Hz (2s) sampling started");
  for (;;)
  {
    // Always call thermalUpdate so the internal 10-minute timer can be checked
    thermalUpdate();
    
    yield();
    vTaskDelay(pdMS_TO_TICKS(2000)); 
  }
}
