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


    // =========================
    // ZEIT SYNCHRONISIEREN
    // =========================
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    struct tm timeinfo;
    Serial.print("[TIME] Warte auf NTP-Zeit");

    while (!getLocalTime(&timeinfo)) {
      Serial.print(".");
      delay(500);
    }

    Serial.println("");
    Serial.println("[TIME] Zeit synchronisiert");
    Serial.println(&timeinfo, "%d.%m.%Y %H:%M:%S");
    
  }
  setupWebServer();
  mqttConnect();
}


void loop() {
  handleRestart();

  mqttLoop();

  if (wifiConnected) {
    sendDemoWebsocketData();
  }

  handleSpiData();
}
