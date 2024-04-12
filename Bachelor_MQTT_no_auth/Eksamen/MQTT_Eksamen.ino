#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

//IDs
const int siteID = 1;
const int deviceID = 4;
// Location descriptor
const char* locationDescriptor = "garden"; //e.g kitchen
const char* mqtt_server = "192.168.235.131";

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// MQTT Setup
WiFiClient espClient;
PubSubClient client(espClient);
String siteid;
String clientId;



void reconnect();

float temperature, humidity;

void setup() {
  Serial.begin(9600);
  Serial.println();

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);

  clientId = "ESP";
  clientId += deviceID;

  if (!client.connect(clientId.c_str())) {
    Serial.println("Failed to connect to MQTT broker. Restarting...");
    ESP.restart();
  }

  dht.begin();
  delay(5000);
}

void getValues() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" *C");

  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.println();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  getValues();

  // Create a JSON object to hold device ID, sensor readings, and location descriptor
  StaticJsonDocument<200> jsonDocument; // Adjust the size if necessary
  jsonDocument["siteID"] = siteID;
  jsonDocument["deviceID"] = deviceID;
  jsonDocument["temperature"] = temperature;
  jsonDocument["humidity"] = humidity;
  jsonDocument["location"] = locationDescriptor; // Use the constant location descriptor

  // Serialize the JSON object to a string
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  // Publish the JSON data to an MQTT topic
  if (!client.publish("esp32/data", jsonString.c_str())) {
    Serial.println("Publish failed. Restarting...");
    ESP.restart();
  }

  delay(30000);
}

void reconnect() {
  int counter = 0;
  while (!client.connected()) {
    if (counter == 5) {
      Serial.println("Failed to reconnect. Restarting...");
      ESP.restart();
    }
    counter += 1;
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
