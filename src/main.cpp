#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "websocket_manager.h"
#include "spi_manager.h"
#include "mqtt.h"


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

  if (wifiConnected) {

    //========== ZEIT SYNCHRONISIEREN =============================

    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");

    struct tm timeinfo;
    Serial.print("[TIME] Warte auf NTP-Zeit");

    while (!getLocalTime(&timeinfo)) {
      Serial.print(".");
      delay(500);
    }

    Serial.println("");
    Serial.println("[TIME] Zeit synchronisiert");
    Serial.println(&timeinfo, "%d.%m.%Y %H:%M:%S");

    //===============================================================
  }

  mqttConnect();
}


void loop() {

  handleWifiConnectAttempt();

  handleRestart();

  mqttLoop();

  if (wifiConnected) {
    sendDemoWebsocketData();
  }

  handleSpiData();
}
