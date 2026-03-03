#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
//  DISPLAY HARDWARE
// ============================================================
#define SCREEN_WIDTH      240
#define SCREEN_HEIGHT     240
#define BACKLIGHT_PIN     3

// ============================================================
//  SPI PINS (ESP32-2424S012)
// ============================================================
#define SPI_SCLK          6
#define SPI_MOSI          7
#define SPI_DC            2
#define SPI_CS            10

// ============================================================
//  MQTT TOPICS — Subscriptions
// ============================================================
#define MQTT_TOPIC_COUNTRY   "colombia"
#define MQTT_TOPIC_STATE   "cundinamarca"
#define MQTT_TOPIC_CITY   "cajica"
#define MQTT_TOPIC_USER   "user1"

// Subscribe to sensor data published by MeasureDevice
#define MQTT_TOPIC_DATA   MQTT_TOPIC_COUNTRY "/" MQTT_TOPIC_STATE "/" MQTT_TOPIC_CITY "/" MQTT_TOPIC_USER "/out"
// Subscribe/publish topic for incoming messages / alerts
#define MQTT_TOPIC_MSG    MQTT_TOPIC_COUNTRY "/" MQTT_TOPIC_STATE "/" MQTT_TOPIC_CITY "/" MQTT_TOPIC_USER "/in"

// ============================================================
//  MQTT TRANSPORT
//  0 = plain MQTT (WiFiClient), 1 = TLS MQTT (WiFiClientSecure)
// ============================================================
#define MQTT_USE_TLS      0

// ============================================================
//  TIMING (milliseconds)
// ============================================================
const unsigned long DISPLAY_REFRESH_INTERVAL = 500;    // Normal mode screen refresh
const unsigned long MQTT_RECONNECT_INTERVAL  = 5000;   // MQTT reconnect delay
const unsigned long ALERT_DURATION           = 15000;  // Alert on-screen duration
const unsigned long ALERT_BLINK_INTERVAL     = 700;    // Alert blink speed
const unsigned long MESSAGE_DURATION         = 8000;   // Normal message on-screen duration

// ============================================================
//  OTA
// ============================================================
#define OTA_HOSTNAME      "RoundDisplay"
#define OTA_PASSWORD      "12345678"

// ============================================================
//  WEB SERVER
// ============================================================
#define WEB_SERVER_PORT   80

#endif
