#include "tick.h"

extern tickC* tick;

void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      if (length>3) tick->command((char*)payload); // process command
      else tick->processTick(payload[0]); // process serial tick input
      break;
  }
}

tickC::tickC() {

  pinMode(OUT0, OUTPUT);
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);
  pinMode(OUT3, OUTPUT);
  pinMode(OUT4, OUTPUT);
  pinMode(IN0, INPUT_PULLUP); 
  pinMode(IN1, INPUT_PULLUP);
  pinMode(IN2, INPUT_PULLUP); 
  pinMode(IN3, INPUT_PULLUP); 
  pinMode(IN4, INPUT_PULLUP); 
}


byte tickC::getIOs() { // get ticks from IO-Device
  byte input;
  // get inputs
  input = 1-digitalRead(IN4);
  input = (input << 1) | (1-digitalRead(IN3));
  input = (input << 1) | (1-digitalRead(IN2));  
  input = (input << 1) | (1-digitalRead(IN1));
  input = (input << 1) | (1-digitalRead(IN0));
  return input;
}


char tickC::getCharacter() { // get input from IO-device
  
  unsigned long Millis; // time  
  
  byte input = getIOs();
  
  if (input == previousinput ) return(0); // no change

  previousinput = input;
  Millis = millis();

  do { // wait for stable input
    delay(1);
    input = getIOs(); // get current input
    if (input != previousinput) { // input changed?
      previousinput = input;     // reset input
      Millis = millis(); // reset timer   
    }
  } while ( millis() - Millis < stabTime ); // wait for input stabilization
  
  return tickChar[charSet][input]; // return tick character from current char set
}


void tickC::setCharacter(char c) { // output character to IO-Device
  unsigned long currentMillis;
  unsigned long previousMillis = 0; // last time
  int i,pulsecount;
  bool b0,b1,b2,b3,b4;
  
  for (i=1;i<32;i++) if (tickChar[charSet][i] == c) break;
  if (i == 32) { // char not found
    sendWorld("Wrong chracter\n");
    return;  
  }

  b0 = i & 1;
  b1 = i & 2;
  b2 = i & 4;
  b3 = i & 8;
  b4 = i & 16;  
 
  digitalWrite(ledPin, 1);
  for (i=0; i< pulseCount; i++) { // send number of pulses
    digitalWrite(OUT0, ! b0);  // pulse on
    digitalWrite(OUT1, ! b1);  // pulse on
    digitalWrite(OUT2, ! b2);  // pulse on
    digitalWrite(OUT3, ! b3);  // pulse on
    digitalWrite(OUT4, ! b4);  // pulse on
    previousMillis = millis();
    do {
        currentMillis = millis();
    } while (currentMillis - previousMillis < pulseDuration); // wait interval time
    previousMillis = currentMillis;

    digitalWrite(OUT0, 1);  // pulse off
    digitalWrite(OUT1, 1);  // pulse off
    digitalWrite(OUT2, 1);  // pulse off
    digitalWrite(OUT3, 1);  // pulse off
    digitalWrite(OUT4, 1);  // pulse off
 
    do {
      currentMillis = millis();
    } while (currentMillis - previousMillis < pulseDuration); // wait intervall time
    previousMillis = currentMillis;

  }
  digitalWrite(ledPin, 0); 
}


void tickC::command(String c) { // process commands
  switch (c[0]) { // process command
    case 's': if (c.length() > 4) stabTime=atoi((char*)&c[2]);   Serial.printf("stabTime: %d\n",stabTime); break;   // + for intervall increase
    case 'c': if (c.length() > 4) pulseCount=atoi((char*)&c[2]); Serial.printf("pulseCount: %d\n",pulseCount); break;   // + for intervall increase
    case 'd': if (c.length() > 4) pulseDuration=atoi((char*)&c[2]); Serial.printf("pulseDuration: %d\n",pulseDuration); break;   // + for intervall increase
    case 'o': if (c.length() > 4) for (int i=2; i<c.length()-2; i++) { Serial.print(c[i]); setCharacter(c[i]); delay(outSpeed);} Serial.println(); break;
    case 'R': Serial.println("Rebooting..."); ESP.restart(); break;// reset target
  }
}


void tickC::tickCommand(char c) { // process tick command
  uint8_t i = 0, u = 0;
  
  switch (c) { // process command
    case 'w': Serial.println("Switch to wifi"); 
              WiFi.mode(WIFI_STA); 
              WiFi.begin("EasyBox-DB4716", "5EEA7B7DC");
              Serial.print(F("\nConnecting to standard WiFi"));
              delay(1000);
              pinMode(2, OUTPUT);
              do {
                digitalWrite(2, u);
                if (u == 0) u = 1;
                else u = 0;
                if (WiFi.status() == WL_CONNECTED) break;
                Serial.print(".");
                delay(1000);
                i++;
              } while (i < 600);
              Serial.println();
              break; 
    case 'c': Serial.println("Switch to client"); 
              WiFi.mode(WIFI_STA); 
              WiFi.begin(ssid, password);
              Serial.print(F("\nConnecting to access point server"));
              delay(1000);
              pinMode(2, OUTPUT);
              do {
                digitalWrite(2, u);
                if (u == 0) u = 1;
                else u = 0;
                if (WiFi.status() == WL_CONNECTED) break;
                Serial.print(".");
                delay(1000);
                i++;
              } while (i < 6000);
              Serial.println();
              
              webSocketClient.begin(apIP, 81, "/");
              webSocketClient.onEvent(webSocketClientEvent);
              webSocketClient.setReconnectInterval(5000);
              break; 
    case 's': Serial.println("Switch to access point server");
              WiFi.mode(WIFI_AP); 
              WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
              delay(100);
              WiFi.softAP(ssid);
              break; 
  }
}


void tickC::processTick(char c) {  // process tick input 

  if (c == CHARSETCHAR) { // char set character than 
    charSet = ( charSet + 1) % CHARSETNUM; // loop through character sets
     Serial.printf("Char Set %d\n", charSet);
     return;
  }

  if (c == COMMANDCHAR) { // comand character than
    comMode++; // switch com mode on (=1) or lock com mode (=2)
    if (comMode == 3) comMode = 0; // switch com mode off if previously locked
    Serial.printf("CommandMode %d\n",comMode);
    return;
  }

  if (comMode == 0) { // normal character, send to world
    Serial.print(c);
    String s(c);
    webSocket.broadcastTXT(s); // output char to world
    setCharacter(c); // output char to tick device
  }
  else tickCommand(c); // process tick command  

  if (comMode == 1) comMode = 0; // if not locked switch com mode off
}

void tickC::sendWorld(String c) {
    Serial.println(c); // output to serial
    webSocket.broadcastTXT(c); // output to world
}
