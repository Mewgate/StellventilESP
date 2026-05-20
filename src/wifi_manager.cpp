#include "wifi_manager.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>

#include "config.h"
#include "device_identity.h"

static Preferences preferences;

static const char* wifiStatusText(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "UNKNOWN";
  }
}

String loadSSID() {
  preferences.begin("wifi", true);
  String ssid = preferences.getString("ssid", "");
  preferences.end();
  return ssid;
}

String loadPassword() {
  preferences.begin("wifi", true);
  String pass = preferences.getString("pass", "");
  preferences.end();
  return pass;
}

void saveCredentials(const String& ssid, const String& pass) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
}

void clearCredentials() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
}

bool connectToSavedWifi() {
  String ssid = loadSSID();
  String pass = loadPassword();
  ssid.trim();
  pass.trim();

  if (ssid.isEmpty()) {
    Serial.println("Keine gespeicherten WLAN-Daten gefunden.");
    return false;
  }

  Serial.print("Verbinde mit gespeichertem WLAN: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(true, true);
  delay(300);

  Serial.println("Suche nach WLAN-Netzen...");
  int networkCount = WiFi.scanNetworks();
  bool foundSsid = false;

  if (networkCount <= 0) {
    Serial.println("Keine WLAN-Netze gefunden.");
  } else {
    Serial.print("Gefundene WLAN-Netze: ");
    Serial.println(networkCount);

    for (int i = 0; i < networkCount; i++) {
      if (WiFi.SSID(i) == ssid) {
        foundSsid = true;
        Serial.print("Ziel-WLAN gefunden. RSSI: ");
        Serial.print(WiFi.RSSI(i));
        Serial.print(" dBm, Kanal: ");
        Serial.println(WiFi.channel(i));
        break;
      }
    }
  }

  WiFi.scanDelete();

  if (!foundSsid) {
    Serial.println("Ziel-WLAN wurde nicht gefunden. ESP32 kann nur 2.4 GHz WLAN nutzen.");
  }

  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WLAN verbunden.");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("Verbindung fehlgeschlagen.");
  Serial.print("WiFi status: ");
  Serial.println(wifiStatusText(WiFi.status()));
  return false;
}

void startConfigAP() {
  wifiConnected = false;

  WiFi.disconnect(true, true);
  delay(300);

  WiFi.mode(WIFI_AP);
  WiFi.softAPsetHostname("esp32setup");
  WiFi.setSleep(false);
  delay(500);

  String setupSsid = String(AP_SSID) + "-" + getDeviceShortId();
  bool ok = WiFi.softAP(setupSsid.c_str(), AP_PASS, 6, 0, 4);

  if (ok) {
    Serial.println("Setup-AP gestartet.");
    Serial.print("AP SSID: ");
    Serial.println(WiFi.softAPSSID());
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Fehler beim Starten des AP.");
  }
}

void setupMdns() {
  if (!wifiConnected) return;

  String mdnsName = getDeviceMdnsName();

  if (MDNS.begin(mdnsName.c_str())) {
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
    Serial.print("Dashboard erreichbar unter: http://");
    Serial.print(mdnsName);
    Serial.println(".local");
  } else {
    Serial.println("mDNS failed");
  }
}
