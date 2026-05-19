#include "web_server.h"

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

#include "config.h"
#include "html_pages.h"
#include "wifi_manager.h"
#include "websocket_manager.h"

static AsyncWebServer server(80);

static const unsigned long SAVE_RESTART_DELAY_MS = 3000;
static const unsigned long SAVE_REDIRECT_DELAY_MS = 15000;
static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

enum WifiConnectState {
  WIFI_CONNECT_IDLE,
  WIFI_CONNECT_PENDING,
  WIFI_CONNECT_RUNNING,
  WIFI_CONNECT_CONNECTED,
  WIFI_CONNECT_FAILED
};

static WifiConnectState wifiConnectState = WIFI_CONNECT_IDLE;
static String pendingSsid;
static String pendingPass;
static String pendingDashboardUrl;
static unsigned long wifiConnectStartedAt = 0;

static String wifiConnectStateText() {
  switch (wifiConnectState) {
    case WIFI_CONNECT_PENDING:
    case WIFI_CONNECT_RUNNING:
      return "connecting";
    case WIFI_CONNECT_CONNECTED:
      return "connected";
    case WIFI_CONNECT_FAILED:
      return "failed";
    case WIFI_CONNECT_IDLE:
    default:
      return "idle";
  }
}

void setupWebServer() {

  setupWebSocket();
  server.addHandler(&ws);

  // ====================================
  // STARTSEITE
  // ====================================

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {

    if (wifiConnected) {
      request->send(200, "text/html; charset=utf-8", INDEX_HTML);
    } else {
      request->send(200, "text/html; charset=utf-8", CONFIG_HTML);
    }
  });

  // ====================================
  // WLAN SPEICHERN
  // ====================================

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {

    if (!request->hasParam("ssid", true)) {
      request->send(400, "text/plain", "SSID fehlt");
      return;
    }

    String ssid = request->getParam("ssid", true)->value();
    String pass = "";

    if (request->hasParam("password", true)) {
      pass = request->getParam("password", true)->value();
    }

    pendingSsid = ssid;
    pendingPass = pass;
    pendingDashboardUrl = "";
    wifiConnectState = WIFI_CONNECT_PENDING;

    Serial.println("Neue WLAN-Daten erhalten.");
    Serial.println("Verbindung wird im Hintergrund geprueft...");

    String waitHtml = buildWifiConnectWaitHtml(ssid);
    request->send(200, "text/html; charset=utf-8", waitHtml);
  });

  server.on("/wifi-status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = String("{\"state\":\"") + wifiConnectStateText() + "\"";
    if (wifiConnectState == WIFI_CONNECT_CONNECTED && !pendingDashboardUrl.isEmpty()) {
      json += ",\"url\":\"/wifi-saved\"";
    }
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/wifi-saved", HTTP_GET, [](AsyncWebServerRequest *request) {
    String savedHtml = buildSavedHtml(pendingDashboardUrl, SAVE_REDIRECT_DELAY_MS);
    request->send(200, "text/html; charset=utf-8", savedHtml);
  });

  server.on("/wifi-failed", HTTP_GET, [](AsyncWebServerRequest *request) {
    String failedHtml = buildWifiConnectFailedHtml(pendingSsid);
    request->send(200, "text/html; charset=utf-8", failedHtml);
  });

  // ====================================
  // RESET WLAN
  // ====================================

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {

    clearCredentials();

    request->send(
      200,
      "text/plain",
      "Gespeicherte WLAN-Daten geloescht. Neustart..."
    );

    restartPending = true;
    restartAt = millis() + 1000;
  });

  server.begin();
  Serial.println("HTTP/Websocket server startet.");
}

void handleWifiConnectAttempt() {

  if (wifiConnectState == WIFI_CONNECT_PENDING) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    WiFi.begin(pendingSsid.c_str(), pendingPass.c_str());

    wifiConnectStartedAt = millis();
    wifiConnectState = WIFI_CONNECT_RUNNING;

    Serial.print("Pruefe WLAN-Verbindung zu: ");
    Serial.println(pendingSsid);
  }

  if (wifiConnectState != WIFI_CONNECT_RUNNING) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    pendingDashboardUrl = String("http://") + WiFi.localIP().toString();
    wifiConnected = true;

    saveCredentials(pendingSsid, pendingPass);

    Serial.println("WLAN-Daten erfolgreich geprueft und gespeichert.");
    Serial.print("Router hat IP vergeben: ");
    Serial.println(WiFi.localIP());
    Serial.println("Neustart wird vorbereitet...");

    wifiConnectState = WIFI_CONNECT_CONNECTED;
    restartPending = true;
    restartAt = millis() + SAVE_RESTART_DELAY_MS;
    return;
  }

  if (millis() - wifiConnectStartedAt >= WIFI_CONNECT_TIMEOUT_MS) {
    clearCredentials();
    wifiConnected = false;
    wifiConnectState = WIFI_CONNECT_FAILED;

    Serial.println("Verbindung fehlgeschlagen.");
    Serial.println("Neue WLAN-Daten wurden nicht gespeichert.");
    Serial.println("Setup-AP wird wiederhergestellt.");

    startConfigAP();
  }
}

void handleRestart() {

  if (restartPending && millis() >= restartAt) {
    Serial.println("ESP startet jetzt neu...");
    delay(200);
    ESP.restart();
  }
}
