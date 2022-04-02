#include "tick.h"

extern tickC* tick;

void tickC::loadSettings(char* FileN) {
  File f = LITTLEFS.open(FileN, "r");  // Open Setting file
  if (!f) {
    Serial.println(F("Error opening seting file for reading"));
  }
  else {
    String x = f.readStringUntil('\n');
    x.trim();
    x.toCharArray(ssid,32);
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

    IN0 = atoi(f.readStringUntil(' ').c_str());
    IN1 = atoi(f.readStringUntil(' ').c_str());
    IN2 = atoi(f.readStringUntil(' ').c_str());
    IN3 = atoi(f.readStringUntil(' ').c_str());
    IN4 = atoi(f.readStringUntil(' ').c_str());
    OUT0 = atoi(f.readStringUntil(' ').c_str());
    OUT1 = atoi(f.readStringUntil(' ').c_str());
    OUT2 = atoi(f.readStringUntil(' ').c_str());
    OUT3 = atoi(f.readStringUntil(' ').c_str());
    OUT4 = atoi(f.readStringUntil('\n').c_str());

    Serial.printf("In0 %d In1 %d In2 %d In3 %d In4 %d  Out0 %d Out1 %d Out2 %d Out3 %d Out4 %d\n",IN0,IN1,IN2,IN3,IN4,OUT0,OUT1,OUT2,OUT3,OUT4);

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
    f.printf("%03d.%03d.%03d.%03d\n",apIP[0],apIP[1],apIP[2],apIP[3]);
    f.printf("%02d %02d %02d %02d %02d %02d %02d %02d %02d %02d\n",IN0,IN1,IN2,IN3,IN4,OUT0,OUT1,OUT2,OUT3,OUT4);
    f.println("End");
    f.close();
  }
}


tickC::tickC() {

  pinMode(OUT0, OUTPUT);    // init IO-Pins
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);
  pinMode(OUT3, OUTPUT);
  pinMode(OUT4, OUTPUT);
  pinMode(IN0, INPUT_PULLUP); 
  pinMode(IN1, INPUT_PULLUP);
  pinMode(IN2, INPUT_PULLUP); 
  pinMode(IN3, INPUT_PULLUP); 
  pinMode(IN4, INPUT_PULLUP); 

  previousinput = getIOs(); // init IO-State
  
  tickString.reserve(200);
  tickLine.reserve(200);
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
  
  byte input = getIOs();

  if (input == previousinput ) return(0); // no finger change, dont wait
  if ( input == 0 ) { // all fingers up, dont wait 
    previousinput = input; 
    return(0); 
  }      
  
  if (tickMode & 1) if ( input < previousinput ) { // only fingers removed, dont wait
    previousinput = input; 
    return(0); 
  }

  if (tickMode & 2) { // old version (slower)  
    unsigned long Millis; // time  
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
  }
  else {
    delay(stabTime); // wait stab time for all fingers to be placed
    input = getIOs(); // read final input
    previousinput = input;  
  }
  
  if (input>0) return tickChar[charSet][input]; // return tick character from current char set
  return 0;
}


void tickC::setCharacter(char c) { // output character to IO-Device
  unsigned long currentMillis;
  unsigned long previousMillis = 0; // last time
  int i;
  bool b0,b1,b2,b3,b4;
  
  for (i=0;i<32;i++) if (tickChar[charSet][i] == c) break;
  if (i == 32) { // char not found
    Serial.print("?");
    return;  
  }

  b0 = i & 1;
  b1 = i & 2;
  b2 = i & 4;
  b3 = i & 8;
  b4 = i & 16;  
 
  digitalWrite(ledPin, 1);
  for (i=0; i< charSet+1; i++) { // charSet determines number of pulses
    digitalWrite(OUT0, b0);  // pulse on
    digitalWrite(OUT1, b1);  // pulse on
    digitalWrite(OUT2, b2);  // pulse on
    digitalWrite(OUT3, b3);  // pulse on
    digitalWrite(OUT4, b4);  // pulse on
    previousMillis = millis();
    do {
        currentMillis = millis();
    } while (currentMillis - previousMillis < pulseDuration); // wait interval time
    previousMillis = currentMillis;

    digitalWrite(OUT0, 0);  // pulse off
    digitalWrite(OUT1, 0);  // pulse off
    digitalWrite(OUT2, 0);  // pulse off
    digitalWrite(OUT3, 0);  // pulse off
    digitalWrite(OUT4, 0);  // pulse off
 
    do {
      currentMillis = millis();
    } while (currentMillis - previousMillis < pulseDuration); // wait intervall time
    previousMillis = currentMillis;

  }
  digitalWrite(ledPin, 0); 
}


