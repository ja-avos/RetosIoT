#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <DHT.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#include "secrets.h"
#include "config.h"
#include "webpage.h"

// --- OBJECTS ---
CRGB leds[NUM_LEDS];
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);
WebServer server(80);

// --- GLOBAL VARIABLES ---
CRGB userColor = CRGB::Black; 
bool ledState = false;        
float temp = 0;
float hum = 0;
int lux = 0;

// Logic Flags & Timers
unsigned long lastMqttBlink = 0;
bool isBlinking = false;
unsigned long lastSensorRead = 0;

// --- HELPER FUNCTIONS ---

int calculateLux(int rawADC) {
  int invertedADC = 4095 - rawADC;

  float voltage = invertedADC / 4095.0 * 3.3;
  if (voltage == 0) return 0;
  
  float resistance = 2000 * voltage / (1 - voltage / 5);
  float luxValue = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  
  return (luxValue > 100000) ? 100000 : (int)luxValue;
}

void processIncomingMessage(char* topic, byte* payload, unsigned int length) {
  Serial.println("MQTT Message received!");
  isBlinking = true;
  lastMqttBlink = millis();
  leds[0] = CRGB::Green;
  FastLED.show();
}

void connectMQTT() {
  Serial.print("Connecting MQTT...");
  // Use a random client ID to avoid collisions if you reset quickly
  String clientId = String(MQTT_CLIENT_ID) + String(random(0xffff), HEX);
  
  if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
    Serial.println("Success");
    mqttClient.subscribe(MQTT_SUB_TOPIC);
  } else {
    Serial.print("Fail rc=");
    Serial.println(mqttClient.state());
  }
}

// --- WEB SERVER HANDLERS ---

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleData() {
  StaticJsonDocument<256> doc;
  doc["temp"] = temp;
  doc["hum"] = hum;
  doc["lux"] = lux;
  doc["ledState"] = ledState;
  doc["mqtt"] = mqttClient.connected(); // Send MQTT status
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSetColor() {
  if (server.hasArg("hex")) {
    String hex = server.arg("hex");
    long number = (long) strtol(hex.c_str(), NULL, 16);
    userColor = CRGB(number);
    
    // If switch is ON, update immediately
    if (ledState) {
      leds[0] = userColor;
      FastLED.show();
    } 

    ledState = true;
    leds[0] = userColor;
    FastLED.show();
  }
  server.send(200, "text/plain", "OK");
}

void handleSetState() {
  if (server.hasArg("state")) {
    ledState = (server.arg("state") == "1");
    leds[0] = ledState ? userColor : CRGB::Black;
    FastLED.show();
  }
  server.send(200, "text/plain", "OK");
}

void handleReset() {
  server.send(200, "text/plain", "Restarting...");
  delay(500);
  ESP.restart();
}

// --- SETUP ---

void setup() {
  Serial.begin(115200);
  
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK");
  secureClient.setInsecure(); 

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(processIncomingMessage);
  connectMQTT();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set-color", handleSetColor);
  server.on("/set-state", handleSetState);
  server.on("/reset", handleReset); // New reset endpoint
  server.begin();
}

// --- LOOP ---

void loop() {
  unsigned long currentMillis = millis();

  // 1. MQTT Keep-alive
  if (!mqttClient.connected()) {
    static unsigned long lastReconnect = 0;
    if (currentMillis - lastReconnect > RECONNECT_INTERVAL) {
      lastReconnect = currentMillis;
      connectMQTT();
    }
  } else {
    mqttClient.loop();
  }

  // 2. Read Sensors & PUBLISH (Every 2 seconds)
  if (currentMillis - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = currentMillis;
    
    // Read Data
    float newT = dht.readTemperature();
    float newH = dht.readHumidity();
    int rawLight = analogRead(LDR_PIN);
    
    // Validate and Assign
    if (!isnan(newT)) temp = newT;
    if (!isnan(newH)) hum = newH;
    lux = calculateLux(rawLight);

    // --- SEND TO MQTT (New Logic) ---
    if (mqttClient.connected()) {
      char payload[50]; // Buffer to hold the JSON string
      Serial.printf("Sending data to MQTT - %d\n", currentMillis);
      // 1. Publish Temperature
      String jsonT = "{\"value\": " + String(temp) + "}";
      jsonT.toCharArray(payload, 50);
      mqttClient.publish(MQTT_PUB_TEMP, payload);
      Serial.printf("Sent to [%s] value: %s\n", MQTT_PUB_TEMP, payload);

      // 2. Publish Humidity
      String jsonH = "{\"value\": " + String(hum) + "}";
      jsonH.toCharArray(payload, 50);
      mqttClient.publish(MQTT_PUB_HUM, payload);
      Serial.printf("Sent to [%s] value: %s\n", MQTT_PUB_HUM, payload);

      // 3. Publish Light
      String jsonL = "{\"value\": " + String(lux) + "}";
      jsonL.toCharArray(payload, 50);
      mqttClient.publish(MQTT_PUB_LIGHT, payload);
      Serial.printf("Sent to [%s] value: %s\n", MQTT_PUB_LIGHT, payload);
      
    }
  }

  // 3. Blink Logic
  if (isBlinking) {
    if (currentMillis - lastMqttBlink > BLINK_DURATION) {
      isBlinking = false;
      leds[0] = ledState ? userColor : CRGB::Black;
      FastLED.show();
    }
  }

  // 4. Handle Web Requests
  server.handleClient();
}