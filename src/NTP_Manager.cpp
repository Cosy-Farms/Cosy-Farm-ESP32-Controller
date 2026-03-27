#include "NTP_Manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <ArduinoJson.h>

// These definitions allocate memory for the globals. 
// If you prefer these to stay in main.cpp, remove them from here and 
// ensure they are defined (not just declared) in main.cpp without 'static'.
String g_lat;
String g_lon;
String g_timezone;
String g_utcTime;
String g_localTime;
time_t g_epochTime;

void saveGeoCache(long offset) {
  if (prefs.begin("geo-cache", false)) {
    prefs.putString("lat", g_lat);
    prefs.putString("lon", g_lon);
    prefs.putString("tz_name", g_timezone);
    prefs.putLong("offset", offset);
    prefs.end();
    Serial.println("Geo-coordinates saved to Preferences.");
  } else {
    Serial.println("Failed to open Preferences for saving.");
  }
}

bool loadGeoCache() {
  if (prefs.begin("geo-cache", true)) {
    g_lat = prefs.getString("lat", "");
    g_lon = prefs.getString("lon", "");
    g_timezone = prefs.getString("tz_name", "");
    long offset = prefs.getLong("offset", 0);
    prefs.end();

    if (g_lat.length() > 0 && g_timezone.length() > 0) {
      // Apply the loaded offset to the system immediately
      char tzbuf[64];
      int hours = offset / 3600;
      int minutes = abs((int)(offset % 3600) / 60);
      snprintf(tzbuf, sizeof(tzbuf), "UTC%+d:%02d", -hours, minutes);
      
      configTime(offset, 0, "pool.ntp.org");
      Serial.printf("Loaded Geo Cache: %s, %s (Offset: %ld)\n", g_lat.c_str(), g_timezone.c_str(), offset);
      return true;
    }
  }
  Serial.println("No valid Geo Cache found.");
  return false;
}

void ntpInit() {
  Serial.println("NTP init");
  // Try to load cached data so we have a timezone 
  // even before the first successful API sync.
  if (loadGeoCache()) {
    Serial.println("Initial time sync configured from cache.");
  }
}

void ntpUpdateOnConnect() {
  Serial.println("NTP/Geo update on connect");
  
  // Fetch Geo + TZ
  HTTPClient http;
  http.begin("http://ip-api.com/json/?fields=status,lat,lon,timezone,offset");
  http.addHeader("User-Agent", "ESP32");
  int code = http.GET();
  
  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.printf("Geo payload: %s\n", payload.c_str());
    http.end();
    
    // Allocate the JSON document
    // For this small response, 512 bytes is plenty.
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // Check if the API returned a success status
    if (doc["status"] != "success") {
      Serial.println("API Status not success");
      return;
    }
    
    // Extract values safely
    // ArduinoJson handles the conversion from numbers to Strings for g_lat/g_lon
    g_lat = doc["lat"].as<String>();
    g_lon = doc["lon"].as<String>();
    g_timezone = doc["timezone"].as<String>();
    long offset = doc["offset"];
    
    // Set TZ with offset (seconds). 
    // Note: POSIX TZ strings use negative values for East (e.g., UTC-5:30 for GMT+5:30).
    char tzbuf[64];
    int hours = offset / 3600;
    int minutes = abs((int)(offset % 3600) / 60);
    // Formatting as UTC-H:M or UTC+H:M
    snprintf(tzbuf, sizeof(tzbuf), "UTC%+d:%02d", -hours, minutes);
    
    Serial.printf("TZ=%s (POSIX: %s) offset=%ld\n", g_timezone.c_str(), tzbuf, offset);
    
    // NTP sync - Apply the offset directly from the API.
    // This sets the system's local time offset so Local and UTC time will differ.
    configTime(offset, 0, "pool.ntp.org");
    
    // Save for offline use
    saveGeoCache(offset);
    
    // Wait for a valid NTP sync (up to 5 seconds)
    int retry = 0;
    while (time(nullptr) < 1000000000L && retry < 10) {
      delay(500);
      retry++;
    }
    
    g_epochTime = time(nullptr);
    
    struct tm timeinfo;
    // UTC
    gmtime_r(&g_epochTime, &timeinfo);
    char ubuf[64];
    strftime(ubuf, sizeof(ubuf), "%Y-%m-%d %H:%M:%S UTC", &timeinfo);
    g_utcTime = ubuf;
    
    // Local
    localtime_r(&g_epochTime, &timeinfo);
    char lbuf[64];
    strftime(lbuf, sizeof(lbuf), "%Y-%m-%d %H:%M:%S Local", &timeinfo);
    g_localTime = lbuf;
    
    Serial.printf("Success! LAT=%s LON=%s UTC=%s Local=%s TZ=\"%s\"\n", g_lat.c_str(), g_lon.c_str(), g_utcTime.c_str(), g_localTime.c_str(), g_timezone.c_str());
  } else {
    Serial.printf("API fail %d\n", code);
    http.end();
  }
}