void tickC::lineCommand(String c) { // process commands
  int pos;
  
  switch (c[0]) { // process command
    case 's': stabTime=atoi((char*)&c[2]);   Serial.printf("stabTime: %d\n",stabTime); break;
    case 'd': pulseDuration=atoi((char*)&c[2]); Serial.printf("pulseDuration: %d\n",pulseDuration); break;
    case 'x': outSpeed=atoi((char*)&c[2]); Serial.printf("outSpeed: %d\n",outSpeed); break;
    case 'o': if (!tickClient) for (int i=2; i<c.length(); i++) { processTick(c[i]); delay(outSpeed);} break; // output line on server only
    case 'n': if (!tickClient) { 
                strlcpy(ssid,(char*)&c[2],32); 
                Serial.printf("device Name: %s\n",ssid); 
#ifdef ESP32
                WiFi.setHostname(ssid); 
#endif
                MDNS.begin(ssid);
              } 
              break; // change device name  
    case 'r': if (!tickClient) {sendWorld("Rebooting...\n"); ESP.restart();} break;// reset target
    case 'g': c[1]='/'; loadSettings((char*)&c[1]); break; // get (load) current configuration from file system
    case 'p': c[1]='/'; saveSettings((char*)&c[1]); Serial.printf("Config stored in %s\n",(char*)&c[1]); break; // put (save) current configuration to file system
    case 'i': apIP[0] = atoi((char*)&c[2]); apIP[1] = atoi((char*)&c[6]); // change IP-Address
              apIP[2] = atoi((char*)&c[10]); apIP[3] = atoi((char*)&c[14]);
              break; // config save and Reboot needed to be effective! Example i 192.168.002.002
    case 'c': pos=atoi((char*)&c[4]); // set character in character set. Example: c ! 43
              if (pos/32 < CHARSETNUM) {
                tickChar[pos / 32][pos % 32] = c[2]; 
                Serial.printf("Set %d, Pos %d: %c\n",pos/32,pos%32,c[2]);
              }
              break;
    case 't': tickMode=atoi((char*)&c[2]); Serial.printf(" tickMode: %d\n",tickMode); break;     // change tickMode
    case 'q': if (c.length()>2) IN0=atoi((char*)&c[2]); // set IO-Ports Example q 15 04 18
              if (c.length()>5) IN1=atoi((char*)&c[5]);
              if (c.length()>8) IN2=atoi((char*)&c[8]);
              if (c.length()>11) IN3=atoi((char*)&c[11]);
              if (c.length()>14) IN4=atoi((char*)&c[14]);
              if (c.length()>17) OUT0=atoi((char*)&c[17]);
              if (c.length()>20) OUT1=atoi((char*)&c[20]);
              if (c.length()>23) OUT2=atoi((char*)&c[23]);
              if (c.length()>26) OUT3=atoi((char*)&c[26]);
              if (c.length()>29) OUT4=atoi((char*)&c[29]);
              Serial.printf("In0 %2d In1 %2d In2 %2d In3 %2d In4 %2d  Out0 %2d Out1 %2d Out2 %2d Out3 %2d Out4 %2d\n",IN0,IN1,IN2,IN3,IN4,OUT0,OUT1,OUT2,OUT3,OUT4);
              break;
  }
}


