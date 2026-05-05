#pragma once
#include <Arduino.h>

void mqttConnect();
void mqttLoop();
void mqttSend(const char* field, String value);