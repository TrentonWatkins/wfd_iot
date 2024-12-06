#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// WiFi Credentials
const char* ssid = "CyberSec";
const char* ssid_pass = "Cis401303";

// MQTT Broker Information
const char* broker = "192.168.8.210";
const uint16_t port = 1883;
const char* mqtt_username = "smartmqtt";
const char* mqtt_password = "HokieDVE";

// MQTT Topics
const char* topic_outtake_pump = "qual_chamber/outtake_pump";
const char* topic_ph = "qual_chamber/ph_sensor";
const char* topic_level = "qual_chamber/water_level";
const char* topic_temp = "qual_chamber/water_temp";

// GPIO Pins
#define phSensorPin A0 // Only ADC pin on ESP8266
#define levelSensor D1
#define LED D2
#define pumpPin D3
#define ground D4
#define waterTemp D5

// Variables
WiFiClient espClient;
PubSubClient mqttClient(espClient);
OneWire oneWire(waterTemp);
DallasTemperature sensors(&oneWire);
float temp;

// Function to calculate pH level
float calculatePh(float voltage) {
  return 7 + ((2.5 - voltage) / 0.1841);
}

// WiFi Connection
void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

// MQTT Connection
void connectMQTT() {
  mqttClient.setServer(broker, port);
  mqttClient.setCallback(onMqttMessage);

  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("ESP8266Client", mqtt_username, mqtt_password)) {
      Serial.println("connected!");
      mqttClient.subscribe(topic_outtake_pump);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Handle Incoming MQTT Messages
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Received message [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == topic_outtake_pump) {
    if (message == "Turn off") {
      digitalWrite(pumpPin, HIGH);
      Serial.println("Pump OFF");
    } else if (message == "Turn on") {
      digitalWrite(pumpPin, LOW);
      Serial.println("Pump ON");
    }
  }
}

void setup() {
  Serial.begin(115200);

  connectWiFi();

  pinMode(pumpPin, OUTPUT);
  pinMode(ground, OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(ground, LOW);
  digitalWrite(pumpPin, HIGH);

  sensors.begin();
  connectMQTT();
}

void loop() {
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
  if (temp != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature: ");
    Serial.println(temp);
    mqttClient.publish(topic_temp, String(temp).c_str());
  } else {
    Serial.println("Error reading temperature");
  }

  int waterLevelValue = analogRead(levelSensor);
  mqttClient.publish(topic_level, String(waterLevelValue).c_str());
  Serial.print("Water Level: ");
  Serial.println(waterLevelValue);

  int phRaw = analogRead(phSensorPin);
  float voltage = (phRaw / 1024.0) * 5.0;
  float phValue = calculatePh(voltage);
  mqttClient.publish(topic_ph, String(phValue).c_str());
  Serial.print("pH Level: ");
  Serial.println(phValue);

  delay(1000);
}

