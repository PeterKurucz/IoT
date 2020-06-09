#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <main.h>

unsigned long previousMillis = 0;
unsigned long interval = 1000;

unsigned long pumpStateCheckStart = 0;
unsigned long pump_on_interval = 10000;
boolean toggle = false;

enum PUMP_STATE {
  unknown,
  switched_on,
  switched_off
};

enum PUMP_NOTIFICATION_STATE {
  undefined,
  received_on,
  received_off,
  in_progress,
  finished,
  failed
};

PUMP_STATE pump_state;
PUMP_NOTIFICATION_STATE notofication_state;

WiFiClient espClient;
PubSubClient client(espClient);
 
void callback(char* topic, byte* payload, unsigned int length) {
  char payload_as_char[length];
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payload_as_char[i] = (char)payload[i];
  }

  payload_as_char[length] = '\0';

  if (strcmp(payload_as_char, "on") == 0){
//    digitalWrite(LED_BUILTIN, LOW);    
    pump_state = switched_on;
    notofication_state = received_on;
    pumpStateCheckStart = millis();
  } else if (strcmp(payload_as_char, "off") == 0){
//    digitalWrite(LED_BUILTIN, HIGH);
    pump_state = switched_off;
    pumpStateCheckStart = millis();
  }

  if (strcmp(topic, time_interval_topic) == 0) {
    pump_on_interval = atol(payload_as_char) * 1000 * 60;
    Serial.println("interval:");
    Serial.print(pump_on_interval);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}
 
void setup() {

  pump_state = unknown;
  notofication_state = undefined;
  pinMode(D0, OUTPUT);
  digitalWrite(D0, HIGH);
  
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    toggle = !toggle;
    digitalWrite(D0, toggle);
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  toggle = BUILTIN_LED_OFF;
  digitalWrite(D0, toggle);

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
      Serial.println("connected");  
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      toggle = !toggle;
      digitalWrite(D0, toggle);
      delay(2000);
    }
  }
 
  toggle = BUILTIN_LED_OFF;
  digitalWrite(D0, toggle);

  client.subscribe(switch_topic);
  client.subscribe(time_interval_topic);

  digitalWrite(D0, BUILTIN_LED_ON);
  delay(250);
  digitalWrite(D0, BUILTIN_LED_OFF);
  delay(250);
  digitalWrite(D0, BUILTIN_LED_ON);
  delay(250);
  digitalWrite(D0, BUILTIN_LED_OFF);
  delay(250);
  digitalWrite(D0, BUILTIN_LED_ON);
  delay(250);
  digitalWrite(D0, BUILTIN_LED_OFF);
 
}
 
void loop() {
  //  delay(5000);
  unsigned long currentMillis = millis();

  // check MQTT broker
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    client.loop();
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
      digitalWrite(D0, HIGH);
    }  
 } else if (pump_state == switched_off) {
      digitalWrite(D0, HIGH);
      if (notofication_state == in_progress){
        client.publish(notification_topic, "finished");
        client.publish(switch_topic, "off");
        notofication_state = finished;
      }        
 }

}
