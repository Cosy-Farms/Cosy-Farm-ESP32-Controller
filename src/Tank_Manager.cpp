#include "Tank_Manager.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

float g_waterLevelPct = 0.0f;
float g_waterDistanceCm = 0.0f;
bool tankSensorEnabled = true;

OneWire oneWire(PIN_DS18B20);
DallasTemperature waterSensors(&oneWire);

float water_temp_c = 0.0f;
bool ds18b20Enabled = true;
static int ds18b20ConsecutiveFails = 0;
const int MAX_DS18B20_FAILS = 5;

float water_samples[10];
int water_sample_index = 0;
bool water_samples_filled = false;

static int tankConsecutiveFails = 0;
const int MAX_TANK_FAILS = 5;
static unsigned long lastTankRecovery = 0;
const unsigned long TANK_RECOVERY_INTERVAL = 600000; // 10 minutes

const int TANK_AVG_SAMPLES = 10;
float tank_samples[TANK_AVG_SAMPLES];
int tank_sample_idx = 0;
bool tank_samples_filled = false;

void tankInit() {
    pinMode(PIN_TANK_TRIG, OUTPUT);
    pinMode(PIN_TANK_ECHO, INPUT);
    digitalWrite(PIN_TANK_TRIG, LOW);

    waterSensors.begin();
    Serial.printf("Tank Manager: DS18B20 on GPIO%d, devices: %d\n", PIN_DS18B20, waterSensors.getDeviceCount());
    
    lastTankRecovery = millis();

    for(int i=0; i<10; i++) water_samples[i] = 0.0f;
    water_sample_index = 0;
    water_samples_filled = false;

    Serial.println("Tank Manager: HC-SR04 Initialized");
}

void tankReset() {
    tankConsecutiveFails = 0;
    ds18b20ConsecutiveFails = 0;
    tankSensorEnabled = true;
    ds18b20Enabled = true;
    water_samples_filled = false;
    water_sample_index = 0;
    tank_samples_filled = false;
    tank_sample_idx = 0;
    Serial.println("Tank: Sensor flags reset.");
}

void tankUpdate() {
    unsigned long now = millis();
    bool isRecoveryTick = (now - lastTankRecovery >= TANK_RECOVERY_INTERVAL);

    // Define specific recovery flags for each sensor
    bool tankRecoveryActive = isRecoveryTick && !tankSensorEnabled;
    bool dsRecoveryActive = isRecoveryTick && !ds18b20Enabled;

    if (isRecoveryTick) {
        lastTankRecovery = now; // Reset global timer every 10 minutes
    }

    if (tankRecoveryActive) {
        Serial.println("Tank: 10-minute recovery attempt for HC-SR04...");
    }
    if (dsRecoveryActive) {
        Serial.println("Tank: 10-minute recovery attempt for DS18B20...");
        waterSensors.begin();
        Serial.printf("Tank: DS18B20 recovery found %d devices\n", waterSensors.getDeviceCount());
    }

    // --- HC-SR04 Ultrasonic Update ---
    if (tankSensorEnabled || tankRecoveryActive) {
        // Trigger pulse
        digitalWrite(PIN_TANK_TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(PIN_TANK_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(PIN_TANK_TRIG, LOW);

        // Measure echo (timeout at ~30ms as sound travels ~10m in that time)
        long duration = pulseIn(PIN_TANK_ECHO, HIGH, 30000);
        
        // Calculate distance: (duration / 2) * speed of sound (0.0343 cm/us)
        float distance = (duration > 0) ? (duration * 0.0343f) / 2.0f : -1.0f;

        if (distance <= 0 || distance > 400.0f) {
            if (tankSensorEnabled) {
                tankConsecutiveFails++;
                Serial.printf("Tank: Sensor read failed (%d/%d)\n", tankConsecutiveFails, MAX_TANK_FAILS);
                if (tankConsecutiveFails >= MAX_TANK_FAILS) {
                    tankSensorEnabled = false;
                    Serial.println("Tank: CRITICAL - Sensor disabled for 10 minutes.");
                }
            } else if (tankRecoveryActive) {
                Serial.println("Tank: Recovery failed. Sensor still unresponsive.");
            }
        } else {
            // Success
            if (!tankSensorEnabled) Serial.println("Tank: Sensor Self-Healed!");
            tankSensorEnabled = true;
            tankConsecutiveFails = 0;
            g_waterDistanceCm = distance;

            // Rolling average logic
            tank_samples[tank_sample_idx] = distance;
            tank_sample_idx = (tank_sample_idx + 1) % TANK_AVG_SAMPLES;
            if (tank_sample_idx == 0) tank_samples_filled = true;

            float sum = 0;
            int count = tank_samples_filled ? TANK_AVG_SAMPLES : tank_sample_idx;
            for (int i = 0; i < count; i++) sum += tank_samples[i];
            float avgDist = sum / count;

            // Convert distance to Percentage
            // 0% = EmptyDist, 100% = FullDist
            float pct = (TANK_EMPTY_DIST_CM - avgDist) / (TANK_EMPTY_DIST_CM - TANK_FULL_DIST_CM) * 100.0f;
            g_waterLevelPct = constrain(pct, 0.0f, 100.0f);
        }
    }

    // --- DS18B20 Water Temperature Update ---
    if (ds18b20Enabled || dsRecoveryActive)
    {
        waterSensors.requestTemperatures();
        delay(10);
        float wtemp = waterSensors.getTempCByIndex(0);

        if (isnan(wtemp) || wtemp == DEVICE_DISCONNECTED_C)
        {
            if (ds18b20Enabled) {
                ds18b20ConsecutiveFails++;
                Serial.printf("Tank: DS18B20 read failed (%d/%d)\n", ds18b20ConsecutiveFails, MAX_DS18B20_FAILS);
                if (ds18b20ConsecutiveFails >= MAX_DS18B20_FAILS)
                {
                    ds18b20Enabled = false;
                    Serial.println("Tank: CRITICAL - DS18B20 marked as failed.");
                }
            } else if (dsRecoveryActive) {
                Serial.println("Tank: DS18B20 recovery failed.");
            }
        }
        else
        {
            if (!ds18b20Enabled) Serial.println("Tank: DS18B20 Self-Healed!");
            ds18b20ConsecutiveFails = 0;
            ds18b20Enabled = true;

            water_samples[water_sample_index] = wtemp;
            water_sample_index = (water_sample_index + 1) % 10;
            if (water_sample_index == 0)
                water_samples_filled = true;

            float sum_water = 0.0f;
            int wcount = water_samples_filled ? 10 : water_sample_index;
            for (int i = 0; i < wcount; i++)
            {
                sum_water += water_samples[i];
            }
            water_temp_c = sum_water / wcount;
        }
    }
}

void tankTask(void *parameter) {
    Serial.println("Tank Task: Monitoring water level every 5s");
    for (;;) {
        tankUpdate();
        vTaskDelay(pdMS_TO_TICKS(5000)); // Sample every 5 seconds
    }
}