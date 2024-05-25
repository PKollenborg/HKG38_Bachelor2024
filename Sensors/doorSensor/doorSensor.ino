#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

const char* ssid = SECRET_SSID;  // WiFi network name
const char* password = SECRET_PASS;  // WiFi password
const char* mqtt_server = "XXX.XXX.XXX.XXX";  // MQTT broker address ###replace with your own###
const int mqtt_port = 1883;  // MQTT broker port
const char* mqtt_topic = "my/topic";  // MQTT topic to publish data  ###replace with your own###

const int siteID = 456;  // Site identifier ###replace with your own desired ID###
const int deviceID = 1;  // Device identifier ###replace with your own desired ID###
const char* locationDescriptor = "Hallway";  // Location descriptor ###replace with your own desired ID###
const int Sensor = 12;  // Pin connected to the door sensor

WiFiClient espClient;
PubSubClient client(espClient);

bool doorState = false;  // false for OPEN, true for CLOSED
bool lastDoorState = false;  // Last stable door state

// Debounce variables
const unsigned long debounceDelay = 500;  // Debounce time in milliseconds
unsigned long lastDebounceTime = 0;
bool lastButtonState = HIGH;  // Previous state of the door sensor

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 180000; // 3 minutes

void setup() {
  Serial.begin(9600);
  pinMode(Sensor, INPUT_PULLUP); // Using internal pull-up resistor

  setup_wifi();  // Initialize WiFi connection
  client.setServer(mqtt_server, mqtt_port);  // Initialize MQTT client
}

void loop() {
  if (!client.connected()) {
    reconnect();  // Reconnect to MQTT broker if connection lost
  }
  client.loop();  // Maintain MQTT client connection

  // Read the door sensor with debounce
  bool currentState = debounceRead(Sensor);

  // Check if there's a change in status
  if (currentState != lastDoorState) {
    doorState = currentState;
    sendUpdate(); // Send update only when there's a change in status
  }

  // Check if it's time to send a current update
  if (millis() - lastUpdateTime >= updateInterval) {
    lastUpdateTime = millis();
    sendUpdate();
  }
}

bool debounceRead(int pin) {
  bool currentReading = digitalRead(pin);

  if (currentReading != lastButtonState) {
    // Reset the debounce timer
    lastDebounceTime = millis();
  }

  // Update the last button state
  lastButtonState = currentReading;

  // Check if enough time has passed to consider the new state stable
  if ((millis() - lastDebounceTime) > debounceDelay) {
    return currentReading;  // Return the stable state
  }

  return lastDoorState;  // Return the last stable state
}

void sendUpdate() {
  const char* sensorStatus = doorState ? "CLOSED" : "OPEN";

  // Create JSON object
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["siteID"] = siteID;
  jsonDoc["deviceID"] = deviceID;
  jsonDoc["locationDescriptor"] = locationDescriptor;
  jsonDoc["sensorStatus"] = sensorStatus;

  // Serialize JSON to string
  char jsonStr[200];
  serializeJson(jsonDoc, jsonStr);

  // Publish JSON string to MQTT topic
  if (client.connected()) {
    client.publish(mqtt_topic, jsonStr);

    // Print to serial monitor for debugging
    Serial.print("Published: ");
    Serial.println(jsonStr);

    lastDoorState = doorState; // Update lastDoorState
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Send initial status update upon connection
  sendUpdate();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      sendUpdate();  // Send initial status update upon reconnection
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
