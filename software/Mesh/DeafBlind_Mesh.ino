#include "DHSWiFi.h"          // own Network functions
#include "DHSWebsocket.h"     // own websocket functions
#include "tick.h"             // own tick processing functions

#define VERSION 0.17          // current SW-Version
// 0.05->0.06:  LED blink duration during WLan rooter search increased (+delay(10);)
//              Acces-Point channel = mesh channel
// 0.06->0.07:  autoreconnect(true) added to WiFi connection
//              WiFi Rooter search 40Sec.
//              WiFi.mode(WIFI_AP); WiFi.softAPConfig(myIP, myIP, IPAddress(255, 255, 255, 0)); for accespoint removed (disturbs mesh connection)
// 0.07->0.08   Added WiFi Scan function:to be continued, changed device name from dbserver to deafblind (same as mesh/access point name)
// 0.08->0.09   multi file download added
// 0.09->0.10   painless mesh-> after update comment all occurences of #ifdef ESP32 SPIFFS in ota.hpp to use LittleFS, otherwise initOTARecive will corrupt flash filesystem !!!
// 0.10->0.11   äöÄÖ changed to -+, problem with special characters in Websocket, to be corrected in newer version!
// 0.11->0.12   AccesPoint name changed to db (to avoid instable overlap with mesh network), added OLED ConnectType displ. W=WLAN, A=AccessPoint
// 0.11->0.14   Optimized AccessPoint/WLAN connection (hopefully)
// 0.14->0.16   Stability delay(1); added in painlessMeshSTA.cpp line 63 and 76 !!!, without crash scanComplete(): num = ... / several changes in painlessMesh lib, search for DHS comments...
// 0.16->0.17   Stability delay(1) removed, not neccessary with disabled scan, password removed (same password for mesh and access point neccessary)

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED width,  in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels

#define I2C_SDA 33
#define I2C_SCL 35

// create an OLED display object connected to I2C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


Scheduler userScheduler;  // to control own task

String inputString = ""; // String to hold incoming data
tickC* tick;  // process tick class

void setup() {
  pinMode(LED, OUTPUT);  // on board LED defined in DHSWiFi.h
  digitalWrite(LED, 1);  // swith LED on during initialization
  Serial.begin(115200);  // init serial port

  Wire.setPins(I2C_SDA, I2C_SCL);
  Wire.begin(); 
  // initialize OLED display with I2C address 0x3C
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C,false,false)) Serial.println("Failed to start SSD1306 OLED");

  oled.clearDisplay(); // clear display
  oled.setTextWrap(false);
  oled.setTextSize(2);         // set text size
  oled.setTextColor(WHITE,BLACK);    // set text color
  oled.setCursor(8, 0);       // set position to display
  oled.print("DB V"); // set text
  oled.print(VERSION);
  oled.setCursor(0, 16);       // set position to display
  oled.print(" Starting"); // set text
  oled.display();              // display on OLED
  Serial.println("Starting DeafBlind Device");

  delay(1000);  // wait for usb-port to initialize
  digitalWrite(LED, 0);  // swith LED off
  delay(1000);  // wait for usb-port to initialize
  digitalWrite(LED, 1);  // swith LED on
  delay(1000);  // wait for usb-port to initialize

  initWifi();       // start mesh network
  initServer();     // start web server
  startWebSocket(); // start WebSocket server
  tick = new tickC(); // initialize tick class

  inputString.reserve(200);
  
  Serial.printf("\nDHS V%4.2f  %s  %s  %s\n", VERSION, __DATE__ , __TIME__,__FILE__); // version / compile time information
        
  digitalWrite(LED, 0);  // swith LED off
  Serial.println("Deafblind Server running...");
}

int8_t xa=0,xe=0;
String text="";

void loop() {
  // get current time
  unsigned long now = millis(); 

  // process mesh communication
  WiFiLoop(); 

  // process webSocket messages
  webSocket.loop(); 

  // process command from serial port
  while (Serial.available()) { 
    char inChar = (char)Serial.read(); // get the new character:
    inputString += inChar; // add it to the inputString:
    if (inChar == '\n') { // if line complete
      if (inputString.length() > 4) tick->lineCommand(inputString.substring(0, inputString.length() - 2)); // if more than 1 character -> process line command without cr/lf
      else tick->processTick(inputString[0], 7); // process serial tick input
      inputString = ""; // clear line
    }
  }

  // process tick input
  if (char input = tick->getCharacter()) tick->processTick(input, 7);

  // check loop time for to high workload
  uint16_t calcTime = millis() - now;
  if (calcTime > 500) Serial.printf("Loop took %ld ms\n",calcTime);

  // repeat (100ns)
  usleep(100); 
}
