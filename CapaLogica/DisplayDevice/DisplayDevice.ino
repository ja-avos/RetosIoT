// ============================================================
//  DisplayDevice — IoT Round Display Station
//  Receives sensor data via MQTT and displays on a 240x240
//  round GC9A01 screen.  Subscribes to a messaging topic for
//  normal notifications and dramatic full-screen alerts.
//  Serves a web interface for testing messages and alerts.
// ============================================================

#include "config.h"

#include <LovyanGFX.hpp>
#include <WiFi.h>
#if MQTT_USE_TLS
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif
#include <PubSubClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#include "secrets.h"
#include "display.h"
#include "webpage.h"

// ============================================================
//  OBJECTS
// ============================================================
LGFX tft;
LGFX_Sprite sprite(&tft);

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

// Sensor data (received via MQTT)
float temperature = 0.0;
float humidity    = 0.0;
int   luminosity  = 0;
String mqttStatus = "Connecting...";

// Display mode
enum DisplayMode { MODE_NORMAL, MODE_MESSAGE, MODE_ALERT };
DisplayMode displayMode     = MODE_NORMAL;
String      currentMessage  = "";
unsigned long modeStartTime = 0;

// Timers
unsigned long lastDisplayUpdate  = 0;
unsigned long lastMqttReconnect  = 0;

// ============================================================
//  DISPLAY — GAUGE DRAWING
// ============================================================

void drawGauge() {
  sprite.fillScreen(TFT_BLACK);

  // Background ring
  sprite.fillArc(120, 120, 118, 100, 0, 360, 0x222222);

  // Temperature arc (135°–405° range mapped from 0–50 °C)
  float clamped  = constrain(temperature, 0, 50);
  int startAngle = 135;
  int endAngle   = 135 + (int)((clamped / 50.0) * 270);

  uint16_t arcColor = TFT_GREEN;
  if (temperature > 30)      arcColor = TFT_RED;
  else if (temperature > 25) arcColor = TFT_ORANGE;
  else if (temperature < 15) arcColor = TFT_SKYBLUE;

  sprite.fillArc(120, 120, 118, 100, startAngle, endAngle, arcColor);

  // Temperature value (center)
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.setTextDatum(middle_center);
  sprite.setFont(&fonts::Font7);
  sprite.setTextSize(1);
  sprite.drawString(String(temperature, 1), 120, 105);

  sprite.setFont(&fonts::Font4);
  sprite.drawString("C", 120, 150);

  // Humidity (bottom-left)
  sprite.setTextColor(TFT_CYAN, TFT_BLACK);
  sprite.setFont(&fonts::Font2);
  sprite.setTextSize(2);
  sprite.drawString(String(humidity, 0) + "%", 60, 180);

  // Luminosity (bottom-right)
  sprite.setTextColor(TFT_YELLOW, TFT_BLACK);
  sprite.drawString(String(luminosity), 180, 180);

  // Labels
  sprite.setTextSize(1);
  sprite.setTextColor(TFT_SILVER, TFT_BLACK);
  sprite.drawString("HUM", 60, 205);
  sprite.drawString("LUX", 180, 205);

  // Status (top)
  sprite.setTextColor(TFT_DARKGREY, TFT_BLACK);
  sprite.drawString(mqttStatus, 120, 40);
}

// ============================================================
//  DISPLAY — MESSAGE OVERLAY  (shown over the gauge)
// ============================================================

void drawMessageOverlay() {
  // Blue notification banner at the bottom of the circle
  sprite.fillRoundRect(15, 188, 210, 48, 8, 0x001F);
  sprite.drawRoundRect(15, 188, 210, 48, 8, TFT_BLUE);

  sprite.setTextColor(TFT_WHITE);
  sprite.setTextDatum(middle_center);
  sprite.setFont(&fonts::Font2);
  sprite.setTextSize(1);

  String display = currentMessage;
  if (display.length() > 22) display = display.substring(0, 19) + "...";
  sprite.drawString(display, 120, 212);
}

// ============================================================
//  DISPLAY — ALERT SCREEN  (dramatic full-screen blink)
// ============================================================

