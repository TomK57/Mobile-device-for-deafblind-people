#ifndef tick_h
#define tick_h

//#define DEB

#include <Arduino.h>
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#ifdef ESP32
#include <ESPmDNS.h>
#include <LITTLEFS.h>
#else
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#define LITTLEFS LittleFS
#endif

extern WebSocketsServer webSocket;
extern IPAddress apIP;
extern char ssid[32];
extern char ap_ssid[32];
extern char password[32];
extern WebSocketsClient webSocketClient;
extern void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length);

#ifdef ESP32
#define ledPin 2
#else
#define ledPin 16
#endif

#define CHARSETNUM 3
#define CHARSETCHAR '#'
#define COMMANDCHAR '*'

#define MAXCLIENTS 5 // also defined in websocketserver.h

class tickC {
  private:
    byte previousinput;
    char tickChar[CHARSETNUM][32]={{' ','a','e','n','i','d','t',227 /*ä*/,'o','k','m','f','l','g',246 /*ö*/,'r','u','y','b','p','z','w','q','j','s',CHARSETCHAR,'x','v',COMMANDCHAR,'c','h',0},
                                   {' ','1','2','3','4','5','6',      '7','8','9','0','+','-',':'      ,'=','&','/','(',')','<','>',',','.','?','{',CHARSETCHAR,'}','$',COMMANDCHAR,'@','%',0},
                                   {' ','A','E','N','I','D','T',196 /*Ä*/,'O','K','M','F','L','G',214 /*Ö*/,'R','U','Y','B','P','Z','W','Q','J','S',CHARSETCHAR,'X','V',COMMANDCHAR,'C','H',0}};
    
    byte getIOs();

  public:

      byte  IN0=33; 
      byte  IN1=32;
      byte  IN2=27;
      byte  IN3=14;
      byte  IN4=12;

      byte  OUT0=15;
      byte  OUT1=02;
      byte  OUT2=04;
      byte  OUT3=23;
      byte  OUT4=25;
      
      int stabTime = 70; // tick stabilization time in ms
      int outSpeed = 500; // string output speed in ms/character
      int pulseDuration = 70; // duration of tick pulses

      int comMode = 0; //comand Mode aktive 
      int charSet = 0; //current character Set
      byte charSetLock = 0; // char set locked flag
      byte tickClient = 0;   // client mode active flag
      byte outMode = 0; // output mode normal, report all characters to world, 1 no special characters 2 no output
      byte raiseHandMode = 0; // raise hand mode on / of
      byte tickMode = 0; // select tickMode Bit0 up/down or down only delay, Bit1 all fingers up no delay, Bit2 time at start or time at every change (slower) 
      
      String tickString = ""; // current text line
      String tickLine = ""; // memorized last text line
      String clientName[MAXCLIENTS]; // names of connected clients
      byte raiseHand[MAXCLIENTS+1]; // status of clients and server [32] raise hand
      byte currentClient = 0; // current message comes from client x
      
      tickC(); // constructor

      char getCharacter(); // get character from tick device input
      void setCharacter(char c); // send character to tick device output
      void lineCommand(String c);
      void tickCommand(char c);
      void processTick(char c);
      void sendWorld(String c); // send string to all connected devices
      void sendWorld(char c); // send character to all connected devices
      void saveSettings(char* FileN);
      void loadSettings(char* FileN);
};
#endif
