#include "CO2_Manager.h"
#include <HardwareSerial.h>

int g_co2Ppm = 0;
int g_co2Temp = 0;
bool co2Enabled = true;
bool co2WarmedUp = false;

static MHZ19 mhz;
static HardwareSerial mhzSerial(2); // Use UART2

static int co2ConsecutiveFails = 0;
const int MAX_CO2_FAILS = 5;
static unsigned long lastCo2Recovery = 0;
const unsigned long CO2_RECOVERY_INTERVAL = 600000; // 10 minutes

const int CO2_AVG_SAMPLES = 10;
int co2_samples[CO2_AVG_SAMPLES];
int co2_sample_idx = 0;
bool co2_samples_filled = false;

void co2Init() {
    // MH-Z19E typically operates at 9600 baud
    mhzSerial.begin(9600, SERIAL_8N1, PIN_MHZ_RX, PIN_MHZ_TX);
    mhz.begin(mhzSerial);
    
    // Enable auto-calibration (ABC logic) - usually recommended for 24/7 apps
    mhz.autoCalibration(true);
    
    // Set range to 5000ppm for MH-Z19E model
    mhz.setRange(5000);

    lastCo2Recovery = millis();
    Serial.println("CO2 Manager: MH-Z19E Initialized on UART2");
}

void co2Reset() {
    co2ConsecutiveFails = 0;
    co2Enabled = true;
    co2_samples_filled = false;
    co2_sample_idx = 0;
    Serial.println("CO2: Sensor flags reset.");
}

void co2Update() {
    unsigned long now = millis();
    bool isRecoveryTick = (now - lastCo2Recovery >= CO2_RECOVERY_INTERVAL);
    bool co2RecoveryActive = isRecoveryTick && !co2Enabled;

    if (isRecoveryTick) {
        lastCo2Recovery = now;
    }

    if (co2Enabled || co2RecoveryActive) {
        int currentCo2 = mhz.getCO2();
        
        // MH-Z19 returns 0 or a value outside expected range on failure
        // Note: 400 is the baseline for outdoor air.
        if (currentCo2 < 300 || currentCo2 > 5000) {
            if (co2Enabled) {
                co2ConsecutiveFails++;
                Serial.printf("CO2: Sensor read failed (%d/%d)\n", co2ConsecutiveFails, MAX_CO2_FAILS);
                if (co2ConsecutiveFails >= MAX_CO2_FAILS) {
                    co2Enabled = false;
                    Serial.println("CO2: CRITICAL - Sensor disabled for recovery period.");
                }
            }
        } else {
            // Success
            if (!co2Enabled) Serial.println("CO2: Sensor Self-Healed!");
            co2Enabled = true;
            co2ConsecutiveFails = 0;

            // Check warmup status (Library provides this based on time since boot)
            // Usually ~3 minutes for stability
            if (!co2WarmedUp && millis() > 180000) {
                co2WarmedUp = true;
                Serial.println("CO2: Sensor warm-up complete.");
            }
            
            // Get internal sensor temperature
            g_co2Temp = mhz.getTemperature();

            // Rolling average logic
            co2_samples[co2_sample_idx] = currentCo2;
            co2_sample_idx = (co2_sample_idx + 1) % CO2_AVG_SAMPLES;
            if (co2_sample_idx == 0) co2_samples_filled = true;

            long sum = 0;
            int count = co2_samples_filled ? CO2_AVG_SAMPLES : co2_sample_idx;
            for (int i = 0; i < count; i++) sum += co2_samples[i];
            
            g_co2Ppm = (int)(sum / count);
        }
    }
}

void co2Task(void *parameter) {
    Serial.println("CO2 Task: Monitoring CO2 levels every 5s");
    // Initial delay to allow UART stability
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    for (;;) {
        co2Update();
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}