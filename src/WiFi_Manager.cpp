#include "WiFi_Manager.h"
#include <WiFi.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "NTP_Manager.h"
#include "OTA_Manager.h"

// Globals in main.cpp

void wifiInit()
{
  prefs.begin("wifi-creds", false);
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("pass", "");
  if (ssid == "")
  {
    ssid = "Mestry";
    password = "12345678";
    Serial.printf("No saved WiFi creds. Set default: %s\n", ssid.c_str());
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
  }
  prefs.end();

  Serial.printf("Connecting to '%s'...\n", ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());

  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 40)
  {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf("Connected! IP=%s, Signal=%d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  }
  else
  {
    Serial.println("Connection FAILED - check creds");
  }
}

bool ntpAttempt()
{
  currentState = STATE_NTP_SYNC;
  ntpUpdateOnConnect();
  delay(100); // Let LED blink
  if (g_epochTime > 0 && g_lat.length() > 0)
  {
    currentState = STATE_CONNECTED;
    ntpRetryCount = 0;
    return true;
  }
  return false;
}

void wifiMonitorTask(void *parameter)
{
  Serial.println("WiFi Monitor Task started");
  unsigned long lastCheck = 0;

  for (;;)
  {
    unsigned long now = millis();

    ledBlink(currentState, now);

    if (now - lastCheck > 5000)
    {
      lastCheck = now;

      if (WiFi.status() == WL_CONNECTED)
      {
        if (!wifiConnected)
        {
          wifiConnected = true;
          Serial.printf("WiFi Connected: %s (%d dBm)\n", WiFi.SSID().c_str(), WiFi.RSSI());
          ntpRetryCount = 0;
          if (!ntpAttempt())
          {
            ntpRetryCount++;
            if (ntpRetryCount >= 3)
            {
              Serial.println("NTP failed after 3 tries, skip");
              currentState = STATE_CONNECTED;
            }
          }
          otaCheckAfterNtp();
        }
      }
      else
      {
        wifiConnected = false;
        currentState = STATE_ERROR;
        Serial.println("WiFi disconnected, retrying...");

        WiFi.reconnect();
        delay(5000);

        if (WiFi.status() != WL_CONNECTED)
        {
          Serial.println("Reconnect failed, retrying with saved creds...");
          prefs.begin("wifi-creds", false);
          String ssid = prefs.getString("ssid", DEFAULT_SSID);
          String passw = prefs.getString("pass", DEFAULT_PASS);
          prefs.end();
          WiFi.disconnect(true);
          WiFi.begin(ssid.c_str(), passw.c_str());
          currentState = STATE_CONNECTING;
        }
        else
        {
          currentState = STATE_CONNECTED;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
