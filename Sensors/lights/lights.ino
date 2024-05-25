#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "secrets.h"

// Define pins used
#define LED_PIN1 12
#define LED_PIN2 11
#define LED_PIN3 10
#define SWITCH_PIN1 2
#define SWITCH_PIN2 3

// MQTT and WiFi details
const char* mqtt_topic = "esp32/data";
const int mqtt_port = 1883;
const int siteID = 187;
const int deviceID = 4;
const char* locationDescriptor = "kjeller";
const char* clientId = "arduinoClient";

// WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Variables for LED and switch states
bool ledState = true;
bool switchState1 = false;
bool switchState2 = false;
bool lastSwitchState1 = false;
bool lastSwitchState2 = false;
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
const unsigned long debounceDelay = 50;

// JSON document for MQTT payload
StaticJsonDocument<200> jsonDocument;

// Function to set up WiFi connection
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
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT callback function
void callback(char* topic, byte* payload, unsigned int length) {
  // Handle message received from MQTT broker (if needed)
}

// Reconnect with MQTT broker function
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi(); // Initialize WiFi connection
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(LED_PIN3, OUTPUT);
  pinMode(SWITCH_PIN1, INPUT_PULLUP);
  pinMode(SWITCH_PIN2, INPUT_PULLUP);
  
  digitalWrite(LED_PIN1, ledState);
  digitalWrite(LED_PIN2, ledState);
  digitalWrite(LED_PIN3, ledState);

  // Set MQTT server and callback function
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect(); // Initialize reconnect function if not connected to MQTT broker
  }
  client.loop(); // Maintain MQTT client connection

  // Read switch states with debouncing
  bool reading1 = digitalRead(SWITCH_PIN1);
  bool reading2 = digitalRead(SWITCH_PIN2);

  if (reading1 != lastSwitchState1) {
    lastDebounceTime1 = millis();
  }
  if (reading2 != lastSwitchState2) {
    lastDebounceTime2 = millis();
  }

  if ((millis() - lastDebounceTime1) > debounceDelay) {
    if (reading1 != switchState1) {
      switchState1 = reading1;
      if (!switchState1) { // If switch 1 is pressed
        ledState = !ledState; // Toggle LED state
        digitalWrite(LED_PIN1, ledState);
        digitalWrite(LED_PIN2, ledState);
        digitalWrite(LED_PIN3, ledState);
        Serial.print("LED State: ");
        Serial.println(ledState ? "ON" : "OFF");

        // Clear previous JSON data
        jsonDocument.clear();

        // Data sent to JSON document
        jsonDocument["siteID"] = siteID;
        jsonDocument["deviceID"] = deviceID;
        jsonDocument["locationDescriptor"] = locationDescriptor;
        jsonDocument["switchUsed"] = "Switch1"; // Switch used
        jsonDocument["switchState"] = ledState ? "on" : "off"; // LED state

        // Convert JSON document to string
        String jsonString;
        serializeJson(jsonDocument, jsonString);

        // Publish JSON data to MQTT topic
        if (client.connected()) {
          client.publish(mqtt_topic, jsonString.c_str());
          Serial.println("Data published to MQTT");
        } else {
          Serial.println("Failed to publish data to MQTT");
        }
      }
    }
  }

  if ((millis() - lastDebounceTime2) > debounceDelay) {
    if (reading2 != switchState2) {
      switchState2 = reading2;
      if (!switchState2) { // If switch 2 is pressed
        ledState = !ledState; // Toggle LED state
        digitalWrite(LED_PIN1, ledState);
        digitalWrite(LED_PIN2, ledState);
        digitalWrite(LED_PIN3, ledState);
        Serial.print("LED State: ");
        Serial.println(ledState ? "ON" : "OFF");

        // Clear previous JSON data
        jsonDocument.clear();

        // Data sent to JSON document
        jsonDocument["siteID"] = siteID;
        jsonDocument["deviceID"] = deviceID;
        jsonDocument["locationDescriptor"] = locationDescriptor;
        jsonDocument["switchUsed"] = "Switch2"; // Switch used
        jsonDocument["switchState"] = ledState ? "on" : "off"; // LED state

        // Convert JSON document to string
        String jsonString;
        serializeJson(jsonDocument, jsonString);

        // Publish JSON data to MQTT topic
        if (client.connected()) {
          client.publish(mqtt_topic, jsonString.c_str());
          Serial.println("Data published to MQTT");
        } else {
          Serial.println("Failed to publish data to MQTT");
        }
      }
    }
  }

  lastSwitchState1 = reading1;
  lastSwitchState2 = reading2;
}
