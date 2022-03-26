#ifndef tick_h
#define tick_h

//#define DEB

#include <Arduino.h>
#include <WebSocketsServer.h>
#include <WebSocketsClient.h>
#include <ESPmDNS.h>
#include <LITTLEFS.h>

extern WebSocketsServer webSocket;
extern IPAddress apIP;
extern char ssid[32];
extern char ap_ssid[32];
extern char password[32];
extern WebSocketsClient webSocketClient;
extern void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length);

#define IN0 15 //D15
#define IN1 4 //D4
#define IN2 18 //D18
#define IN3 22 //D22
#define IN4 23 //D23

#define OUT0 32
#define OUT1 33
#define OUT2 34
#define OUT3 35
#define OUT4 25

#define ledPin 2

#define CHARSETNUM 3
#define CHARSETCHAR '#'
#define COMMANDCHAR '*'

#define MAXCLIENTS 5 // also defined in websocketserver.h

class tickC {
  private:
    byte previousinput;
    char tickChar[CHARSETNUM][32]={{' ','a','e','n','i','d','t','ä','o','k','m','f','l','g','ö','r','u','y','b','p','z','w','q','j','s',CHARSETCHAR,'x','v','ü','c','h',COMMANDCHAR},
                                   {' ','1','2','3','4','5','6','7','8','9','0','!','"',' ','%','&','/','(',')','<','>',',','.','?',';',CHARSETCHAR,'-','+',':','@','=',COMMANDCHAR},
                                   {' ','A','E','N','I','D','T','Ä','O','K','M','F','L','G','Ö','R','U','Y','B','P','Z','W','Q','J','S',CHARSETCHAR,'X','V','Ü','C','H',COMMANDCHAR}};
    
    byte getIOs();

    
  public:

      int stabTime = 20; // tick stabilization time in ms
      int outSpeed = 500; // string output speed in ms/character
      int pulseDuration = 5; // duration of tick pulses
      int comMode = 0; //comand Mode aktive 
      int charSet = 0; //current character Set
      byte charSetLock = 0; // char set locked flag
      byte tickClient = 0;   // client mode active flag
      byte outMode = 0; // output mode normal, report all characters to world
      String tickString = ""; // current text line
      String tickLine = ""; // memorized last text line
      String clientName[MAXCLIENTS]; // names of connected clients
      
      tickC();

      char getCharacter(); // get character from tick device input
      void setCharacter(char c); // send character to tick device output
      void lineCommand(String c);
      void tickCommand(char c);
      void processTick(char c);
      void sendWorld(String c); // send character to all connected devices
      void saveSettings(char* FileN);
      void loadSettings(char* FileN);
};
#endif
