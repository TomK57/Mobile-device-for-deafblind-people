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
char ssid[32] = "dbclient";   // standard name of client
char ap_ssid[32] = "dbserver"; // name of server access point
char password[32] = ""; // access point password
IPAddress apIP(192, 168, 2, 2); // access point IP adress
/////////////////////////////////////////////////////////////////////////////////////////////////////

DNSServer dnsServer;
WebServer server(80);    // webserver on port 80

String inputString = ""; // String to hold incoming data

unsigned long Millis;

tickC* tick;  // process tick class

WebSocketsClient webSocketClient;
void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      tick->sendWorld("! ");
      tick->sendWorld(ssid);
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      if (length>3) tick->lineCommand((char*)payload); // process command
      else tick->processTick(payload[0]); // process serial tick input
      break;
  }
}


void setup(void) {

  Serial.begin(115200); // start serial communication
  delay(100);

  Serial.println(F("DeafBlind Server Version 0.1 28.03.2022 DHS"));
  
  if (!LITTLEFS.begin()) Serial.println(F("Error initializing Fielsystem"));

  tick = new tickC(); // initialize tick class

  tick->loadSettings("/ap.cfg"); // load current configuration
  
  WiFi.persistent(false); // don't write to flash

// try client connection to server first
  WiFi.mode(WIFI_STA); 
  WiFi.setHostname(ssid); 
  WiFi.begin(ap_ssid,password); // try to connect to dbserver
  Serial.println(F("Try to connect to deaf blind server"));
  delay(100);
  byte u=0,i=0;
  do {
    digitalWrite(ledPin, u);
    if (u == 0) u = 1;
    else u = 0;
    if (WiFi.status() == WL_CONNECTED) break;
    Serial.print(".");
    delay(1000);
    i++;
  } while (i < 10);
  digitalWrite(ledPin, 0);
  if (i < 10) {
    Serial.println("\nConnected");
    MDNS.begin(ssid);               
    webSocketClient.begin(apIP, 81, "/");
    webSocketClient.onEvent(webSocketClientEvent);
    webSocketClient.setReconnectInterval(5000);
    tick->tickClient=1;

  }
  else { // no acces point found switch to acces point mode
    Serial.println("\nNo server found switch to server mode");
    // start access point init
    WiFi.setHostname(ap_ssid);
    WiFi.mode(WIFI_AP); // wifi acces point mode
    //  WiFi.setSleepMode(WIFI_NONE_SLEEP);
    delay(100);
  
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    delay(100);
    WiFi.softAP(ap_ssid);

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
  
    if (!MDNS.begin(ap_ssid)) Serial.println("Error setting up MDNS responder!");
    else Serial.println("mDNS responder started");

    // end access point init
  }
  
  // if DNSServer is started with "*" for domain name, it will reply with provided IP to all DNS request
  dnsServer.start(53, "*", apIP);
  
  Serial.print("AP IP address: ");
  Serial.println(apIP);

  delay(100);

  ArduinoOTA.setHostname(ap_ssid);
  //ArduinoOTA.setPassword("");

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
  Millis = millis();
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
      if (inputString.length() > 4) tick->lineCommand(inputString.substring(0, inputString.length() - 2)); // process line command without cr/lf
      else tick->processTick(inputString[0]); // process serial tick input
      inputString = ""; // clear line
    }
  }

  // process tick input
  if (char input = tick->getCharacter()) tick->processTick(input);

  if ((millis() - Millis) > tick->stabTime+tick->pulseDuration*4+10) Serial.print("long processing time!");
  Millis = millis();
  
  delay(1); // 1ms loop
}
