// ============================================================
//  MeasureDevice — IoT Sensor Station
//  Reads temperature, humidity, and luminosity.
//  Publishes sensor data via MQTT.
//  Subscribes to messaging topic for LED alerts.
//  Serves a web dashboard for monitoring and LED control.
// ============================================================

#include "config.h"

#include <WiFi.h>
#if MQTT_USE_TLS
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif
#include <PubSubClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#include "secrets.h"
#include "webpage.h"

// ============================================================
//  OBJECTS
// ============================================================
CRGB leds[NUM_LEDS];
DHT dht(DHT_PIN, DHT_TYPE);
#if MQTT_USE_TLS
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);
#else
WiFiClient mqttNetClient;
PubSubClient mqttClient(mqttNetClient);
#endif
WebServer server(WEB_SERVER_PORT);

// ============================================================
//  STATE
// ============================================================

// Sensor readings
float temperature = 0.0;
float humidity    = 0.0;
int   luminosity  = 0;

// LED state
CRGB userColor   = CRGB::Black;
bool ledEnabled  = false;

// Blink state machine
enum BlinkMode { BLINK_NONE, BLINK_NORMAL, BLINK_ALERT };
BlinkMode blinkMode       = BLINK_NONE;
int       blinkStep       = 0;
int       blinkTotalSteps = 0;
unsigned long lastBlinkTime = 0;

// Timers
unsigned long lastSensorRead    = 0;
unsigned long lastMqttReconnect = 0;

// ============================================================
//  SENSOR HELPERS
// ============================================================

int calculateLux(int rawADC) {
  int inverted = 4095 - rawADC;
  float voltage = inverted / 4095.0 * 3.3;
  if (voltage <= 0) return 0;

  float resistance = 2000.0 * voltage / (1.0 - voltage / 5.0);
  float luxValue = pow(LDR_RL10 * 1e3 * pow(10, LDR_GAMMA) / resistance,
                       1.0 / LDR_GAMMA);
  return constrain((int)luxValue, 0, 100000);
}

void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int   raw = analogRead(LDR_PIN);

  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity    = h;
  luminosity = calculateLux(raw);
}

// ============================================================
//  LED BLINK STATE MACHINE
// ============================================================

void startBlink(BlinkMode mode) {
  blinkMode       = mode;
  blinkStep       = 0;
  blinkTotalSteps = (mode == BLINK_NORMAL)
                        ? NORMAL_MSG_BLINK_COUNT * 2
                        : ALERT_MSG_BLINK_COUNT * 2;
  lastBlinkTime   = millis();

  // Show first state immediately
  leds[0] = (mode == BLINK_NORMAL) ? CRGB::Blue : CRGB::Red;
  FastLED.show();
}

void updateBlink() {
  if (blinkMode == BLINK_NONE) return;

  unsigned long now = millis();
  if (now - lastBlinkTime < LED_BLINK_INTERVAL) return;
  lastBlinkTime = now;
  blinkStep++;

  if (blinkStep >= blinkTotalSteps) {
    // Finished — restore user LED state
    blinkMode = BLINK_NONE;
    leds[0]   = ledEnabled ? userColor : CRGB::Black;
    FastLED.show();
    return;
  }

  bool isOn = (blinkStep % 2 == 0);

  if (blinkMode == BLINK_NORMAL) {
    leds[0] = isOn ? CRGB::Blue : CRGB::Black;
  } else {  // BLINK_ALERT
    leds[0] = isOn ? CRGB::Red : CRGB::Yellow;
  }
  FastLED.show();
}

// ============================================================
//  MQTT
// ============================================================

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String topicStr(topic);

  if (topicStr == MQTT_TOPIC_MSG) {
    char buf[256];
    unsigned int len = min(length, (unsigned int)255);
    memcpy(buf, payload, len);
    buf[len] = '\0';

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, buf)) return;

    String type = doc["type"] | "normal";

    if (type == "alert") {
      startBlink(BLINK_ALERT);
    } else {
      startBlink(BLINK_NORMAL);
    }
    Serial.printf("[MQTT] Message — type: %s\n", type.c_str());
  }
}

void connectMqtt() {
  String clientId = String(MQTT_CLIENT_ID) + "-" + String(random(0xFFFF), HEX);
  Serial.printf("[MQTT] Connecting as %s...\n", clientId.c_str());

  if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
    Serial.println("[MQTT] Connected");
    mqttClient.subscribe(MQTT_TOPIC_MSG);
    Serial.printf("[MQTT] Subscribed to %s\n", MQTT_TOPIC_MSG);
  } else {
    Serial.printf("[MQTT] Failed, rc=%d\n", mqttClient.state());
  }
}

void publishSensorData() {
  if (!mqttClient.connected()) return;

  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"temperature\":%.1f,\"humidity\":%.1f,\"luminosity\":%d}",
           temperature, humidity, luminosity);

  mqttClient.publish(MQTT_TOPIC_DATA, payload);
  Serial.printf("[MQTT] Published T=%.1f H=%.1f L=%d\n",
                temperature, humidity, luminosity);
}

// ============================================================
//  WEB SERVER HANDLERS
// ============================================================

void handleRoot() {
  server.send(200, "text/html", WEBPAGE_HTML);
}

void handleData() {
  StaticJsonDocument<256> doc;
  doc["temp"]     = temperature;
  doc["hum"]      = humidity;
  doc["lux"]      = luminosity;
  doc["ledState"] = ledEnabled;
  doc["mqtt"]     = mqttClient.connected();

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSetColor() {
  if (server.hasArg("hex")) {
    long color = strtol(server.arg("hex").c_str(), NULL, 16);
    userColor  = CRGB(color);
    ledEnabled = true;
    leds[0]    = userColor;
    FastLED.show();
  }
  server.send(200, "text/plain", "OK");
}

void handleSetState() {
  if (server.hasArg("state")) {
    ledEnabled = (server.arg("state") == "1");
    leds[0]    = ledEnabled ? userColor : CRGB::Black;
    FastLED.show();
  }
  server.send(200, "text/plain", "OK");
}

void handleReset() {
  server.send(200, "text/plain", "Restarting...");
  delay(500);
  ESP.restart();
}

// ============================================================
//  SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n=============================");
  Serial.println("  MeasureDevice Starting...");
  Serial.println("=============================\n");

  // Hardware init
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  dht.begin();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[WIFI] Connected — IP: %s\n",
                WiFi.localIP().toString().c_str());

  // MQTT
#if MQTT_USE_TLS
  secureClient.setInsecure();
#endif
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  connectMqtt();

  // Web server
  server.on("/",          handleRoot);
  server.on("/data",      handleData);
  server.on("/set-color", handleSetColor);
  server.on("/set-state", handleSetState);
  server.on("/reset",     handleReset);
  server.begin();
  Serial.printf("[WEB] Server started on port %d\n", WEB_SERVER_PORT);
}

// ============================================================
//  LOOP
// ============================================================

void loop() {
  unsigned long now = millis();

  // --- MQTT keep-alive & reconnect ---
  if (!mqttClient.connected()) {
    if (now - lastMqttReconnect > MQTT_RECONNECT_INTERVAL) {
      lastMqttReconnect = now;
      connectMqtt();
    }
  } else {
    mqttClient.loop();
  }

  // --- Read sensors & publish ---
  if (now - lastSensorRead > SENSOR_READ_INTERVAL) {
    lastSensorRead = now;
    readSensors();
    publishSensorData();
  }

  // --- LED blink state machine ---
  updateBlink();

  // --- Web server ---
  server.handleClient();
}