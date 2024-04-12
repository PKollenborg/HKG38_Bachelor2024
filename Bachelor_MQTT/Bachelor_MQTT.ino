#include <DHT.h> // DHT library for interacting with the DHT sensor
#include <WiFi.h> // WiFi library for connecting to WiFi
#include <PubSubClient.h> // PubSubClient library for MQTT
#include <ArduinoJson.h> // Include ArduinoJSON library for JSON formatting and processing
#include "secrets.h" // Include the secrets file

const char* ssid = SECRET_SSID; // Using the defined constant for SSID, pulling its contents from secrets.h
const char* password = SECRET_PASS; // Using the defined constant for password, pulling its contents from secrets.h
const int siteID = 1;
const int deviceID = 1; // Set the device ID here
const char* locationDescriptor = "hallway"; //e.g kitchen
const char* mqtt_server = "192.168.1.16"; 

#define DHTPIN 15 // DHT22 is attached to pin 15 on the ESP32
#define DHTTYPE DHT22 // Define DHT sensor type (DHTTYPE) as DHT22
DHT dht(DHTPIN, DHTTYPE); // Establish the DHT object

// MQTT Setup Start - WiFi and MQTT client objects
WiFiClient espClient;
PubSubClient client(espClient);
String siteid;
String clientId;

// Function prototype for the "reconnect" function
void reconnect();

// MQTT Setup End

float temperature, humidity; // Declare temp and hum variables

void setup() {
  Serial.begin(9600); // Begin serial communications at 9600 baud
  Serial.println();

  Serial.print("Connecting to ");
  Serial.println(ssid); // Connecting to <$SSID_Name> printed
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, password);
  // Use SSID and network password declared as constants at the start of the sketch
  // Pull these values from secrets
  // Then wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: "); // Print local IP
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);

  clientId = "ESP";
  clientId += deviceID; // Combine "ESP" and deviceID to create a unique clientId

  if (!client.connect(clientId.c_str())) { // Use clientId as the MQTT client ID
    Serial.println("Failed to connect to MQTT broker. Restarting...");
    ESP.restart();
  }

  dht.begin(); // Initialize DHT sensor
  delay(5000); // Wait 5 seconds, as the DHT sensors are slow and need to stabilize
}

// Get readings from DHT and print them to serial
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

// Reconnect function for MQTT
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

void loop() {
  if (!client.connected()) {
    // Reconnect using the reconnect function
    reconnect();
  }

  getValues(); // Get humidity and temperature readings
 // Create a JSON object to hold device ID and sensor readings
  StaticJsonDocument<100> jsonDocument;
  jsonDocument["siteID"] = siteID;
  jsonDocument["deviceID"] = deviceID;
  jsonDocument["temperature"] = temperature;
  jsonDocument["humidity"] = humidity;
  jsonDocument["location"] = locationDescriptor; 

  // Serialize the JSON object to a string
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  // Publish the JSON data to an MQTT topic
  if (!client.publish("esp32/data", jsonString.c_str())) {
    // If publish fails, handle accordingly (e.g., restart)
    Serial.println("Publish failed. Restarting...");
    ESP.restart();
  }

  delay(30000); // Repeat every 30 seconds - For production use, this should be set higher.
}