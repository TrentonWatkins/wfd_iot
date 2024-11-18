#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>

// WiFi and MQTT Configuration
WiFiClient wifiClient;
HttpClient httpClient(wifiClient, "192.168.1.179", 8080);

const char* ssid = "hydro";
const char* ssid_pass = "hydrohydro";
const char* mqtt_username = "smartmqtt"; // Replace with the actual username
const char* mqtt_password = "HokieDVE"; // Replace with the actual password
const char* broker = "192.168.1.179";
const int mqtt_port = 1883;
PubSubClient mqttClient(wifiClient);

// MQTT Topics
String topic_level, topic_pump, topic_ph;
const char* default_level_topic = "default/level";
const char* default_pump_topic = "default/pump";
const char* default_ph_topic = "default/ph";

// Arduino Pins
#define phSensorPin A0
#define levelSensorPin A1
#define pumpPin 20
#define LED 13
#define ground 7
#define GND_PIN 2

// Sensor and Conversion Constants
int samples = 10;
float adc_resolution = 1024.0;

// Timing for MQTT publishing
unsigned long lastPublishTimePH = 0;
unsigned long lastPublishTimeLevel = 0;

float calculatePH(float voltage) {
    return 7 + ((2.5 - voltage) / 0.18);
}

void setup() {
    Serial.begin(9600);
    Serial.println("Serial initialized");

    // WiFi connection
    WiFi.begin(ssid, ssid_pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Pin initialization
    pinMode(pumpPin, OUTPUT);
    pinMode(ground, OUTPUT);
    digitalWrite(ground, LOW);
    pinMode(GND_PIN, OUTPUT);
    digitalWrite(GND_PIN, LOW);
    pinMode(LED, OUTPUT);
    digitalWrite(pumpPin, LOW); // Default pump off

    // MQTT setup
    mqttClient.setServer(broker, mqtt_port);
    mqttClient.setCallback(mqttCallback);

    // Retrieve global settings and topics
    getGlobal();
    getTopics();
}

void loop() {
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();

    unsigned long currentMillis = millis();

    // Publish pH value every 1 second
    if (currentMillis - lastPublishTimePH > 1000U) {
        lastPublishTimePH = currentMillis;
        publishPH();
    }

    // Publish water level every 2 seconds
    if (currentMillis - lastPublishTimeLevel > 2000U) {
        lastPublishTimeLevel = currentMillis;
        publishWaterLevel();
    }
}

// Retrieve global configuration
void getGlobal() {
    httpClient.beginRequest();
    httpClient.get("/api/collections/global/records/r1en4aa61ndcg6y");
    httpClient.sendHeader("Content-Type", "application/json");
    httpClient.endRequest();

    int status = httpClient.responseStatusCode();
    if (status != 200) {
        Serial.println("Failed to fetch global configuration");
        return;
    }

    String responseBody = httpClient.responseBody();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, responseBody);

    if (error) {
        Serial.println("Failed to parse global configuration JSON");
        return;
    }

    topic_level = doc["topic_level"] | default_level_topic;
    topic_pump = doc["topic_pump"] | default_pump_topic;
    topic_ph = doc["topic_ph"] | default_ph_topic;
}

void getTopics() {
    // Similar HTTP calls to retrieve topic-specific configurations
    // Can be adapted based on the individual API endpoints
}

// Reconnect to MQTT
void reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (mqttClient.connect("arduino-client", mqtt_username, mqtt_password)) {
            Serial.println("connected");
            mqttClient.subscribe(topic_pump.c_str());
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

// Callback for MQTT messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    if (String(topic) == topic_pump) {
        if (message == "on") {
            digitalWrite(pumpPin, HIGH);
            Serial.println("Pump ON");
        } else if (message == "off") {
            digitalWrite(pumpPin, LOW);
            Serial.println("Pump OFF");
        }
    }
}

// Publish pH data to MQTT
void publishPH() {
    int totalMeasurings = 0;
    for (int i = 0; i < samples; i++) {
        totalMeasurings += analogRead(phSensorPin);
        delay(10);
    }

    float voltage = (5.0 / adc_resolution) * (totalMeasurings / samples);
    float phValue = calculatePH(voltage);
    String phStr = String(phValue, 2);

    mqttClient.publish(topic_ph.c_str(), phStr.c_str());
    Serial.print("pH Value: ");
    Serial.println(phStr);
}

// Publish water level data to MQTT
void publishWaterLevel() {
    int waterLevel = analogRead(levelSensorPin);
    String waterLevelStr = String(waterLevel);

    mqttClient.publish(topic_level.c_str(), waterLevelStr.c_str());
    Serial.print("Water Level: ");
    Serial.println(waterLevelStr);
}
