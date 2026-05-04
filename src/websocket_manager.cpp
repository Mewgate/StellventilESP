#include "websocket_manager.h"

#include <WiFi.h>

AsyncWebSocket ws("/ws");

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WS Client connected: id=%u ip=%s\n",
                  client->id(),
                  client->remoteIP().toString().c_str());

    client->text("12345|22|03:15|Automatik|-61");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WS Client disconnected: id=%u\n", client->id());
  } else if (type == WS_EVT_ERROR) {
    Serial.printf("WS Client error: id=%u\n", client->id());
  }
}

void setupWebSocket() {
  ws.onEvent(onWsEvent);
}

void sendWebsocketMessage(const String& message) {
  ws.textAll(message);
  ws.cleanupClients();
}

void sendDemoWebsocketData() {
  static unsigned long lastSend = 0;

  if (millis() - lastSend < 500) return;
  lastSend = millis();

  int valveFlow = random(0, 50001);
  int leftValue = random(0, 50001);
  int hours = random(0, 24);
  int minutes = random(0, 60);

  String mode;
  int modeRand = random(0, 3);

  if (modeRand == 0) mode = "Automatik";
  else if (modeRand == 1) mode = "Manuell";
  else mode = "Standby";

  String timeText =
    String(hours < 10 ? "0" : "") + String(hours) + ":" +
    String(minutes < 10 ? "0" : "") + String(minutes);

  String msg = String(valveFlow) + "|" +
               String(leftValue) + "|" +
               timeText + "|" +
               mode + "|" +
               String(WiFi.RSSI());

  Serial.print("Sende WS: ");
  Serial.println(msg);

  sendWebsocketMessage(msg);
}