void tickC::tickCommand(char c) { // process tick command
  uint8_t i = 0, u = 0;

  if (tickClient) return; // only execute on server (lokal commands to be implemented!!!)
  
  switch (c) { // process command
    case 'w': Serial.println(" Switch to wifi");
              WiFi.disconnect();
              delay(100); 
              WiFi.mode(WIFI_AP_STA);
              delay(100);
              WiFi.begin("EasyBox-DB4716", "xxxxxxx");
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
              } while (i < 60);
              Serial.println(ssid);
              tickClient=0;
              
              break; 
    case 'c': Serial.println(" Switch to client"); 
              WiFi.mode(WIFI_STA); 
              WiFi.begin(ap_ssid,password);
#ifdef ESP32
              WiFi.setHostname(ssid); 
#endif
              Serial.println(F("Connecting to deaf blind server"));
              delay(1000);
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
              if (i != 10) {
                Serial.println("Connected");
                MDNS.begin(ssid);               
                webSocketClient.begin(apIP, 81, "/");
                webSocketClient.onEvent(webSocketClientEvent);
                webSocketClient.setReconnectInterval(5000);
                tickClient=1;
              }
              //break; no break to switch back to access point mode
    case 's': Serial.println(" Switch to access point server");
              WiFi.mode(WIFI_AP); 
              WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
              delay(100);
              WiFi.softAP(ap_ssid);
#ifdef ESP32
              WiFi.setHostname(ap_ssid); 
#endif
              MDNS.begin(ap_ssid); 
              tickClient=0;
              break; 
    case 'm': outMode= (outMode+1); // change outMode 
              if (raiseHandMode) outMode = outMode % 3;
              else outMode = outMode % 2;
              Serial.printf(" outMode: %d ",outMode); 
              break; 
    case 'r': raiseHandMode = !raiseHandMode; // change raiseHandMode
              outMode = 2 * raiseHandMode; // no character output 
              Serial.printf(" raiseHandMode %d ",raiseHandMode); 
              break;
    case 'h': ; // allow next communication from raise hand to be implemented
              Serial.print(" raiseHand sheduler "); 
              break;
    case 'n': sendWorld("\n");   // send return CR/LF
              tickLine = tickString.substring(0, tickString.length() - 2);
              tickString = "";
              Serial.print(" Stored line: ");
              Serial.println(tickLine);
              break;
    case 'z': tickString = tickString.substring(0, tickString.length() - 2)+" "; // add space character to text line
              break;
    case 'e': if (tickLine.length() > 2) { comMode = 0; tick->lineCommand(tickLine);} // process tickLine without cr/lf
              break;
    case 'l': u = 0; // send names of all active clients to world
              comMode=0;
              for (int j=0; j<strlen(ssid); j++) { processTick(ssid[j]); delay(outSpeed); } // first output server name
              processTick(' '); delay(outSpeed); // short pause between names
              for (i=0; i<MAXCLIENTS; i++) { // loop through clients
                if (clientName[i].length() != 0) { // activ with given name?
                  for (int j=0; j<clientName[i].length(); j++) { processTick(clientName[i][j]); delay(outSpeed); } // output active client name
                  processTick(' '); delay(outSpeed); // short pause between names
                  u++; // count clients
                }
              }
              Serial.printf(" %d active clients\n",u);
              break;

    default:  Serial.print("?"); // command not found
  }
}


void tickC::processTick(char c) {  // process tick input 

  tickString += c; // add it to the inputString:

  if ((outMode == 0) || ((outMode == 1)&&(c!=COMMANDCHAR)&&(c!=CHARSETCHAR))) { // send character to world if outmode = sendall (0) or no command and outMode = 1
    sendWorld(c); // output char to world
  }   
  if (outMode < 2) switch (c) { // communication alowed
    
    case COMMANDCHAR:
      if (charSet == 0) { // normal char set -> normal command
        comMode++; // switch com mode on (=1) or lock com mode (=2)
        if (comMode == 3) comMode = 0; // switch com mode off if previously locked
      }
      else charSetLock = 1; // commandchar after charset change -> lock charset
      break;
      
    case CHARSETCHAR:
      if (charSetLock) { // char set locked -> unlock and reset to normal charset
        charSetLock = 0;
        charSet = 0;
      }
      else  charSet = ( charSet + 1) % CHARSETNUM; // loop through character sets
      Serial.printf(" CharSet %d ", charSet);
      break;
      
   default:
     if ((comMode > 0) && ( outMode < 2)) tickCommand(c); // process tick command if communication alowed
     if (comMode == 1) comMode = 0; // if not locked switch com mode back to normal mode
     if (charSetLock == 0) charSet = 0; // switch back to normal charset after every character if not locked
   }
   else {
    Serial.printf(" raiseHand ");
    if (tickClient) webSocketClient.sendTXT(">1"); // client raise hand
    else raiseHand[MAXCLIENTS] = 1; // server raise hand    
    outMode=0; // reset of outmode -> in future done by server! sheduler to be implemented.
   }
}


void tickC::sendWorld(String c) {
    Serial.print(c); // output to serial
    webSocket.broadcastTXT(c); // output to world
}

void tickC::sendWorld(char c) {
    Serial.print(c); // output to serial
    String s(c);
    webSocket.broadcastTXT(s); // output to world
    setCharacter(c); // output char to tick device
}
