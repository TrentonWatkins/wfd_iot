#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// PINOUTS
#define PUMP_SIGNAL_PIN 13
#define LED_PIN 2

// WIFI Information
const char* ssid = "hydro";         // Unified WIFI SSID
const char* ssid_pass = "hydrohydro"; // Unified WIFI Password

// Broker Information
String broker = "192.168.8.210";    // Unified Broker IP Address
String mqtt_username = "smartmqtt"; // Unified Broker Username
String mqtt_password = "HokieDVE";  // Unified Broker Password
String port = "1883";               // Unified Broker Port

// Node Topics
String topic_pump = "basic_pump_control";        // Pump control topic
String topic_outtake_pump = "base_chamber/outtake_pump"; // Outtake pump topic

// URL Information
const char* global_URL = "http://192.168.1.179:8080/api/collections/global/records/r1en4aa61ndcg6y";
const char* node_URL = "http://192.168.1.179:8080/api/collections/topics/records/";
const char* endpoints[] = {"basic_pump_0000"};

// MQTT and WiFi Clients
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // WiFi Setup
  WiFi.begin(ssid, ssid_pass);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Pin Setup
  pinMode(PUMP_SIGNAL_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(PUMP_SIGNAL_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);

  // Fetch Configuration
  grabGlobalInformation();
  grabPumpInformation();

  // MQTT Setup
  connectMQTT();
}

void loop() {
  if (!client.connected()) {
    digitalWrite(LED_PIN, HIGH);
    reconnect();
    digitalWrite(LED_PIN, LOW);
  }
  client.loop();
  delay(1000);
}

// MQTT Callback
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(message);

  if (String(topic) == topic_pump) {
    if (message == "on") {
      digitalWrite(PUMP_SIGNAL_PIN, LOW);
      Serial.println("Pump ON");
    } else if (message == "off") {
      digitalWrite(PUMP_SIGNAL_PIN, HIGH);
      Serial.println("Pump OFF");
    }
  } else if (String(topic) == topic_outtake_pump) {
    if (message == "Turn on") {
      digitalWrite(PUMP_SIGNAL_PIN, LOW);
      Serial.println("Outtake Pump ON");
    } else if (message == "Turn off") {
      digitalWrite(PUMP_SIGNAL_PIN, HIGH);
      Serial.println("Outtake Pump OFF");
    }
  }
}

// MQTT Connection
void connectMQTT() {
  client.setServer(broker.c_str(), port.toInt());
  client.setCallback(callback);

  reconnect();
}

// MQTT Reconnect
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_username.c_str(), mqtt_password.c_str())) {
      Serial.println("Connected to MQTT Broker!");
      client.subscribe(topic_pump.c_str());
      client.subscribe(topic_outtake_pump.c_str());
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// Fetch Pump Information
void grabPumpInformation() {
  HTTPClient http;
  char fullURL[87];
  strcpy(fullURL, node_URL);
  strcat(fullURL, endpoints[0]);

  http.begin(fullURL);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    topic_pump = String(doc["topic"]);
    Serial.println("Updated Pump Topic: " + topic_pump);
  } else {
    Serial.println("Failed to fetch pump information");
  }
  http.end();
}

// Fetch Global Information
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
    Serial.println("Fetched Global Information:");
    Serial.println("Broker: " + broker);
    Serial.println("Username: " + mqtt_username);
    Serial.println("Password: " + mqtt_password);
    Serial.println("Port: " + port);
  } else {
    Serial.println("Failed to fetch global information");
  }
  http.end();
}
