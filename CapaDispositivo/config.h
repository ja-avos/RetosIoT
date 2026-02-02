#ifndef CONFIG_H
#define CONFIG_H

// --- HARDWARE PINS ---
#define DHT_PIN       23
#define LDR_PIN       33
#define LED_PIN       22

// --- SENSOR SETTINGS ---
#define DHT_TYPE      DHT11
#define NUM_LEDS      1
#define LED_BRIGHTNESS 150

// --- MQTT TOPICS ---
// Susbcribe topics
#define MQTT_SUB_TOPIC  "temperatura/<ciudad>/<usuario>"
// Publishing Topics
#define MQTT_PUB_TEMP   "temperatura/<ciudad>/<usuario>"
#define MQTT_PUB_HUM    "humedad/<ciudad>/<usuario>"
#define MQTT_PUB_LIGHT  "luminosidad/<ciudad>/<usuario>"

// --- LDR CALIBRATION ---
// Adjust these based on your specific LDR and resistor
const float GAMMA = 0.7;
const float RL10 = 50; 

// --- TIMING (Milliseconds) ---
const int BLINK_DURATION = 1000;
const int SENSOR_INTERVAL = 500;
const int RECONNECT_INTERVAL = 5000;

#endif