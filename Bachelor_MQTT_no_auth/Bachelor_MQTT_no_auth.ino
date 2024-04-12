#include <DHT.h> // DHT library for interacting with the DHT sensor
#include <WiFi.h> // WiFi library for connecting to WiFi
#include <PubSubClient.h> // PubSubClient library for MQTT
#include <ArduinoJson.h> // Include ArduinoJSON library for JSON formatting and processing
#include "secrets.h" // Include the secrets file
#include <esp_sleep.h> // ESP32 deep sleep library

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  300       /* Time ESP32 will go to sleep (in seconds) - 300 seconds = 5 minutes */

#define DHTPIN 15 // DHT22 is attached to pin 15 on the ESP32
#define DHTTYPE DHT22 // Define DHT sensor type (DHTTYPE) as DHT22
DHT dht(DHTPIN, DHTTYPE); // Establish the DHT object

const char* ssid = SECRET_SSID; // Using the defined constant for SSID, pulling its contents from secrets.h
const char* password = SECRET_PASS; // Using the defined constant for password, pulling its contents from secrets.h
const int siteID = 1;
const int deviceID = 1; // Set the device ID here
const char* locationDescriptor = "outside"; //e.g kitchen
const char* mqtt_server = "192.168.1.16"; // MQTT - B - No auth/wildcard.

WiFiClient espClient;
PubSubClient client(espClient);
String clientId;

void reconnect() {
  int counter = 0;
  while (!client.connected()) { // Check if not connected to MQTT broker
    if (counter == 5) { // If connection attempts exceed 5 times
      Serial.println("Failed to reconnect. Restarting...");
      ESP.restart(); // Restart the ESP32
    }
    counter += 1;
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId.c_str())) { // Use clientId as the MQTT client ID and attempt to reconnect
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
  delay(1000); // Take some time to open up the Serial Monitor

  // Initialize DHT sensor
  dht.begin();
  delay(5000); // Wait 5 seconds, as the DHT sensors are slow and need to stabilize

  // Connect to WiFi
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

  // Set up MQTT client
  client.setServer(mqtt_server, 1883);
  clientId = "ESP";
  clientId += deviceID;

  // Reconnect to MQTT
  reconnect();

  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Print temperature and humidity to serial monitor
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" *C");

  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println(" %");

  // Create JSON object
  StaticJsonDocument<100> jsonDocument;
  jsonDocument["siteID"] = siteID;
  jsonDocument["deviceID"] = deviceID;
  jsonDocument["location"] = locationDescriptor;
  
  jsonDocument["temperature"] = temperature;
  jsonDocument["humidity"] = humidity;
  

  // Serialize the JSON object to a string
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  // Publish sensor data to MQTT topic
  if (client.connected()) {
    client.publish("esp32/data", jsonString.c_str());
    Serial.println("Data published to MQTT");
  } else {
    Serial.println("Failed to publish data to MQTT");
  }

  // Disconnect from MQTT
  client.disconnect();

  // Set up deep sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup will put ESP32 to sleep for " + String(TIME_TO_SLEEP) + " seconds");

  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush(); 
  esp_deep_sleep_start();
}

void loop() {
  // This is not going to be called
}
