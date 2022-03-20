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
#include "websocket.h"
#include "handleHttp.h"

//Basic comunication settings ///////////////////////////////////////////////////////////////////////
char ssid[32] = "ap";   // name of access point
char password[32] = ""; // access point password
IPAddress apIP(192, 168, 2, 2); // access point IP adress
/////////////////////////////////////////////////////////////////////////////////////////////////////

DNSServer dnsServer;
WebServer server(80);    // webserver on port 80

int ledPin     = 2; // the port number of the on-board LED pin
long interval  = 5; // initial interval duration to vibrate
int pulsecount = 3; // initial vibrate pulsecount

unsigned long currentMillis;
unsigned long previousMillis = 0; // last time

// String inputString = "";         // a String to hold incoming data (currently not used here)

int input, previousinput; // input pin states

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
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(15, INPUT); 

//  inputString.reserve(200);
}

void loop(void) {

  webSocket.loop();
  server.handleClient();
  ArduinoOTA.handle();

  while (Serial.available()) { // new commadn from serial port?
    // get the new byte:
    char inChar = (char)Serial.read();
/*    // add it to the inputString:
    inputString += inChar;
    if (inChar == '\n') {
      Serial.println(inputString);
      previousinput = 2;
      inputString = "";
    }
*/
    switch (inChar) { // process command
      case '+': interval++; break;   // + for intervall increase
      case '-': interval--; break;   // - for intervall decrease
      case 'u': pulsecount++; break; // u for pulsecount increase
      case 'd': pulsecount--; break; // d for pulsecount decreased
    }
    // report new settings
    Serial.print("Interval ");
    Serial.print(interval);
    Serial.print(" Pulsecount ");
    Serial.print(pulsecount);
    Serial.print(" Freq. ");
    Serial.print(250/interval/pulsecount);
    Serial.print(" Duration ");
    Serial.println(interval*2*pulsecount);
  }

  // get input status
  input = digitalRead(15);
  if (input != previousinput ) { // if input changes
    
    if ((input==0) || (previousinput==2)) { // if input low or tick command from websocket
     for (int i=0; i< pulsecount; i++) { // send number of pulses
      digitalWrite(ledPin, 1); // pulse on
      digitalWrite(32, 1);
      digitalWrite(33, 0);
      do {
        currentMillis = millis();
      } while (currentMillis - previousMillis < interval); // wait interval time
      previousMillis = currentMillis;

      digitalWrite(ledPin, 0); // pulse off
      digitalWrite(32, 0);
      digitalWrite(33, 1);
      do {
        currentMillis = millis();
      } while (currentMillis - previousMillis < interval); // wait intervall time
      previousMillis = currentMillis;
     }
     Serial.println("*");
     webSocket.broadcastTXT("*\n");  
    }
    digitalWrite(32, 0); // pulse deactivate
    digitalWrite(33, 0);
    previousinput = input;
  }
  
  delay(10); // 10ms loop
}
