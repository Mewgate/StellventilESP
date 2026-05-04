#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "websocket_manager.h"
#include "spi_manager.h"

void setup() {
  Serial.begin(115200);
  delay(200);
  randomSeed(micros());

  setupSpi();

  wifiConnected = connectToSavedWifi();

  if (!wifiConnected) {
    startConfigAP();
  } else {
    setupMdns();
  }

  setupWebServer();
}

void loop() {
  handleRestart();

  if (wifiConnected) {
    sendDemoWebsocketData();
  }

  handleSpiData();
}
