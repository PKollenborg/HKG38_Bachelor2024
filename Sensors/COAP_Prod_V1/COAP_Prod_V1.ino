#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h>

const char* ssid = "SSID"; //CHANGE AS REQUIRED
const char* password = "PASSWORD"; //CHANGE AS REQUIRED

const char* deviceID = "esp32-CoAP"; //CHANGE AS DESIRED
const char* siteID = "1000"; //CHANGE AS DESIRED
const char* locationDescriptor = "Location1"; //CHANGE AS DESIRED

WiFiUDP udp;
Coap coap(udp);

void callback_hello(CoapPacket &packet, IPAddress ip, int port);

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  coap.server(callback_hello, "hello");

  coap.start();
}

void loop() {
  delay(1000);
  coap.loop();
}

void callback_hello(CoapPacket &packet, IPAddress ip, int port) {
  Serial.print("Request from ");
  Serial.print(ip);
  Serial.println(" received.");

  StaticJsonDocument<200> jsonDocument;
  jsonDocument["siteID"] = siteID;
  jsonDocument["deviceID"] = deviceID;
  jsonDocument["location"] = locationDescriptor;
  jsonDocument["response"] = "hello from " + WiFi.localIP().toString();

  char responseChar[200];
  serializeJson(jsonDocument, responseChar);

  Serial.print("Sending response: ");
  Serial.println(responseChar);

  // Send the response to the server at IP address 192.168.1.15 and port 5683
  coap.send(IPAddress(192, 168, 1, 15), 5683, "hello", COAP_NONCON, COAP_POST, NULL, 0, (uint8_t *)responseChar, strlen(responseChar));
}    // 192.168.1.15 is designated response, change as required.
