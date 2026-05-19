#pragma once

#include <Arduino.h>

extern const char INDEX_HTML[] PROGMEM;
extern const char CONFIG_HTML[] PROGMEM;
extern const char SAVED_HTML[] PROGMEM;
extern const char WIFI_CONNECT_WAIT_HTML[] PROGMEM;
extern const char WIFI_CONNECT_FAILED_HTML[] PROGMEM;

String buildSavedHtml(const String& dashboardUrl, unsigned long redirectDelayMs);
String buildWifiConnectWaitHtml(const String& ssid);
String buildWifiConnectFailedHtml(const String& ssid);
