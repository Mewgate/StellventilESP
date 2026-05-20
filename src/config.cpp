#include "config.h"

const char* AP_SSID = "Stellventil-Setup";
const char* AP_PASS = "12345678";

bool wifiConnected = false;
bool restartPending = false;
unsigned long restartAt = 0;
