#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// PINOUTS
#define INTAKE_PUMP_SIGNAL 13 // Intake pump control
#define OUTTAKE_PUMP_SIGNAL 12 // Outtake pump control
#define TEST_PIN 2            // General test pin for debugging

// WiFi Information
const char* ssid = "hydro";
const char* ssid_pass = "hydrohydro";

// Broker Information
String broker = "192.168.8.210"; // Default broker IP
String mqtt_username = "smartmqtt";
String mqtt_password = "HokieDVE";
String port = "1883";

// URL Information
const char* global_URL = "http://192.168.1.179:8080/api/collections/global/records/r1en4aa61ndcg6y";
const char* node_URL = "http://192.168.1.179:8080/api/collections/topics/records/";

// Topics
String topic_intake_pump = "prim_chamber/intake_pump";
String topic_outtake_pump = "acid_chamber/outtake_pump";
String topic_acid_pump = "acid_pump_00000";

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
  WiFi.begin(ssid, ssid_pass);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Configure pin modes
  pinMode(INTAKE_PUMP_SIGNAL, OUTPUT);
  pinMode(OUTTAKE_PUMP_SIGNAL, OUTPUT);
  pinMode(TEST_PIN, OUTPUT);

  // Default pin states
  digitalWrite(INTAKE_PUMP_SIGNAL, HIGH);
  digitalWrite(OUTTAKE_PUMP_SIGNAL, HIGH);
  digitalWrite(TEST_PIN, LOW);

  // Retrieve broker and topic information
  grabGlobalInformation();
  grabPumpInformation();

  // Print retrieved configurations
  Serial.print("Broker: "); Serial.println(broker);
  Serial.print("Port: "); Serial.println(port);

  // Connect to MQTT
  connectMQTT();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(1000);
}

// MQTT Connection Setup
void connectMQTT() {
  uint16_t converted_port = static_cast<uint16_t>(port.toInt());
  IPAddress ip;
  ip.fromString(broker);

  client.setServer(ip, converted_port);
  client.setCallback(callback);

  while (!client.connected()) {
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    Serial.println("Connecting to MQTT broker...");
    if (client.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
      Serial.println("Connected to broker");
      client.subscribe(topic_intake_pump.c_str());
      client.subscribe(topic_outtake_pump.c_str());
      client.subscribe(topic_acid_pump.c_str());
    } else {
      Serial.print("Failed to connect, state: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// MQTT Callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (strcmp(topic, topic_intake_pump.c_str()) == 0) {
    controlPump(INTAKE_PUMP_SIGNAL, message);
  } else if (strcmp(topic, topic_outtake_pump.c_str()) == 0) {
    controlPump(OUTTAKE_PUMP_SIGNAL, message);
  } else if (strcmp(topic, topic_acid_pump.c_str()) == 0) {
    controlPump(TEST_PIN, message);
  }
}

// Helper function to control pumps
void controlPump(int pin, String message) {
  if (message == "Turn on" || message == "on") {
    digitalWrite(pin, LOW);
    Serial.println("Pump ON");
  } else if (message == "Turn off" || message == "off") {
    digitalWrite(pin, HIGH);
    Serial.println("Pump OFF");
  }
}

// Retrieve Global Information
void grabGlobalInformation() {
  HTTPClient http;
  http.begin(global_URL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    broker = String(doc["broker"]);
    mqtt_username = String(doc["username"]);
    mqtt_password = String(doc["password"]);
    port = String(doc["port"]);
  } else {
    Serial.print("Failed to retrieve global info, error code: ");
    Serial.println(httpCode);
  }
  http.end();
}

// Retrieve Pump Information
void grabPumpInformation() {
  HTTPClient http;
  String url = String(node_URL) + topic_acid_pump;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    topic_acid_pump = String(doc["topic"]);
  } else {
    Serial.print("Failed to retrieve pump info, error code: ");
    Serial.println(httpCode);
  }
  http.end();
}

// Reconnect to MQTT
void reconnect() {
  while (!client.connected()) {
    connectMQTT();
  }
}
