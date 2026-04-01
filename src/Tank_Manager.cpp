#include "Tank_Manager.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <vector>
#include <algorithm>

float g_waterLevelPct = 0.0f;
float g_waterDistanceCm = 0.0f;
bool tankSensorEnabled = true;
float g_tankStdDev = 0.0f; // Track sensor noise/precision
float g_tankHealthPct = 100.0f; // 100% means all pings in burst succeeded

OneWire oneWire(PIN_DS18B20);
DallasTemperature waterSensors(&oneWire);

float water_temp_c = 0.0f;
bool ds18b20Enabled = true;
static int ds18b20ConsecutiveFails = 0;
const int MAX_DS18B20_FAILS = 5;

float water_samples[10];
int water_sample_index = 0;
bool water_samples_filled = false;

// Buffer for Ultrasonic Jitter Analysis
#define TANK_AVG_SAMPLES 10
float tank_samples[TANK_AVG_SAMPLES];
int tank_sample_idx = 0;
bool tank_samples_filled = false;

static int tankConsecutiveFails = 0;
const int MAX_TANK_FAILS = 5;
static unsigned long lastTankRecovery = 0;
const unsigned long TANK_RECOVERY_INTERVAL = 600000; // 10 minutes

// Constants for filtering
#define TANK_MEDIAN_SAMPLES 5
#define EMA_ALPHA 0.3f // Smoothing factor (0.0 to 1.0)
static float filteredDistance = -1.0f;
static unsigned long healthDropStartTime = 0;
static unsigned long lastWipeAlertTime = 0;

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
    g_tankHealthPct = 100.0f;
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
        std::vector<float> validReadings;
        
        // Take a burst of samples for the Median Filter
        for (int i = 0; i < TANK_MEDIAN_SAMPLES; i++) {
            digitalWrite(PIN_TANK_TRIG, LOW);
            delayMicroseconds(2);
            digitalWrite(PIN_TANK_TRIG, HIGH);
            delayMicroseconds(10);
            digitalWrite(PIN_TANK_TRIG, LOW);

            long duration = pulseIn(PIN_TANK_ECHO, HIGH, 30000);
            float d = (duration > 0) ? (duration * 0.0343f) / 2.0f : -1.0f;
            
            if (d > 0 && d < 400.0f) validReadings.push_back(d);
            delay(15); // Short delay to let previous echoes dissipate
        }

        float distance = -1.0f;
        if (!validReadings.empty()) {
            std::sort(validReadings.begin(), validReadings.end());
            distance = validReadings[validReadings.size() / 2]; // Pick the middle value
        }

        // --- Signal Quality (Health) Calculation ---
        // Calculate what % of the burst samples were valid
        float burstHealth = (validReadings.size() * 100.0f) / TANK_MEDIAN_SAMPLES;
        g_tankHealthPct = (0.2f * burstHealth) + (0.8f * g_tankHealthPct); // EMA for stability

        // --- Sensor Wipe Alert Logic ---
        if (g_tankHealthPct < TANK_HEALTH_THRESHOLD) {
            if (healthDropStartTime == 0) {
                healthDropStartTime = now;
            } else if (now - healthDropStartTime >= TANK_HEALTH_WIPE_MS) {
                // Health has been low for more than an hour. Alert every 15 mins.
                if (now - lastWipeAlertTime >= 900000UL || lastWipeAlertTime == 0) {
                    Serial.println("Tank: ALERT - Signal quality has been below 70% for over an hour. Please wipe the sensor face!");
                    lastWipeAlertTime = now;
                }
            }
        } else {
            healthDropStartTime = 0; // Reset timer if health recovers
            lastWipeAlertTime = 0;
        }

        // --- Persistence Check Logic ---
        if (distance <= 0 || distance > 400.0f) {
            // This median-filtered burst failed
            if (tankSensorEnabled) {
                tankConsecutiveFails++;
                Serial.printf("Tank: Persistence Check - Failure %d/%d\n", tankConsecutiveFails, MAX_TANK_FAILS);
                
                if (tankConsecutiveFails >= MAX_TANK_FAILS) {
                    tankSensorEnabled = false;
                    Serial.println("Tank: CRITICAL - 5 consecutive median-filtered failures. Disabling sensor.");
                }
            } else if (tankRecoveryActive) {
                Serial.println("Tank: Recovery attempt failed. Sensor remains disabled.");
            }
        } else {
            // Success: Any valid median reading resets the persistence counter
            if (!tankSensorEnabled) Serial.println("Tank: Sensor Self-Healed!");
            tankSensorEnabled = true; 
            tankConsecutiveFails = 0;

            // --- Slew Rate Limit ---
            // Prevents splashes or ripples from causing sudden jumps in reading.
            float processedDist = distance;
            if (g_waterDistanceCm > 0) {
                float delta = distance - g_waterDistanceCm;
                if (abs(delta) > TANK_MAX_SLEW_CM) {
                    processedDist = g_waterDistanceCm + (delta > 0 ? TANK_MAX_SLEW_CM : -TANK_MAX_SLEW_CM);
                }
            }
            g_waterDistanceCm = processedDist;

            // Rolling average logic
            tank_samples[tank_sample_idx] = processedDist;
            tank_sample_idx = (tank_sample_idx + 1) % TANK_AVG_SAMPLES;
            if (tank_sample_idx == 0) tank_samples_filled = true;

            int count = tank_samples_filled ? TANK_AVG_SAMPLES : tank_sample_idx;
            
            // 1. Calculate Mean (Average)
            float sum = 0;
            for (int i = 0; i < count; i++) sum += tank_samples[i];
            float avgDist = sum / count;

            // 2. Calculate Standard Deviation (Precision Check)
            float sumSqDiff = 0;
            for (int i = 0; i < count; i++) {
                sumSqDiff += sq(tank_samples[i] - avgDist);
            }
            g_tankStdDev = sqrt(sumSqDiff / count);

            // 3. Reliability Diagnostic
            if (count == TANK_AVG_SAMPLES && g_tankStdDev > TANK_SD_THRESHOLD) {
                Serial.printf("Tank: WARNING - High Jitter (SD: %.2f cm). Sensor face may be dirty, wet, or too close to water.\n", g_tankStdDev);
            }

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