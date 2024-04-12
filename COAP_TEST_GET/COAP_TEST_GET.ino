#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h> // Include ArduinoJson library

const char* ssid     = "BSG38";
const char* password = "Bachelor2024";

const char* deviceID = "esp32-CoAP"; // Specify your device ID
const char* siteID = "1000"; // Specify your site ID
const char* locationDescriptor = "Location1"; // Specify your location descriptor


// UDP and CoAP class
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
  Serial.println(WiFi.localIP()); // Print IP address to serial

  // Registering endpoint for "hello" callback
  coap.server(callback_hello, "hello");

  coap.start();
}

void loop() {
  delay(1000);
  coap.loop();
}

void callback_hello(CoapPacket &packet, IPAddress ip, int port) {
  // Get origin IP address of the request
  IPAddress originIP = udp.remoteIP();
  Serial.print("Request from ");
  Serial.print(originIP);
  Serial.println(" received.");

  // Create JSON object
  StaticJsonDocument<200> jsonDocument;
  jsonDocument["siteID"] = siteID;
  jsonDocument["deviceID"] = deviceID;
  jsonDocument["location"] = locationDescriptor;
  jsonDocument["response"] = "hello from " + WiFi.localIP().toString();

  // Serialize JSON to string
  char responseChar[200];
  serializeJson(jsonDocument, responseChar);

  // Print the response to the serial port
  Serial.print("Sending response: ");
  Serial.println(responseChar);

  // Send the response as POST request
  coap.send(ip, port, "hello", COAP_NONCON, COAP_POST, NULL, 0, (uint8_t *)responseChar, strlen(responseChar));
}
