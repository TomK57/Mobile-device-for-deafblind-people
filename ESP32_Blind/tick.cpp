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
      if (length>3) tick->lineCommand((char*)payload); // process command
      else tick->processTick(payload[0]); // process serial tick input
      break;
  }
}

void tickC::loadSettings(char* FileN) {
  File f = LITTLEFS.open(FileN, "r");  // Open Setting file
  if (!f) {
    Serial.println(F("Error opening seting file for reading"));
  }
  else {
    String x = f.readStringUntil('\n');
    x.trim();
    String ssid = x;
    Serial.print("Device ");
    Serial.println(ssid);
     
    stabTime = atoi(f.readStringUntil('\n').c_str());
    Serial.printf("stabTime = %d\n",stabTime);
    outSpeed = atoi(f.readStringUntil('\n').c_str());
    Serial.printf("outSpeed = %d\n",outSpeed);
    pulseDuration = atoi(f.readStringUntil('\n').c_str());
    Serial.printf("pulseDuration = %d\n",pulseDuration);
    apIP[0] = atoi(f.readStringUntil('.').c_str());
    apIP[1] = atoi(f.readStringUntil('.').c_str());
    apIP[2] = atoi(f.readStringUntil('.').c_str());
    apIP[3] = atoi(f.readStringUntil('\n').c_str());
    Serial.printf("apIP = %d.%d.%d.%d\n",apIP[0],apIP[1],apIP[2],apIP[3]);
    
    String s=f.readStringUntil('\n');
    if (s != String("End\r")) Serial.println(F("Error reading seting file"));
    f.close();
  }
}

void tickC::saveSettings(char* FileN) {
  File f = LITTLEFS.open(FileN, "w");  // Open Setting file
  if (!f) {
    Serial.println(F("Error opening seting file for writing"));
  }
  else {
    f.println(ssid);
    f.println(stabTime);
    f.println(outSpeed);
    f.println(pulseDuration);
    f.printf("%d.%d.%d.%d\n",apIP[0],apIP[1],apIP[2],apIP[3]);
    f.println("End");
    f.close();
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
  int i;
  bool b0,b1,b2,b3,b4;
  
  for (i=1;i<32;i++) if (tickChar[charSet][i] == c) break;
  if (i == 32) { // char not found
    sendWorld("???");
    return;  
  }

  b0 = i & 1;
  b1 = i & 2;
  b2 = i & 4;
  b3 = i & 8;
  b4 = i & 16;  
 
  digitalWrite(ledPin, 1);
  for (i=0; i< charSet+1; i++) { // charSet determines number of pulses
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


void tickC::lineCommand(String c) { // process commands
  switch (c[0]) { // process command
    case 's': stabTime=atoi((char*)&c[2]);   Serial.printf("stabTime: %d\n",stabTime); break;
    case 'd': pulseDuration=atoi((char*)&c[2]); Serial.printf("pulseDuration: %d\n",pulseDuration); break;
    case 'x': outSpeed=atoi((char*)&c[2]); Serial.printf("outSpeed: %d\n",outSpeed); break;
    case 'o': for (int i=2; i<c.length(); i++) { Serial.print(c[i]); setCharacter(c[i]); delay(outSpeed);} Serial.println(); break;
    case 'n': strlcpy(ssid,(char*)&c[2],32); Serial.printf("device Name: %s\n",ssid); WiFi.setHostname(ssid); WiFi.softAP(ssid); MDNS.begin(ssid); break; // change device name  
    case 'r': sendWorld("Rebooting...\n"); ESP.restart(); break;// reset target
    case 'g': c[1]='/'; loadSettings((char*)&c[1]); break; // get (load) current configuration from file system
    case 'p': c[1]='/'; saveSettings((char*)&c[1]); Serial.printf("Config stored in %s\n",(char*)&c[1]); break; // put (save) current configuration to file system
    case 'i': apIP[0] = atoi((char*)&c[2]); apIP[1] = atoi((char*)&c[6]);
              apIP[2] = atoi((char*)&c[10]); apIP[3] = atoi((char*)&c[14]);
              break; // config save and Reboot needed to be effective! Example i 192.168.002.002
  }
}


void tickC::tickCommand(char c) { // process tick command
  uint8_t i = 0, u = 0;
  
  switch (c) { // process command
    case 'w': Serial.println("Switch to wifi"); 
              WiFi.mode(WIFI_STA); 
              WiFi.begin("WLAN-SSID", "xxxxxxxx");
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
              tickClient=0;
              break; 
    case 'c': Serial.println("Switch to client"); 
              WiFi.mode(WIFI_STA); 
              WiFi.begin("ap", "");
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
              tickClient=1;
              break; 
    case 's': Serial.println("Switch to access point server");
              WiFi.mode(WIFI_AP); 
              WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
              delay(100);
              WiFi.softAP(ssid);
              tickClient=0;
              break; 
    case 'm': outMode = ! outMode; 
              Serial.printf("outMode: %d\n",outMode); 
              break; // change outMode
    case 'n': sendWorld("\n");   // send return CR/LF
              break;
  }
}


void tickC::processTick(char c) {  // process tick input 

  if ((c != CHARSETCHAR) && (c != COMMANDCHAR) && (comMode == 0) || (outMode == 0)) { // all character if outMode normal otherwise no special characters
    Serial.print(c);
    String s(c);
    webSocket.broadcastTXT(s); // output char to world
    setCharacter(c); // output char to tick device
  }

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

  if (comMode > 0) tickCommand(c); // process tick command  
  if (comMode == 1) comMode = 0; // if not locked switch com mode back to normal mode

  charSet = 0; // switch back to normal charset after every character
}


void tickC::sendWorld(String c) {
    Serial.print(c); // output to serial
    webSocket.broadcastTXT(c); // output to world
}
