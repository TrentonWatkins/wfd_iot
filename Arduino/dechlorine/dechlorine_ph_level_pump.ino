// Dependencies
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi Information
const char* ssid = "CyberSec";
const char* ssid_pass = "Cis401303";
const char* broker = "192.168.8.210";
const char* mqtt_username = "smartmqtt";
const char* mqtt_password = "HokieDVE";
const int mqtt_port = 1883;

// MQTT Topics
const char* topic_outtake_pump = "dech_chamber/outtake_pump";
const char* topic_ph = "dech_chamber/ph_sensor";
const char* topic_level = "dech_chamber/water_level";

// NodeMCU Pin Definitions
#define phSensorPin A0
#define levelSensor A0  // ESP8266 has only one ADC pin
#define LED 13         // Use built-in LED
#define pumpPin 20      // Reassign to valid GPIO
#define ground 7       // Reassign to valid GPIO

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMqttPublishTimePH = 0;
unsigned long lastMqttPublishTimeWater = 0;

// ADC resolution for ESP8266 (default 1V reference)
float adc_resolution = 1024.0;

// Converts voltage value to readable pH value
float ph(float voltage) {
    return 7 + ((2.5 - voltage) / 0.1841);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    // Connect to Wi-Fi
    WiFi.begin(ssid, ssid_pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Configure Pins
    pinMode(pumpPin, OUTPUT);
    pinMode(ground, OUTPUT);
    digitalWrite(ground, LOW);
    pinMode(LED, OUTPUT);
    digitalWrite(pumpPin, HIGH); // Default pump state is OFF

    // MQTT setup
    client.setServer(broker, mqtt_port);
    client.setCallback(callback);
}

void loop() {
    // Ensure MQTT connection
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Read pH sensor
    int measurings = 0;
    for (int i = 0; i < 10; i++) {
        measurings += analogRead(phSensorPin);
        delay(10);
    }

    // Publish pH value
    if (millis() - lastMqttPublishTimePH > 1000U) {
        lastMqttPublishTimePH = millis();
        float voltage = (1.0 / adc_resolution) * (measurings / 10); // Adjust for 1.0V reference
        float ph_level_value = ph(voltage);
        String ph_value_str = String(ph_level_value, 2);
        client.publish(topic_ph, ph_value_str.c_str());
        Serial.print("PhValue: ");
        Serial.println(ph_value_str);
    }

    // Publish water level
    if (millis() - lastMqttPublishTimeWater > 2000U) {
        lastMqttPublishTimeWater = millis();
        String water_level_str = String(analogRead(levelSensor));
        client.publish(topic_level, water_level_str.c_str());
        Serial.print("Water level: ");
        Serial.println(water_level_str);
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP8266Client-" + String(random(0xffff), HEX);
        if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT");
            client.subscribe(topic_outtake_pump);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(". Retrying in 5 seconds...");
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
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
            digitalWrite(pumpPin, LOW); // Activate pump
            Serial.println("Pump ON");
        } else if (message == "Turn off") {
            digitalWrite(pumpPin, HIGH); // Deactivate pump
            Serial.println("Pump OFF");
        }
    }
}
