#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include "define.h"


void ntpInit();
void ntpUpdateOnConnect(); // Called on fresh WiFi connect or day change
void saveGeoCache(long offset);
bool loadGeoCache();

extern String g_lat;
extern String g_lon;
extern time_t g_epochTime;

#endif
