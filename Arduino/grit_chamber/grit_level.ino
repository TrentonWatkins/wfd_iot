#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// PINOUTS
#define SIGNAL_PIN A0
#define BUILTIN_LED 2 // On-board LED pin

int value = 0;  // variable to store the sensor value

// WIFI Information
const char* ssid = "hydro";       // Replace as needed
const char* ssid_pass = "hydrohydro"; // Replace as needed

// Broker Information
String broker;
String username;
String password;
String port;

// Node information
String topic_level;

// URL information
const char* global_URL = "http://192.168.1.179:8080/api/collections/global/records/r1en4aa61ndcg6y";
const char* node_URL = "http://192.168.1.179:8080/api/collections/topics/records/";

// MQTT connection
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, ssid_pass);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  pinMode(SIGNAL_PIN, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);  // Turn off LED initially

  grabGlobalInformation();
  grabLevelInformation();

  Serial.print("Broker: ");
  Serial.println(broker);
  Serial.print("Port: ");
  Serial.println(port);

  reconnect();  // Connect to MQTT
}

void loop() {
  if (!client.connected()) {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn on LED to indicate reconnect
    reconnect();
    digitalWrite(BUILTIN_LED, LOW);   // Turn off LED when reconnected
  }
  
  value = analogRead(SIGNAL_PIN);

  Serial.print("Sensor value: ");
  Serial.println(value);

  char str[20];
  sprintf(str, "%d", value);

  const char* topic = topic_level.c_str();
  client.publish(topic, str);

  Serial.printf("Published %s to %s\n", str, topic);
  client.loop();
  delay(1000);
}

void reconnect() {
  uint16_t converted_port = static_cast<uint16_t>(port.toInt());
  IPAddress ip = IPAddress();
  ip.fromString(broker);

  client.setServer(ip, converted_port);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "esp8266-client-level";
    if (client.connect(clientId.c_str(), username.c_str(), password.c_str())) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(topic_level.c_str());
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message: ");
  Serial.println(message);
  Serial.println("-----------------------");
}

void grabGlobalInformation() {
  HTTPClient http;
  WiFiClient client;
  http.begin(client, global_URL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    broker = doc["broker"].as<String>();
    username = doc["username"].as<String>();
    password = doc["password"].as<String>();
    port = doc["port"].as<String>();
  } else {
    Serial.print("HTTP request failed with error code ");
    Serial.println(httpCode);
  }
  http.end();
}

void grabLevelInformation() {
  HTTPClient http;
  WiFiClient client;
  char fullURL[87];
  strcpy(fullURL, node_URL);
  strcat(fullURL, "grit_level_0000");

  http.begin(client, fullURL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    topic_level = doc["topic"].as<String>();
  } else {
    Serial.print("HTTP request failed with error code ");
    Serial.println(httpCode);
  }
  http.end();
}
