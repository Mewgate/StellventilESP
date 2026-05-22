#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

void storeChartHistoryValue(int value);
bool storeChartHistoryValueFromMessage(const String& message, String* chartMessage);
void sendChartHistory(AsyncWebSocketClient *client);
