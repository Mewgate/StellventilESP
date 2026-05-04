#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebSocket ws;

void setupWebSocket();
void sendDemoWebsocketData();
void sendWebsocketMessage(const String& message);
