#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// PINOUTS
#define PUMP_PIN 12 // For controlling the pump
#define TEST_PIN 2  // Test pin (optional)
#define TDS_PIN A0  // TDS Sensor input pin
#define VREF 3.3    // ADC Reference Voltage
#define SCOUNT 30   // Number of samples for averaging

// WiFi Credentials
const char* ssid = "hydro";
const char* ssid_pass = "hydrohydro";

// URLs for dynamic configuration
const char* global_URL = "http://192.168.1.179:8080/api/collections/global/records/r1en4aa61ndcg6y";
const char* node_URL = "http://192.168.1.179:8080/api/collections/topics/records/";
const char* endpoints[] = {"grit_pump_00000", "grit_tds_000000"};

// MQTT Variables
WiFiClient espClient;
PubSubClient client(espClient);
String broker;
String username;
String password;
String port;
String topic_tds;
String topic_pump;

// TDS Variables
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
float averageVoltage = 0.0;
float tdsValue = 0.0;
float temperature = 23.0; // Default temperature for compensation

// Function Prototypes
void grabGlobalInformation();
void grabNodeInformation();
void connectMQTT();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
int getMedianNum(int bArray[], int iFilterLen);

void setup() {
  Serial.begin(115200);

  // Initialize Wi-Fi
  WiFi.begin(ssid, ssid_pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Initialize Pins
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(TEST_PIN, OUTPUT);
  pinMode(TDS_PIN, INPUT);

  // Fetch dynamic configuration
  grabGlobalInformation();
  grabNodeInformation();

  // Connect to MQTT Broker
  connectMQTT();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // TDS Measurement
  static unsigned long sampleTime = millis();
  if (millis() - sampleTime > 40) {
    sampleTime = millis();
    analogBuffer[analogBufferIndex++] = analogRead(TDS_PIN);
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }

  // TDS Calculation and Publishing
  static unsigned long printTime = millis();
  if (millis() - printTime > 800) {
    printTime = millis();
    for (int i = 0; i < SCOUNT; i++) {
      analogBufferTemp[i] = analogBuffer[i];
    }
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;
    float compensationVoltage = averageVoltage / (1.0 + 0.02 * (temperature - 25.0));
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage -
                255.86 * compensationVoltage * compensationVoltage +
                857.39 * compensationVoltage) * 0.5;

    Serial.printf("TDS Value: %.2f ppm\n", tdsValue);
    String tdsStr = String(tdsValue);
    client.publish(topic_tds.c_str(), tdsStr.c_str());
  }
}

// Callback for MQTT Messages
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  if (String(topic) == topic_pump) {
    if (message == "on") {
      digitalWrite(PUMP_PIN, HIGH);
    } else if (message == "off") {
      digitalWrite(PUMP_PIN, LOW);
    }
  }
}

// Reconnect to MQTT Broker
void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP8266Client", username.c_str(), password.c_str())) {
      client.subscribe(topic_pump.c_str());
      client.subscribe(topic_tds.c_str());
    } else {
      delay(5000);
    }
  }
}

// Fetch Global Configuration
void grabGlobalInformation() {
  HTTPClient http;
  http.begin(global_URL);
  int httpCode = http.GET();
  if (httpCode > 0) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());
    broker = doc["broker"].as<String>();
    username = doc["username"].as<String>();
    password = doc["password"].as<String>();
    port = doc["port"].as<String>();
  }
  http.end();
}

// Fetch Node-Specific Configuration
void grabNodeInformation() {
  HTTPClient http;
  String url = String(node_URL) + endpoints[0];
  http.begin(url);
  if (http.GET() > 0) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());
    topic_pump = doc["topic"].as<String>();
  }
  url = String(node_URL) + endpoints[1];
  http.begin(url);
  if (http.GET() > 0) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());
    topic_tds = doc["topic"].as<String>();
  }
  http.end();
}

// Get Median for TDS Filtering
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (int i = 0; i < iFilterLen; i++) {
    bTab[i] = bArray[i];
  }
  for (int j = 0; j < iFilterLen - 1; j++) {
    for (int i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        int temp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = temp;
      }
    }
  }
  return (iFilterLen & 1) ? bTab[iFilterLen / 2] : (bTab[iFilterLen / 2 - 1] + bTab[iFilterLen / 2]) / 2;
}
