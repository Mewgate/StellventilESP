#pragma once

#include <Arduino.h>

#define RANGE 4

extern const char* AP_SSID;
extern const char* AP_PASS;
extern const char* MDNS_NAME;

extern bool wifiConnected;
extern bool restartPending;
extern unsigned long restartAt;
