#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <main.h>
#include <data_types.cpp>

#define DEBUG

unsigned long previousMillis = 0;
unsigned long interval = 1000;

unsigned long pumpStateCheckStart = 0;
unsigned long pump_on_interval = 0;
boolean toggle = false;

PUMP_STATE pump_state;
PUMP_NOTIFICATION_STATE notofication_state;

WiFiClient espClient;
PubSubClient client(espClient);
 
void callback(char* topic, byte* payload, unsigned int length) {
  char payload_as_char[length];
  #ifdef DEBUG
    Serial.println("Message arrived in topic: " + (String)topic);
  #endif

  #ifdef DEBUG
    Serial.print("Message: ");
  #endif  
  for (unsigned int i = 0; i < length; i++) {
    #ifdef DEBUG
      Serial.print((char)payload[i]);
    #endif  
    payload_as_char[i] = (char)payload[i];
  }

  payload_as_char[length] = '\0';

  if (strcmp(payload_as_char, "on") == 0){   
    pump_state = switched_on;
    notofication_state = received_on;
    pumpStateCheckStart = millis();
  } else if (strcmp(payload_as_char, "off") == 0){
    pump_state = switched_off;
    pumpStateCheckStart = millis();
  }

  if (strcmp(topic, time_interval_topic) == 0) {
    pump_on_interval = atol(payload_as_char) * 1000 * 60;
    #ifdef DEBUG
      Serial.println("\ninterval: " + (String)pump_on_interval);
    #endif  
  }

  if (strcmp(topic, status_topic) == 0 && (strcmp(payload_as_char, "sync") == 0)) {
    client.publish(status_topic, "online");
  }

  #ifdef DEBUG
    Serial.println();
    Serial.println("-----------------------");
  #endif  
 
}

void softReset(){
  ESP.restart();
}
 
void setup() {
  pump_state = unknown;
  notofication_state = undefined;
  pinMode(D0, OUTPUT);
  digitalWrite(D0, BUILTIN_LED_OFF);
  
  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    toggle = !toggle;
    digitalWrite(D0, toggle);
    delay(500);
    #ifdef DEBUG 
      Serial.println("Connecting to WiFi..");
    #endif  
  }

  #ifdef DEBUG
    Serial.println("Connected to the WiFi network");
  #endif

  toggle = BUILTIN_LED_OFF;
  digitalWrite(D0, toggle);

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    #ifdef DEBUG
      Serial.println("Connecting to MQTT...");
    #endif  
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
      #ifdef DEBUG
        Serial.println("connected"); 
      #endif   
    } else {
      #ifdef DEBUG
        Serial.println("failed with state: " + (String)client.state());
      #endif  
      toggle = !toggle;
      digitalWrite(D0, toggle);
      delay(2000);
    }
  }
 
  toggle = BUILTIN_LED_OFF;
  digitalWrite(D0, toggle);

  client.subscribe(switch_topic);
  client.subscribe(time_interval_topic);
  client.subscribe(status_topic);

  for (int i=0; i<10; i++){
    toggle = !toggle;
    digitalWrite(D0, toggle);
    delay(100);  
  }
  digitalWrite(D0, BUILTIN_LED_OFF);

  client.publish(notification_topic, "rebooted");
}
 
void loop() {
  unsigned long currentMillis = millis();
  boolean isConnected;

  // check MQTT broker
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    isConnected = client.loop();
    if (!isConnected){
      softReset();
    }
  }   

  // setup pump
  if (pump_state == switched_on) {
        digitalWrite(D0, LOW);
    if (notofication_state == received_on){
      client.publish(notification_topic, "received_on");   
      notofication_state = in_progress;
      client.publish(notification_topic, "in_progress");
    }     
    if (currentMillis - pumpStateCheckStart >= pump_on_interval){
      pump_state = switched_off;
      digitalWrite(D0, BUILTIN_LED_OFF);
    }  
  } else if (pump_state == switched_off) {
      digitalWrite(D0, BUILTIN_LED_OFF);
      if (notofication_state == in_progress){
        client.publish(notification_topic, "finished");
        client.publish(switch_topic, "off");
        notofication_state = finished;
      }        
  }

}
