#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

const char* ssid = SECRET_SSID;               // Change it to your WiFi SSID
const char* password = SECRET_PASS;    // Change it to your WiFi password
const char* mqtt_server = "192.168.1.15"; // Change it to your MQTT broker's IP address
const char* mqtt_topic = "esp32/data";
const int mqtt_port = 1883;

const int siteID = 174;
const int deviceID = 15;
const char* locationDescriptor = "Doorbell"; 

const int buttonPin = 2;
const int buzzerPin = 3;
const int relayPin = 4;

const int sirenLowFreq = 500;
const int sirenHighFreq = 1500;
const int sirenDuration = 3000;
const int numLoops = 2;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    delay(5000); // Adjust this delay if needed
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ArduinoClient")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  setup_wifi();  // Establish WiFi connection
  client.setServer(mqtt_server, mqtt_port); // Use defined MQTT server IP and port
  pinMode(buttonPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH) {
    StaticJsonDocument<200> jsonDocument;
    jsonDocument["siteID"] = siteID;
    jsonDocument["deviceID"] = deviceID;
    jsonDocument["location"] = locationDescriptor;
    jsonDocument["doorbellStatus"] = "Doorbell Pressed";

    char jsonStr[200];
    serializeJson(jsonDocument, jsonStr);

    if (client.publish(mqtt_topic, jsonStr)) {
      Serial.println("Message published successfully.");
      Serial.println(jsonStr);
    } else {
      Serial.println("Failed to publish message.");
    }

    for (int i = 0; i < numLoops; i++) {
      activateSiren();
    }
  }
}

void activateSiren() {
  digitalWrite(relayPin, HIGH);

  unsigned long startTime = millis();

  while (millis() - startTime < sirenDuration) {
    unsigned long elapsed = millis() - startTime;
    int frequency = map(elapsed, 0, sirenDuration, sirenLowFreq, sirenHighFreq);

    tone(buzzerPin, frequency);

    delay(50);
  }

  noTone(buzzerPin);
  digitalWrite(relayPin, LOW);
}
