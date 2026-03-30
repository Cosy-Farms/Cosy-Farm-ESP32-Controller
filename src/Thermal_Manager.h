#ifndef THERMAL_MANAGER_H
#define THERMAL_MANAGER_H

#include "define.h"

void thermalInit();
void thermalUpdate();
void thermalReset();
void thermalTask(void *parameter);

extern float avg_temp_c;
extern float avg_humid_pct;
extern bool dhtEnabled;

extern float water_temp_c;
extern bool ds18b20Enabled;

#endif