void drawAlertScreen() {
  bool phase = ((millis() / ALERT_BLINK_INTERVAL) % 2 == 0);

  if (phase) {
    sprite.fillScreen(TFT_RED);

    // Warning triangle
    int cx = 120, cy = 60;
    sprite.fillTriangle(cx, cy - 32, cx - 38, cy + 28, cx + 38, cy + 28, TFT_YELLOW);
    sprite.fillTriangle(cx, cy - 18, cx - 26, cy + 22, cx + 26, cy + 22, TFT_RED);

    // Exclamation mark inside triangle
    sprite.setTextColor(TFT_YELLOW);
    sprite.setTextDatum(middle_center);
    sprite.setFont(&fonts::Font4);
    sprite.setTextSize(1);
    sprite.drawString("!", cx, cy + 5);

    // ALERTA label
    sprite.setTextColor(TFT_WHITE);
    sprite.setFont(&fonts::Font4);
    sprite.drawString("ALERTA", 120, 120);

    // Message text (simple 2-line wrap)
    sprite.setTextColor(TFT_WHITE, TFT_RED);
    sprite.setFont(&fonts::Font4);
    sprite.setTextSize(1);
    if (currentMessage.length() <= 16) {
      sprite.drawString(currentMessage, 120, 165);
    } else {
      int split = currentMessage.lastIndexOf(' ', 16);
      if (split <= 0) split = 16;
      sprite.drawString(currentMessage.substring(0, split), 120, 155);
      String line2 = currentMessage.substring(split + 1);
      if (line2.length() > 16) line2 = line2.substring(0, 13) + "...";
      sprite.drawString(line2, 120, 185);
    }
  } else {
    // Dark flash phase
    sprite.fillScreen(TFT_BLACK);
    sprite.setTextColor(0x8000);  // dark red
    sprite.setTextDatum(middle_center);
    sprite.setFont(&fonts::Font4);
    sprite.setTextSize(2);
    sprite.drawString("!!!", 120, 120);
  }

  sprite.pushSprite(0, 0);
}

// ============================================================
//  DISPLAY — UPDATE  (called from loop)
// ============================================================

void updateDisplay() {
  unsigned long now = millis();

  // Check expiration of current mode
  if (displayMode == MODE_ALERT && now - modeStartTime > ALERT_DURATION) {
    displayMode = MODE_NORMAL;
  }
  if (displayMode == MODE_MESSAGE && now - modeStartTime > MESSAGE_DURATION) {
    displayMode = MODE_NORMAL;
  }

  if (displayMode == MODE_ALERT) {
    drawAlertScreen();              // pushes internally for blink control
  } else {
    drawGauge();
    if (displayMode == MODE_MESSAGE) {
      drawMessageOverlay();         // drawn on top of gauge sprite
    }
    sprite.pushSprite(0, 0);
  }
}

// ============================================================
//  INCOMING MESSAGE HANDLER
// ============================================================

void handleIncomingMessage(const String& type, const String& message) {
  currentMessage = message;
  modeStartTime  = millis();

  if (type == "alert") {
    displayMode = MODE_ALERT;
    Serial.printf("[MSG] ALERT: %s\n", message.c_str());
  } else {
    displayMode = MODE_MESSAGE;
    Serial.printf("[MSG] Normal: %s\n", message.c_str());
  }
}

// ============================================================
//  MQTT
// ============================================================

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  char buf[256];
  unsigned int len = min(length, (unsigned int)255);
  memcpy(buf, payload, len);
  buf[len] = '\0';

  String topicStr(topic);
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, buf)) {
    Serial.printf("[MQTT] JSON parse error on %s\n", topic);
    return;
  }

  if (topicStr == MQTT_TOPIC_DATA) {
    temperature = doc["temperature"] | 0.0f;
    humidity    = doc["humidity"]    | 0.0f;
    luminosity  = doc["luminosity"]  | 0;
  } else if (topicStr == MQTT_TOPIC_MSG) {
    String type    = doc["type"]    | "normal";
    String message = doc["message"] | "";
    handleIncomingMessage(type, message);
  }
}

