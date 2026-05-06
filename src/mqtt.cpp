#include "mqtt.h"
#include <WiFi.h>
#include <PubSubClient.h>

// --- MQTT CONFIG ---
const char* MQTT_HOST = "mqtt.datacake.co";
const uint16_t MQTT_PORT = 1883;

// HIER EINTRAGEN
const char* MQTT_USER = "a7ab1b44644d72e5c7550394c3f5775652408bce";
const char* MQTT_PASS = "a7ab1b44644d72e5c7550394c3f5775652408bce";

const char* MQTT_TOPIC_BASE = "dtck-pub/c7750/1a060742-68f5-4aed-9cb0-09e14708cb9d";

static String mqttClientId;
static unsigned long lastReconnectAttempt = 0;

// --- CLIENT ---
WiFiClient mqttWifiClient;
PubSubClient mqtt(mqttWifiClient);

static String getMqttClientId() {
  if (mqttClientId.length() == 0) {
    mqttClientId = "ESP32_C7750_";
    mqttClientId += WiFi.macAddress();
    mqttClientId.replace(":", "");
  }

  return mqttClientId;
}

// =========================
// MQTT CONNECT
// =========================
void mqttConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[MQTT] WLAN nicht verbunden, MQTT wird uebersprungen.");
    return;
  }

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(10);

  if (mqtt.connected()) {
    return;
  }

  lastReconnectAttempt = millis();

  Serial.print("[MQTT] Connecting to ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.print(" as ");
  Serial.print(getMqttClientId());
  Serial.print(" ... ");

  // MIT USERNAME + PASSWORT
  if (mqtt.connect(
        getMqttClientId().c_str(),
        MQTT_USER,
        MQTT_PASS
      )) {

    Serial.println("connected");

    Serial.print("[MQTT] Publish base topic: ");
    Serial.println(MQTT_TOPIC_BASE);

  } else {
    Serial.print("failed, rc=");
    Serial.println(mqtt.state());
  }
}

// =========================
// MQTT LOOP
// =========================
void mqttLoop() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!mqtt.connected()) {
    unsigned long now = millis();

    if (now - lastReconnectAttempt >= 5000) {
      lastReconnectAttempt = now;
      mqttConnect();
    }
  }

  if (mqtt.connected()) {
    mqtt.loop();
  }
}

// =========================
// MQTT SEND
// =========================
void mqttSend(const char* field, String value) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!mqtt.connected()) {
    mqttConnect();

    if (!mqtt.connected()) {
      Serial.println("[MQTT] Publish uebersprungen, keine Broker-Verbindung.");
      return;
    }
  }

  String topic = MQTT_TOPIC_BASE;
  topic += "/";
  topic += field;

  Serial.print("[MQTT] Publish ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(value);

  bool ok = mqtt.publish(topic.c_str(), value.c_str(), false);

  if (ok) {
    Serial.println("[MQTT] Publish OK");
  } else {
    Serial.println("[MQTT] Publish FEHLGESCHLAGEN");
  }
}
