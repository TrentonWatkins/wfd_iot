#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO Definitions
#define PIN_PUMP 13
#define PIN_LED 2
#define PIN_LEVEL A0
#define PIN_TEMP 13  // For Dallas Temperature sensor
#define PIN_PH_SENSOR A1

// WiFi Credentials
const char *ssid = "hydro";
const char *password = "hydrohydro";

// MQTT Broker Info
const char *mqtt_broker = "192.168.1.179";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

// MQTT Topics
const char *topic_pump = "quality/pump";
const char *topic_level = "quality/level";
const char *topic_temp = "quality/temp";
const char *topic_ph = "quality/ph";

// Temperature Sensor Setup
OneWire oneWire(PIN_TEMP);
DallasTemperature sensors(&oneWire);

// Variables for Readings
int level_value = 0;
float temp_value = 0.0;
float ph_value = 0.0;
unsigned long lastMillis = 0;
const long publishInterval = 10000;

// WiFi and MQTT Setup
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
    // Serial and GPIO Setup
    Serial.begin(115200);
    pinMode(PIN_PUMP, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_PH_SENSOR, INPUT);
    pinMode(PIN_LEVEL, INPUT);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Connect to MQTT Broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    connectMQTT();

    // Initialize Sensors
    sensors.begin();
}

void loop() {
    // Ensure MQTT Connection
    if (!client.connected()) {
        connectMQTT();
    }
    client.loop();

    // Publish Sensor Readings Periodically
    unsigned long currentMillis = millis();
    if (currentMillis - lastMillis >= publishInterval) {
        lastMillis = currentMillis;

        // Read and Publish Temperature
        sensors.requestTemperatures();
        temp_value = sensors.getTempFByIndex(0);
        char tempBuffer[10];
        dtostrf(temp_value, 1, 2, tempBuffer);
        client.publish(topic_temp, tempBuffer);

        // Read and Publish Water Level
        level_value = analogRead(PIN_LEVEL);
        char levelBuffer[10];
        itoa(level_value, levelBuffer, 10);
        client.publish(topic_level, levelBuffer);

        // Read and Publish pH Level
        ph_value = readPH();
        char phBuffer[10];
        dtostrf(ph_value, 1, 2, phBuffer);
        client.publish(topic_ph, phBuffer);

        Serial.println("Published Sensor Data");
    }
}

void callback(char *topic, byte *payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    Serial.println(message);

    // Handle Pump Control
    if (String(topic) == topic_pump) {
        if (message == "on") {
            digitalWrite(PIN_PUMP, HIGH);
            digitalWrite(PIN_LED, HIGH);
        } else if (message == "off") {
            digitalWrite(PIN_PUMP, LOW);
            digitalWrite(PIN_LED, LOW);
        }
    }
}

void connectMQTT() {
    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect("ESP8266Client", mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT");
            client.subscribe(topic_pump);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            delay(2000);
        }
    }
}

float readPH() {
    const int samples = 10;
    float adc_resolution = 1024.0;
    int ph_adc = 0;
    for (int i = 0; i < samples; i++) {
        ph_adc += analogRead(PIN_PH_SENSOR);
        delay(10);
    }
    float voltage = (5.0 / adc_resolution) * (ph_adc / samples);
    return 7.0 + ((2.5 - voltage) / 0.1841);  // Calibrated pH formula
}
