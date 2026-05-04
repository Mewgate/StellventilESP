#include "spi_manager.h"

#include <Arduino.h>
#include <ESP32SPISlave.h>

#include "config.h"
#include "websocket_manager.h"

static ESP32SPISlave slave;

void setupSpi() {
  slave.setDataMode(SPI_MODE0);
  slave.setQueueSize(1);
  slave.begin(VSPI, 18, 19, 23, 5);
  Serial.println("ESP32 SPI Slave ready");
}

void handleSpiData() {
  /*
  uint8_t rxBuf[RANGE] = {0};
  uint8_t txBuf[RANGE] = {0x05};

  size_t received = slave.transfer(txBuf, rxBuf, sizeof(rxBuf));

  if (received > 0) {
    uint32_t valveFlow = 0;

    for (int i = 0; i < received; i++) {
      valveFlow = (valveFlow << 8) | rxBuf[i];
    }

    String msg = String(valveFlow) + "|" + String(WiFi.RSSI());
    sendWebsocketMessage(msg);

    Serial.print("Valve flow: ");
    Serial.print(valveFlow);
    Serial.println(" g/h");
    Serial.println(received);
  }
  */
}
