#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "define.h"
#include "LED_Manager.h"


extern int currentState;

void wifiInit();
void wifiMonitorTask(void *parameter);

#endif

