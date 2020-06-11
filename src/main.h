#include <config.h>

const char* ssid = WIFI_SSID;
const char* password =  WIFI_PASSWORD;
const char* mqttServer = MQTT_SERVER;
const int mqttPort = MQTT_PORT
const char* mqttUser = MQTT_USER;
const char* mqttPassword = MQTT_PASSWORD;
const char* switch_topic = MAIN_TOPIC "/kapcsolo";
const char* notification_topic = MAIN_TOPIC "/notification";
const char* time_interval_topic = MAIN_TOPIC "/time_interval";
const char* status_topic = MAIN_TOPIC "/status";

#define BUILTIN_LED_ON false
#define BUILTIN_LED_OFF true