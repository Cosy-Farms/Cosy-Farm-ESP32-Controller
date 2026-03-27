#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "define.h"
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

void otaCheckAfterNtp();
void otaUpdateTask(void *parameter);
bool isOtaInProgress();

#endif
