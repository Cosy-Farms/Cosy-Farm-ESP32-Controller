#ifndef DEFINE_H
#define DEFINE_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
// No need <String.h>, Arduino.h covers

// Global variables
extern String g_lat;
extern String g_lon;
extern time_t g_epochTime;
extern String g_utcTime;
extern String g_localTime;
extern String g_timezone;

extern int currentState; // 0=NTP_SYNC, 1=AP, 2=CONNECTING, 3=CONNECTED, 4=ERROR

#define STATE_NTP_SYNC 0
#define STATE_AP 1
#define STATE_CONNECTING 2
#define STATE_CONNECTED 3
#define STATE_ERROR 4
#define STATE_OTA_CHECK 5
#define STATE_OTA_UPDATE 6
extern Preferences prefs;
extern bool wifiConnected;
extern int ntpRetryCount;
extern String localOtaVersion;
extern time_t lastOtaCheck;

// Defaults
#define DEFAULT_SSID "Mestry"
#define DEFAULT_PASS "12345678"

#define OTA_VERSION_URL "https://raw.githubusercontent.com/PrathameshMestry/CosyFarm-ESP32/main/version.txt"
#define OTA_FIRMWARE_URL "https://raw.githubusercontent.com/PrathameshMestry/CosyFarm-ESP32/main/firmware.bin"
#define OTA_CHECK_INTERVAL 86400UL

// Pins
#define PIN_R 2
#define PIN_G 1
#define PIN_B 3

// PWM
#define PWM_FREQ 1000
#define PWM_RES 8
#define PWM_CH_R 0
#define PWM_CH_G 1
#define PWM_CH_B 2

// NTP
#define NTP_TIMEOUT_MS 3000

#endif

