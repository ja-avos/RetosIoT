#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
//  HARDWARE PINS
// ============================================================
#define DHT_PIN           23
#define LDR_PIN           33
#define LED_PIN           22

// ============================================================
//  SENSOR CONFIGURATION
// ============================================================
#define DHT_TYPE          DHT11
#define NUM_LEDS          1
#define LED_BRIGHTNESS    150

// ============================================================
//  LDR CALIBRATION
//  Adjust GAMMA and RL10 based on your specific LDR
// ============================================================
const float LDR_GAMMA     = 0.7;
const float LDR_RL10      = 50;

// ============================================================
//  MQTT TOPICS
// ============================================================
#define MQTT_TOPIC_COUNTRY   "colombia"
#define MQTT_TOPIC_STATE   "cundinamarca"
#define MQTT_TOPIC_CITY   "cajica"
#define MQTT_TOPIC_USER   "user1"

// Single publish topic for all sensor data
#define MQTT_TOPIC_DATA   MQTT_TOPIC_COUNTRY "/" MQTT_TOPIC_STATE "/" MQTT_TOPIC_CITY "/" MQTT_TOPIC_USER "/out"
// Single subscribe topic for incoming messages / alerts
#define MQTT_TOPIC_MSG    MQTT_TOPIC_COUNTRY "/" MQTT_TOPIC_STATE "/" MQTT_TOPIC_CITY "/" MQTT_TOPIC_USER "/in"

// ============================================================
//  MQTT TRANSPORT
//  0 = plain MQTT (WiFiClient), 1 = TLS MQTT (WiFiClientSecure)
// ============================================================
#define MQTT_USE_TLS      0

// ============================================================
//  TIMING (milliseconds)
// ============================================================
const unsigned long SENSOR_READ_INTERVAL    = 2000;   // Sensor read & MQTT publish rate
const unsigned long MQTT_RECONNECT_INTERVAL = 5000;   // MQTT reconnect delay
const unsigned long LED_BLINK_INTERVAL      = 300;    // LED blink step duration

// ============================================================
//  MESSAGE / ALERT LED BEHAVIOR
// ============================================================
const int NORMAL_MSG_BLINK_COUNT = 3;    // Blue blinks for normal messages
const int ALERT_MSG_BLINK_COUNT  = 5;    // Red/Yellow alternation for alerts

// ============================================================
//  WEB SERVER
// ============================================================
#define WEB_SERVER_PORT   80

#endif