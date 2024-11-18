#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <ArduinoMqttClient.h>

// WiFi and MQTT information
const char* ssid = "Testbed-W";
const char* ssid_pass = "HokieDVE";
const char* broker = "192.168.8.210";
const char* mqtt_username = "smartmqtt";
const char* mqtt_password = "HokieDVE";
const int mqtt_port = 1883;

// WiFi and MQTT clients
WiFiClient wifiClient;
HttpClient httpClient(wifiClient, "192.168.8.210", 8080);
PubSubClient client(wifiClient);    // For PubSubClient
MqttClient mqttClient(wifiClient);  // For ArduinoMqttClient

// Topics
const char* topic_outtake_pump = "chlo_chamber/outtake_pump";  
const char* topic_ph = "chlo_chamber/ph_sensor";  
const char* topic_level = "chlo_chamber/water_level";  

// Pins
#define phSensorPin A0
#define levelSensor A1
#define LED 13
#define pumpPin 0
#define ground 7
#define GND_PIN 2 

// Variables for pH calculation and delays
int samples = 10;
float adc_resolution = 1024.0;
unsigned long lastMqttPublishTimePH = 0;
unsigned long lastMqttPublishTimeWater = 0;

// Converts voltage to pH value
float ph(float voltage) {
    return 7 + ((2.5 - voltage) / 0.1841);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing...");

  // Connect to Wi-Fi
  WiFi.begin(ssid, ssid_pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize pins
  pinMode(pumpPin, OUTPUT);
  pinMode(ground, OUTPUT);
  digitalWrite(ground, LOW);
  pinMode(GND_PIN, OUTPUT);
  digitalWrite(GND_PIN, LOW); 
  pinMode(LED, OUTPUT);
  digitalWrite(pumpPin, HIGH);  // Default pump state: off

  // Configure PubSubClient and ArduinoMqttClient
  client.setServer(broker, mqtt_port);
  client.setCallback(pubSubCallback);

  mqttClient.begin(broker, mqtt_port, wifiClient);
  mqttClient.setUsernamePassword(mqtt_username, mqtt_password);
  mqttClient.setId("arduino-client");
  mqttClient.onMessage(arduinoMqttCallback);
}

void loop() {
  // Ensure MQTT connection
  if (!client.connected()) {
    reconnectPubSub();
  }
  client.loop();

  // Read water level and pH sensor
  int water_level_value = analogRead(levelSensor);
  int measurings = 0;
  for (int i = 0; i < samples; i++) {
    measurings += analogRead(phSensorPin);
    delay(10);
  }

  // Publish pH level every second
  if (millis() - lastMqttPublishTimePH > 1000U) {
    lastMqttPublishTimePH = millis();
    float voltage = (5.0 / adc_resolution) * (measurings / samples);
    float ph_level_value = ph(voltage);
    String ph_value_str = String(ph_level_value, 2);
    client.publish(topic_ph, ph_value_str.c_str());
    Serial.print("PhValue: ");
    Serial.println(ph_value_str);
  }

  // Publish water level every 2 seconds
  if (millis() - lastMqttPublishTimeWater > 2000U) {
    lastMqttPublishTimeWater = millis();
    String water_level_str = String(water_level_value);
    client.publish(topic_level, water_level_str.c_str());
    Serial.print("Water level: ");
    Serial.println(water_level_str);
  }

  delay(1000); // General delay for loop
}

/*
  Reconnect PubSubClient
*/
void reconnectPubSub() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(topic_outtake_pump);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

/*
  PubSubClient callback
*/
void pubSubCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i]; 
  }

  if (strcmp(topic, topic_outtake_pump) == 0) {
    if (message == "Turn on") {
        digitalWrite(pumpPin, LOW);
        Serial.println("Pump ON");
    } else if (message == "Turn off") {
        digitalWrite(pumpPin, HIGH);
        Serial.println("Pump OFF");
    }
  }
}

/*
  ArduinoMqttClient callback
*/
void arduinoMqttCallback(int messageSize) {
  String topic = mqttClient.messageTopic();
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  String payload = "";
  while (mqttClient.available()) {
    char c = mqttClient.read();
    payload += c;
  }
  Serial.println("Payload: " + payload);

  if (topic == topic_outtake_pump) {
    if (payload == "Turn on") {
      digitalWrite(pumpPin, LOW);
      Serial.println("Pump ON");
    } else if (payload == "Turn off") {
      digitalWrite(pumpPin, HIGH);
      Serial.println("Pump OFF");
    }
  }
}
