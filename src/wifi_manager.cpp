#include "wifi_manager.h"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>

#include "config.h"

static Preferences preferences;

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

  if (ssid.isEmpty()) {
    Serial.println("Keine gespeicherten WLAN-Daten gefunden.");
    return false;
  }

  Serial.print("Verbinde mit gespeichertem WLAN: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
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
  return false;
}

void startConfigAP() {
  wifiConnected = false;

  WiFi.disconnect(true, true);
  delay(300);

  WiFi.mode(WIFI_AP);
  delay(500);

  bool ok = WiFi.softAP(AP_SSID, AP_PASS, 6, 0, 4);

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

  if (MDNS.begin(MDNS_NAME)) {
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
    Serial.print("Dashboard erreichbar unter: http://");
    Serial.print(MDNS_NAME);
    Serial.println(".local");
  } else {
    Serial.println("mDNS failed");
  }
}
