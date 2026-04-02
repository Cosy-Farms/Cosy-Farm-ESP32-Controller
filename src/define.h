#ifndef DEFINE_H
#define DEFINE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h> // Required for QueueHandle_t
#include <WiFi.h>
#include <Preferences.h>
// No need <String.h>, Arduino.h covers

// Global variables, declared as extern to be defined in a single .cpp file (e.g., NTP_Manager.cpp or main.cpp).
// Geographic latitude.
extern String g_lat;
// Geographic longitude.
extern String g_lon;
// Current epoch time (seconds since Jan 1, 1970).
extern time_t g_epochTime;
// Current UTC time string.
extern String g_utcTime;
// Current local time string.
extern String g_localTime;
// Timezone name (e.g., "Europe/London").
extern String g_timezone;

// Current operational state of the device.
// Replaced by a queue for inter-task communication and a global for current status.
extern QueueHandle_t stateQueue;
extern volatile int g_currentSystemState;

// Define various states for the device's operation.
#define STATE_NTP_SYNC 0
#define STATE_AP 1
#define STATE_CONNECTING 2
#define STATE_CONNECTED 3
#define STATE_ERROR 4
#define STATE_OTA_CHECK 5
#define STATE_OTA_UPDATE 6
// NVS Preferences object.
extern Preferences prefs;
// Flag indicating if WiFi is currently connected.
extern bool wifiConnected;
// Global flag for Safe Mode status
// Counter for NTP synchronization retries.
extern int ntpRetryCount;
// Local firmware version string.
extern String localOtaVersion;
// Timestamp of the last OTA check.

extern time_t lastOtaCheck;
extern String g_deviceId;

// Default WiFi credentials, used if no saved credentials are found.

#define DEFAULT_SSID "COSYFARM"
#define DEFAULT_PASS "12345678"

// The current version of the firmware. This is used to compare against the remote version
// to decide if an update is required.
#define FIRMWARE_VERSION "1.0.0"

#define OTA_VERSION_URL "https://raw.githubusercontent.com/PrathameshMestry/CosyFarm-ESP32/main/version.txt"
#define OTA_FIRMWARE_URL "https://raw.githubusercontent.com/PrathameshMestry/CosyFarm-ESP32/main/firmware.bin"
#define OTA_CHECK_INTERVAL 86400UL // How often to check for updates (in seconds, e.g., 24 hours)

// Pin definitions for the RGB LED.
#define PIN_R 2
#define PIN_G 1
#define PIN_B 3

// PWM (Pulse Width Modulation) settings for the RGB LED.
#define PWM_FREQ 1000
#define PWM_RES 8
#define PWM_CH_R 0 // PWM channel for Red LED
#define PWM_CH_G 1 // PWM channel for Green LED
#define PWM_CH_B 2 // PWM channel for Blue LED

// NTP (Network Time Protocol) settings.
#define NTP_TIMEOUT_MS 3000

#define PIN_DHT22 6
#define PIN_DS18B20 7

extern float avg_temp_c;
extern float avg_humid_pct;
extern float g_thermalStdDev;
#define DHT_SD_THRESHOLD 1.0f    // Jitter threshold for DHT instability

// Tank Management Settings
#define PIN_TANK_TRIG 4
#define PIN_TANK_ECHO 5

// Calibration for a 30cm Tank (Assuming sensor is mounted at the very top lid)
#define TANK_FULL_DIST_CM 5.0f   // Distance when tank is 100% full (Careful of blind zone!)
#define TANK_EMPTY_DIST_CM 30.0f // Distance when tank is 0% full

#define TANK_SD_THRESHOLD 2.5f    // Standard Deviation threshold for jitter detection
#define TANK_MAX_SLEW_CM 10.0f    // Max allowable distance change per 5s sample
#define TANK_HEALTH_THRESHOLD 70.0f // Trigger alert if health is below this
#define TANK_HEALTH_WIPE_MS 3600000UL // Duration required to trigger alert (1 hour)

extern float g_waterLevelPct;
extern float g_waterDistanceCm;
extern bool tankSensorEnabled;
extern float g_tankHealthPct;

// AC Condensate Management
#define PIN_AC_FLOAT 8
#define PIN_AC_PUMP 9
#define AC_FLOAT_DEBOUNCE_MS 3000UL // 3 seconds of stable signal required
#define AC_PUMP_RUN_TIME_MS 90000UL // 90 seconds (1.33 min required for 2L @ 1.5L/min)
extern float g_acWaterPumpedToday;
extern bool g_acPumpRunning;

// CO2 Sensor Pins (MH-Z19E)
#define PIN_MHZ_RX 10
#define PIN_MHZ_TX 11
#define CO2_SD_THRESHOLD 20.0f    // PPM jitter threshold
extern float g_co2StdDev;
extern int g_co2Ppm;

#endif
