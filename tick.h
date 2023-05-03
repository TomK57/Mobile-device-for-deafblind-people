#ifndef tick_h
#define tick_h

#include <Arduino.h>
//#define TOUCH

#define CHARSETNUM   3
#define CHARSETCHAR '#'
#define COMMANDCHAR '*'

#define MAXCLIENTS  20 // maximal number of mesh clients
#define TOUCH_HYST 20

#define LED 15

class tickC {
  private:
    uint8_t previousinput;
    char tickChar[CHARSETNUM][32]={{' ','a','e','n','i','d','t',227 /*ä*/,'o','k','m','f','l','g',246 /*ö*/,'r','u','y','b','p','z','w','q','j','s',CHARSETCHAR,'x','v',COMMANDCHAR,'c','h',0},
                                   {' ','1','2','3','4','5','6',      '7','8','9','0','+','-',':'      ,'=','&','/','(',')','<','>',',','.','?','{',CHARSETCHAR,'}','$',COMMANDCHAR,'@','%',0},
                                   {' ','A','E','N','I','D','T',196 /*Ä*/,'O','K','M','F','L','G',214 /*Ö*/,'R','U','Y','B','P','Z','W','Q','J','S',CHARSETCHAR,'X','V',COMMANDCHAR,'C','H',0}};
    
    uint8_t getIOs();
    uint8_t getTouch();
    bool touchR(uint8_t Port);

  public:

      uint8_t  IN0 = 6; 
      uint8_t  IN1 = 7;
      uint8_t  IN2 = 8;
      uint8_t  IN3 = 9;
      uint8_t  IN4 = 10;

      uint8_t  OUT0 = 1;
      uint8_t  OUT1 = 2;
      uint8_t  OUT2 = 3;
      uint8_t  OUT3 = 4;
      uint8_t  OUT4 = 5;
      
      int stabTime = 70; // tick stabilization time in ms
      int outSpeed = 500; // string output speed in ms/character
      int pulseDuration = 150; // duration of tick pulses
      int touchCompare = 50; // touch compare value

      int comMode = 0; //comand Mode aktive 
      int charSet = 0; //current character Set
      uint8_t charSetLock = 0; // char set locked flag
      uint8_t tickClient = 0;   // client mode active flag
      uint8_t outMode = 0; // output mode normal, report all characters to world, 1 no special characters 2 no output
      uint8_t raiseHandMode = 0; // raise hand mode on / of
      uint8_t tickMode = 0; // select tickMode Bit0 up/down or down only delay, Bit1 all fingers up no delay, Bit2 time at start or time at every change (slower) 
      
      String tickString = ""; // current text line
      String tickLine = ""; // memorized last text line
      uint8_t raiseHand[MAXCLIENTS]; // status of clients and server [32] raise hand
      
      String dispText = "";
      
      tickC(); // constructor

      static bool tickCallback(uint8_t num, char* payload); // websocket tick processing

      char getCharacter(); // get character from tick device input
      void setCharacter(char c); // send character to tick device output
      void lineCommand(String c);
      void tickCommand(char c);
      void processTick(char c, uint8_t send); // send bit 0: tick_device, bit 1: mesh_brodcast, bit 2: websocket
      void sendWorld(String c, uint8_t send); // send string to all connected devices
      void sendWorld(char c, uint8_t send); // send character to all connected devices
      void saveSettings(char* FileN);
      void loadSettings(char* FileN);
      void display(String s);
};
#endif
