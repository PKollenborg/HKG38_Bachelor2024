#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "secrets.h"

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

WiFiUDP udp;
Coap coap(udp);

#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const char* coap_server = "192.168.235.131";
const int coap_port = 5683;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  dht.begin();
  coap.start();
}

void loop() {
  static unsigned long lastSendTime = 0;
  const unsigned long sendInterval = 60000; // Send data every 60 seconds

  if (millis() - lastSendTime > sendInterval) {
    sendTemperatureData();
    sendHumidityData();
    lastSendTime = millis();
  }

  coap.loop();
}

void sendTemperatureData() {
  float temperature = dht.readTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed to read temperature from DHT sensor!");
    return;
  }

  DynamicJsonDocument jsonDocument(256);
  jsonDocument["temperature"] = temperature;
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  IPAddress serverIp;
  if (!WiFi.hostByName(coap_server, serverIp)) {
    Serial.println("DNS lookup failed. Rebooting.");
    ESP.restart();
  }
  
  coap.post(serverIp, coap_port, "/temperature", jsonString);
}

void sendHumidityData() {
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed to read humidity from DHT sensor!");
    return;
  }

  DynamicJsonDocument jsonDocument(256);
  jsonDocument["humidity"] = humidity;
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  IPAddress serverIp;
  if (!WiFi.hostByName(coap_server, serverIp)) {
    Serial.println("DNS lookup failed. Rebooting.");
    ESP.restart();
  }

  coap.post(serverIp, coap_port, "/humidity", jsonString);
}
