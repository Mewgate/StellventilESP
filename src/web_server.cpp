#include "web_server.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "config.h"
#include "html_pages.h"
#include "wifi_manager.h"
#include "websocket_manager.h"

static AsyncWebServer server(80);

void setupWebServer() {
  setupWebSocket();
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (wifiConnected) {
      request->send_P(200, "text/html; charset=utf-8", INDEX_HTML);
    } else {
      request->send_P(200, "text/html; charset=utf-8", CONFIG_HTML);
    }
  });

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

    saveCredentials(ssid, pass);

    Serial.println("Neue WLAN-Daten gespeichert.");
    Serial.println("Neustart wird vorbereitet...");

    request->send_P(200, "text/html; charset=utf-8", SAVED_HTML);

    restartPending = true;
    restartAt = millis() + 2000;
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    clearCredentials();
    request->send(200, "text/plain", "Gespeicherte WLAN-Daten geloescht. Neustart...");

    restartPending = true;
    restartAt = millis() + 1000;
  });

  server.begin();
  Serial.println("HTTP/WebSocket server started.");
}

void handleRestart() {
  if (restartPending && millis() >= restartAt) {
    Serial.println("ESP startet jetzt neu...");
    delay(200);
    ESP.restart();
  }
}
