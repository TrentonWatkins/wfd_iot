#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Sensor and Motor Ports
#define WATER_LEVEL_SIGNAL A0
#define INTAKE_PUMP_SIGNAL 13
#define OUTTAKE_PUMP_SIGNAL 12
#define BUILTIN_LED 2

// Broker Information
String broker = "192.168.8.210";
String mqtt_username = "smartmqtt";
String mqtt_password = "HokieDVE";
String port = "1883";

// Node Information
String topic_intake_pump = "prim_chamber/intake_pump";
String topic_outtake_pump = "prim_chamber/outtake_pump";
String topic_pump = "pump_control/primary";

// Wi-Fi Information
const char* ssid = "Testbed-W";
const char* ssid_pass = "HokieDVE";

// HTTP URL Information
const char* global_URL = "http://192.168.1.179:8080/api/collections/global/records/r1en4aa61ndcg6y";
const char* node_URL = "http://192.168.1.179:8080/api/collections/topics/records/";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to Wi-Fi
  WiFi.begin(ssid, ssid_pass);
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected to Wi-Fi!");

  // Initialize GPIOs
  pinMode(INTAKE_PUMP_SIGNAL, OUTPUT);
  pinMode(OUTTAKE_PUMP_SIGNAL, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(INTAKE_PUMP_SIGNAL, HIGH);
  digitalWrite(OUTTAKE_PUMP_SIGNAL, HIGH);
  digitalWrite(BUILTIN_LED, LOW);

  grabGlobalInformation();
  connectMQTT();
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Publish water level readings
  int value = analogRead(WATER_LEVEL_SIGNAL);
  String water_level = String(value);
  client.publish("prim_chamber/level", water_level.c_str());
  delay(1000);
}

void connectMQTT() {
  uint16_t converted_port = static_cast<uint16_t>(port.toInt());
  IPAddress ip;
  ip.fromString(broker);

  client.setServer(ip, converted_port);
  client.setCallback(callback);

  reconnectMQTT();
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
      Serial.println("Connected!");
      client.subscribe(topic_intake_pump.c_str());
      client.subscribe(topic_outtake_pump.c_str());
      client.subscribe(topic_pump.c_str());
    } else {
      Serial.print("Failed, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (strcmp(topic, topic_intake_pump.c_str()) == 0) {
    handlePumpControl(INTAKE_PUMP_SIGNAL, message);
  } else if (strcmp(topic, topic_outtake_pump.c_str()) == 0) {
    handlePumpControl(OUTTAKE_PUMP_SIGNAL, message);
  } else if (strcmp(topic, topic_pump.c_str()) == 0) {
    handlePumpControl(INTAKE_PUMP_SIGNAL, message); // Example behavior for `topic_pump`
  }
}

void handlePumpControl(int pin, String message) {
  if (message == "Turn on") {
    digitalWrite(pin, LOW);
    Serial.println("Pump ON");
  } else if (message == "Turn off") {
    digitalWrite(pin, HIGH);
    Serial.println("Pump OFF");
  }
}

void grabGlobalInformation() {
  HTTPClient http;
  http.begin(global_URL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    broker = String(doc["broker"].as<const char*>());
    mqtt_username = String(doc["username"].as<const char*>());
    mqtt_password = String(doc["password"].as<const char*>());
    port = String(doc["port"].as<const char*>());
  } else {
    Serial.print("HTTP GET failed: ");
    Serial.println(httpCode);
  }
  http.end();
}
