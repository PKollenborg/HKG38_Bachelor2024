#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "secrets.h"



// Pin definitions
int pirPin = 7; // PIR sensor pin
int ledPin = 5; // LED pin
int buzzerPin = 6; // Buzzer pin
int greenButtonPin = 8; // Button to arm the system
int redButtonPin = 9; // Button to disarm the system

// State variables
bool sensorEnabled = false;
bool alarmActive = false;
bool arming = false;
bool lastMotionState = false;

// Timing constants
unsigned long armTime = 0;
unsigned long lastMotionTime = 0; // Time of the last motion event
unsigned long lastButtonPress = 0; // Time of the last button press
unsigned long lastPrintTime = 0; // Track the last print time during arming for periodic MQTT updates
const int armDuration = 60000; // 90 seconds for arming
const int printInterval = 30000; // 30 seconds print interval
const int motionDebounce = 1000; // Debounce time for motion detection
const int buttonDebounce = 500; // Debounce time for button presses

// MQTT configuration
const char* mqtt_topic = "my/topic"; 
const int mqtt_port = 1883;
const int siteID = 1385; // site identifier ###replace with your own desired ID###
const int deviceID = 7; // Device identifier ###replace with your own desired ID###
const char* locationDescriptor = "door"; // Location descriptor ###replace with your own desired ID###


WiFiClient espClient;
PubSubClient client(espClient);
StaticJsonDocument<100> jsonDocument;

void setup_wifi() {
  Serial.begin(9600);
  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  setup_wifi();
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(greenButtonPin, INPUT_PULLUP);
  pinMode(redButtonPin, INPUT_PULLUP);
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);
  client.setServer(mqtt_server, mqtt_port);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("arduinoClient")) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void publishStatus(const char* status) {
  jsonDocument.clear();
  jsonDocument["siteID"] = siteID;
  jsonDocument["deviceID"] = deviceID;
  jsonDocument["locationDescriptor"] = locationDescriptor;
  jsonDocument["status"] = status;

  String jsonMessage;
  serializeJson(jsonDocument, jsonMessage);
  client.publish(mqtt_topic, jsonMessage.c_str());
  Serial.print("Published MQTT message: ");
  Serial.println(jsonMessage);
}

void playArmingTone() {
    tone(buzzerPin, 880, 200); // Play a tone when arming starts
}

void playArmedTone() {
    tone(buzzerPin, 1046, 200); // C6 tone to indicate the system is fully armed
}

void playDisarmTone() {
    tone(buzzerPin, 1046, 100); // C6
    delay(150);
    tone(buzzerPin, 988, 100);  // B5
    delay(150);
    tone(buzzerPin, 880, 100);  // A5
    delay(150);
    noTone(buzzerPin); // Stop any tone being played
}

void flashLEDGradually(int startTime) {
  unsigned long elapsed = millis() - startTime;
  int interval = max(50, 250 - (250 * elapsed / armDuration)); // Gradually decrease interval
  digitalWrite(ledPin, HIGH);
  delay(interval);
  digitalWrite(ledPin, LOW);
  delay(interval);
}


void playAlarmTone() {
    unsigned long startMillis = millis();
    unsigned long currentMillis = startMillis;

    // Play the sweeping tone for 5 seconds
    while (currentMillis - startMillis < 5000) {
        for (int freq = 800; freq <= 1200; freq += 20) {
            tone(buzzerPin, freq, 10);
            delay(10);
        }
        for (int freq = 1200; freq >= 800; freq -= 20) {
            tone(buzzerPin, freq, 10);
            delay(10);
        }
        currentMillis = millis();
    }
    noTone(buzzerPin); // Ensure to stop the tone after looping
}


void checkMotion() {
  if (sensorEnabled) {
    bool currentMotion = digitalRead(pirPin) == HIGH;
    if (currentMotion && !lastMotionState && (millis() - lastMotionTime > motionDebounce)) {
      lastMotionTime = millis();
      lastMotionState = true;
       playAlarmTone();  // Play the extended sweeping alarm tone
      publishStatus("Motion Detected");
      Serial.println("Motion Detected");
    } else if (!currentMotion && lastMotionState) {
      lastMotionState = false;
      noTone(buzzerPin);  // Stop any alarm tone being played
      publishStatus("No Motion");
      Serial.println("No Motion");
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();

  bool greenButtonPressed = digitalRead(greenButtonPin) == LOW;
  bool redButtonPressed = digitalRead(redButtonPin) == LOW;

  if (greenButtonPressed && !sensorEnabled && !arming && currentMillis - lastButtonPress > buttonDebounce) {
    arming = true;
    armTime = currentMillis;
    lastButtonPress = currentMillis; // Update last button press time
    playArmingTone();
    publishStatus("Arming Started");
    Serial.println("Arming Started");
  }

  if (arming) {
    flashLEDGradually(armTime);

    if (currentMillis - lastPrintTime >= printInterval) {
      publishStatus("System is still arming...");
      Serial.println("System is still arming...");
      lastPrintTime = currentMillis;
    }

    if (currentMillis - armTime >= armDuration) {
      sensorEnabled = true;
      arming = false;
      playArmedTone();
      digitalWrite(ledPin, HIGH); // Turn on LED when system is armed
      publishStatus("Fully Armed");
      Serial.println("Fully Armed");
    }
  }

  checkMotion();

  if (redButtonPressed && currentMillis - lastButtonPress > buttonDebounce) {
    if (sensorEnabled || alarmActive || arming) {
      sensorEnabled = false;
      alarmActive = false;
      arming = false;
      digitalWrite(ledPin, LOW);
      playDisarmTone();
      publishStatus("Disarmed");
      Serial.println("Disarmed");
      lastButtonPress = currentMillis; // Reset print timer on disarming
    }
  }
}
