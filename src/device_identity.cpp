#include "device_identity.h"

#include <WiFi.h>

static String cachedShortId;
static String cachedDeviceName;
static String cachedMdnsName;

String getDeviceShortId() {
  if (cachedShortId.length() == 0) {
    cachedShortId = WiFi.macAddress();
    cachedShortId.replace(":", "");

    if (cachedShortId.length() > 6) {
      cachedShortId = cachedShortId.substring(cachedShortId.length() - 6);
    }

    cachedShortId.toUpperCase();
  }

  return cachedShortId;
}

String getDeviceName() {
  if (cachedDeviceName.length() == 0) {
    cachedDeviceName = "Stellventil-";
    cachedDeviceName += getDeviceShortId();
  }

  return cachedDeviceName;
}

String getDeviceMdnsName() {
  if (cachedMdnsName.length() == 0) {
    cachedMdnsName = getDeviceName();
    cachedMdnsName.toLowerCase();
  }

  return cachedMdnsName;
}