void connectMqtt() {
  String clientId = String(MQTT_CLIENT_ID) + "-" + String(random(0xFFFF), HEX);
  Serial.printf("[MQTT] Connecting as %s...\n", clientId.c_str());

  if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
    Serial.println("[MQTT] Connected");
    mqttClient.subscribe(MQTT_TOPIC_DATA);
    mqttClient.subscribe(MQTT_TOPIC_MSG);
    mqttStatus = "ONLINE";
    Serial.println("[MQTT] Subscribed to all topics");
  } else {
    Serial.printf("[MQTT] Failed, rc=%d\n", mqttClient.state());
    mqttStatus = "MQTT ERR";
  }
}

// ============================================================
//  WEB SERVER
// ============================================================

void handleRoot() {
  server.send(200, "text/html", WEBPAGE_HTML);
}

void handleData() {
  StaticJsonDocument<256> doc;
  doc["temp"] = temperature;
  doc["hum"]  = humidity;
  doc["lux"]  = luminosity;
  doc["mqtt"] = mqttClient.connected();
  doc["mode"] = (displayMode == MODE_ALERT)   ? "alert"
              : (displayMode == MODE_MESSAGE) ? "message"
                                              : "normal";
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSendMessage() {
  String type    = server.arg("type");
  String message = server.arg("message");

  if (message.length() == 0) {
    server.send(400, "text/plain", "Message required");
    return;
  }

  // Publish via MQTT so every device reacts
  if (mqttClient.connected()) {
    StaticJsonDocument<256> doc;
    doc["type"]    = type;
    doc["message"] = message;

    char payload[256];
    serializeJson(doc, payload, sizeof(payload));
    mqttClient.publish(MQTT_TOPIC_MSG, payload);
    server.send(200, "text/plain", "Sent via MQTT");
  } else {
    // MQTT offline — trigger local display only
    handleIncomingMessage(type, message);
    server.send(200, "text/plain", "Shown locally (MQTT offline)");
  }
}

void handleReset() {
  server.send(200, "text/plain", "Restarting...");
  delay(500);
  ESP.restart();
}

// ============================================================
//  OTA
// ============================================================

void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]()   { Serial.println("[OTA] Start"); });
  ArduinoOTA.onEnd([]()     { Serial.println("[OTA] Done");  });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
    Serial.printf("[OTA] %u%%\r", p / (t / 100));
  });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.printf("[OTA] Error %u\n", e);
  });
  ArduinoOTA.begin();
}

// ============================================================
//  SETUP
// ============================================================

void setup() {
  // Backlight on
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  Serial.begin(115200);
  delay(500);
  Serial.println("\n=============================");
  Serial.println("  DisplayDevice Starting...");
  Serial.println("=============================\n");

  // Display init
  tft.init();
  tft.startWrite();
  sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  sprite.setSwapBytes(true);
  drawGauge();
  sprite.pushSprite(0, 0);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WIFI] Connecting to %s", WIFI_SSID);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI] Connected — IP: %s\n",
                  WiFi.localIP().toString().c_str());
    setupOTA();

    // MQTT
  #if MQTT_USE_TLS
    secureClient.setInsecure();
  #endif
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(onMqttMessage);
    mqttClient.setBufferSize(512);
    connectMqtt();

    // Web server
    server.on("/",     handleRoot);
    server.on("/data", handleData);
    server.on("/send", handleSendMessage);
    server.on("/reset", handleReset);
    server.begin();
    Serial.printf("[WEB] Server started on port %d\n", WEB_SERVER_PORT);
  } else {
    Serial.println("[WIFI] Connection failed");
    mqttStatus = "NO WIFI";
  }
}

// ============================================================
//  LOOP
// ============================================================

void loop() {
  unsigned long now = millis();

  ArduinoOTA.handle();

  // --- MQTT keep-alive & reconnect ---
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      mqttStatus = "RECONNECTING";
      if (now - lastMqttReconnect > MQTT_RECONNECT_INTERVAL) {
        lastMqttReconnect = now;
        connectMqtt();
      }
    } else {
      mqttClient.loop();
    }
  }

  // --- Display refresh (faster during alerts for smooth blinking) ---
  unsigned long refreshInterval =
      (displayMode == MODE_ALERT) ? 50 : DISPLAY_REFRESH_INTERVAL;

  if (now - lastDisplayUpdate > refreshInterval) {
    lastDisplayUpdate = now;
    updateDisplay();
  }

  // --- Web server ---
  server.handleClient();

  delay(5);
}