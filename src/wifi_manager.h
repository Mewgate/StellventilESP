#pragma once

#include <Arduino.h>

String loadSSID();
String loadPassword();

void saveCredentials(const String& ssid, const String& pass);
void clearCredentials();

bool connectToSavedWifi();
void startConfigAP();
void setupMdns();
