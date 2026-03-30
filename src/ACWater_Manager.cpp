#include "ACWater_Manager.h"
#include <Arduino.h>

bool g_acPumpRunning = false;
float g_acWaterPumpedToday = 0.0f;
static unsigned long pumpStartTime = 0;
static unsigned long floatTriggerStartTime = 0;
static bool faultDetected = false;

enum ACState
{
    IDLE,
    PUMPING,
    COOLDOWN,
    FAULT
};
static ACState currentACState = IDLE;

void acWaterInit()
{
    pinMode(PIN_AC_FLOAT, INPUT_PULLUP);
    pinMode(PIN_AC_PUMP, OUTPUT);
    digitalWrite(PIN_AC_PUMP, LOW);
    Serial.println("ACWater Manager: Initialized (GPIO8 Float Active-Low, GPIO9 Pump)");
}

void acWaterUpdate()
{
    unsigned long now = millis();
    bool floatTriggered = (digitalRead(PIN_AC_FLOAT) == LOW);

    switch (currentACState)
    {
    case IDLE:
        if (floatTriggered)
        {
            if (floatTriggerStartTime == 0)
            {
                floatTriggerStartTime = now; // Mark the first time we saw the trigger
            }
            else if (now - floatTriggerStartTime >= AC_FLOAT_DEBOUNCE_MS)
            {
                // Signal has been stable for the required duration
                Serial.println("ACWater: Tank Full (Stable). Starting pump...");
                digitalWrite(PIN_AC_PUMP, HIGH);
                pumpStartTime = now;
                g_acPumpRunning = true;
                currentACState = PUMPING;
                floatTriggerStartTime = 0;
            }
        }
        else
        {
            floatTriggerStartTime = 0; // Reset timer if signal bounces back to HIGH
        }
        break;

    case PUMPING:
        // Non-blocking timer check
        if (now - pumpStartTime >= AC_PUMP_RUN_TIME_MS)
        {
            digitalWrite(PIN_AC_PUMP, LOW);
            g_acPumpRunning = false;
            Serial.println("ACWater: Pumping cycle complete.");

            // Safety check: if float is still high, the pump failed or tank is blocked
            if (digitalRead(PIN_AC_FLOAT) == LOW)
            {
                currentACState = FAULT;
                Serial.println("ACWater: CRITICAL FAULT - Tank still full after pump cycle!");
            }
            else
            {
                currentACState = COOLDOWN;
                g_acWaterPumpedToday += 2.0f; // Each successful cycle clears the 2L tank
                pumpStartTime = now; // Reuse variable for cooldown timer
            }
        }
        break;

    case COOLDOWN:
        // 10 second delay to prevent rapid cycling from sensor noise
        if (now - pumpStartTime >= 10000)
        {
            currentACState = IDLE;
        }
        break;

    case FAULT:
        // Stay in fault unless float clears or system is reset
        if (!floatTriggered)
        {
            Serial.println("ACWater: Fault cleared manually.");
            currentACState = IDLE;
        }
        break;
    }
}

void acWaterResetDaily()
{
    g_acWaterPumpedToday = 0.0f;
    Serial.println("ACWater: Daily totalizer reset.");
}

void acWaterTask(void *parameter)
{
    acWaterInit();
    for (;;)
    {
        acWaterUpdate();
        // Higher frequency check (200ms) to ensure responsive float detection
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}