#ifndef tick_h
#define tick_h

//#define DEB

#include <Arduino.h>
#include <WebSocketsServer.h>

extern WebSocketsServer webSocket;
extern IPAddress apIP;
extern char ssid[32];
extern char password[32];

#define IN0 15 //D15
#define IN1 4 //D4
#define IN2 10 //D10
#define IN3 22 //D22
#define IN4 23 //D23

#define OUT0 32
#define OUT1 33
#define OUT2 34
#define OUT3 35
#define OUT4 25

#define ledPin 2

#define CHARSETNUM 2
#define CHARSETCHAR '#'
#define COMMANDCHAR '*'

class tickC {
  private:
    byte previousinput;
    char tickChar[CHARSETNUM][32]={{0,'a','e','n','i','d','t','ä','o','k','m','f','l','g','ö','r','u','y','b','p','z','w','q','j','s',CHARSETCHAR,'x','v','ü','c','h',COMMANDCHAR},
                                   {0,'1','2','3','4','5','6','7','8','9','0','!','"',' ','%','&','/','(',')','<','>',',','.','?',';',CHARSETCHAR,'-','+',':','@','=',COMMANDCHAR}};
    
    byte getIOs();
    
  public:

      int stabTime = 20; // tick stabilization time in ms
      int outSpeed = 500; // string output speed in ms/character
      int pulseCount = 3;
      int pulseDuration = 5;
      int comMode = 0; //comand Mode aktive
      int charSet = 0; //character Set
      
      tickC();

      char getCharacter();
      void setCharacter(char c);
      void command(String c);
      void tickCommand(char c);
      void processTick(char c);
      void sendWorld(String c);
};
#endif