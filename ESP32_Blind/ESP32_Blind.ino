// ESP32 Mobile-device-for-deafblind-people
// https://github.com/TomK57/Mobile-device-for-deafblind-people
// Settings: Board: ESP32 Dev Module, Upload Speed 115000, Flash Freq. 80 Mhz, SSL Basic

// regular Libraries
#include <time.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <LITTLEFS.h>

// own c++ classes
#include <WebSocketsClient.h>
#include "websocket.h"
#include "handleHttp.h"
#include "tick.h"

//Basic comunication settings ///////////////////////////////////////////////////////////////////////
char ssid[32] = "ap";   // name of access point
char password[32] = ""; // access point password
IPAddress apIP(192, 168, 2, 2); // access point IP adress
/////////////////////////////////////////////////////////////////////////////////////////////////////

DNSServer dnsServer;
WebServer server(80);    // webserver on port 80

String inputString = "";         // a String to hold incoming data (currently not used here)

tickC* tick;  // process tick class

WebSocketsClient webSocketClient;

void setup(void) {
  
  WiFi.persistent(false); // don't write to flash
  WiFi.setHostname(ssid);
  WiFi.mode(WIFI_AP); // wifi acces point mode
//  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  Serial.begin(115200); // start serial communication
  delay(100);

  if (!LITTLEFS.begin()) Serial.println(F("Error initializing Fielsystem"));

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(100);
  WiFi.softAP(ssid);

  // if DNSServer is started with "*" for domain name, it will reply with provided IP to all DNS request
  dnsServer.start(53, "*", apIP);

  Serial.print("AP IP address: ");
  Serial.println(apIP);

  delay(100);

  ArduinoOTA.setHostname(ssid);
  ArduinoOTA.setPassword(ssid);

  ArduinoOTA.onStart([]() { Serial.println("OTA Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("OTA End"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA started\n");

  if (!MDNS.begin(ssid)) {
        Serial.println("Error setting up MDNS responder!");
    }
  else Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  // setup html pages
  server.begin(); // Web server start
  server.on("/admin", handleRoot); // initial config page (usable even when no html files are uploaded)
  server.on(F("/wifi"), handleWifi); // WiFi configuration (not used for access point)
  server.on(F("/wifisave"), handleWifiSave); // store WiFi Data (not used for access point)
  server.on("/upload", HTTP_POST, []() {}, handleFileUpload); // file uppload
  server.onNotFound ( handleFileRead ); // all other pages are tried to read from file system 
  Serial.println(F("HTTP server started"));

  startWebSocket(); // Start WebSocket server for command processing

  // I/O pin initialisation
  pinMode(ledPin, OUTPUT);

  inputString.reserve(200);

  tick = new tickC(); // initialize tick class
}

void loop(void) {

  // process webSocket messages
  webSocket.loop();
  
  // process webClient if active
  if (tick->tickClient) webSocketClient.loop();
  
  // process web server messages
  server.handleClient();

  // process OTA SW update
  ArduinoOTA.handle();

  // process serial input
  while (Serial.available()) { // new command from serial port ?
    char inChar = (char)Serial.read(); // get the new byte:
    inputString += inChar; // add it to the inputString:
    if (inChar == '\n') { // if line complete
      if (inputString.length() > 3) tick->command(inputString); // process command
      else tick->processTick(inputString[0]); // process serial tick input
      inputString = ""; // clear line
    }
  }

  // process tick input
  if (char input = tick->getCharacter()) tick->processTick(input);
  
  delay(10); // 10ms loop
}
