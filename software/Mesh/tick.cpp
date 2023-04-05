#include "tick.h"
#include <LittleFS.h>

#include <Adafruit_SSD1306.h>
#include "EEPROM.h"

#include "DHSWiFi.h"
#include "DHSWebsocket.h"

extern tickC* tick;
extern painlessMesh mesh;  // Mesh-Network class
extern uint8_t ConnectType;
extern uint8_t ConnectMesh;  // status of mesh connection

extern Adafruit_SSD1306 oled;

void tickC::loadSettings(char* FileN) {
  File f = LittleFS.open(FileN, "r");  // Open Setting file
  if (!f) {
    Serial.println(F("Error opening seting file for reading, use defaults"));
  } else {
    stabTime = atoi(f.readStringUntil('\n').c_str());
    Serial.printf("stabTime = %d\n", stabTime);
    outSpeed = atoi(f.readStringUntil('\n').c_str());
    Serial.printf("outSpeed = %d\n", outSpeed);
    pulseDuration = atoi(f.readStringUntil('\n').c_str());
    Serial.printf("pulseDuration = %d\n", pulseDuration);
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

    Serial.printf("In0 %d In1 %d In2 %d In3 %d In4 %d  Out0 %d Out1 %d Out2 %d Out3 %d Out4 %d\n", IN0, IN1, IN2, IN3, IN4, OUT0, OUT1, OUT2, OUT3, OUT4);

    pinMode(OUT0, OUTPUT);  // init IO-Pins
    pinMode(OUT1, OUTPUT);
    pinMode(OUT2, OUTPUT);
    pinMode(OUT3, OUTPUT);
    pinMode(OUT4, OUTPUT);
#ifndef TOUCH
    pinMode(IN0, INPUT_PULLUP);
    pinMode(IN1, INPUT_PULLUP);
    pinMode(IN2, INPUT_PULLUP);
    pinMode(IN3, INPUT_PULLUP);
    pinMode(IN4, INPUT_PULLUP);

    previousinput = getIOs();  // init IO-State
#else
    previousinput = getTouch();  // init Touch-State
#endif
    String s = f.readStringUntil('\n');
    if (s != String("End\r")) Serial.println(F("Error reading seting file"));
    f.close();
  }
}

void tickC::saveSettings(char* FileN) {
  File f = LittleFS.open(FileN, "w");  // Open Setting file
  if (!f) {
    Serial.println(F("Error opening seting file for writing"));
  } else {
    f.println(stabTime);
    f.println(outSpeed);
    f.println(pulseDuration);
    f.printf("%02d %02d %02d %02d %02d %02d %02d %02d %02d %02d\n", IN0, IN1, IN2, IN3, IN4, OUT0, OUT1, OUT2, OUT3, OUT4);
    f.println("End");
    f.close();
  }
}

bool tickC::tickCallback(uint8_t num, char* payload) {  // websocket tick processing
  if (strlen(payload) > 2) tick->lineCommand(payload);  // if more than 1 character -> process line command
  else tick->processTick(payload[0], 7);                // process websocket tick input

  return false;  // continue processing for other callbacks
}

tickC::tickC() {

  pinMode(OUT0, OUTPUT);  // init IO-Pins
  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);
  pinMode(OUT3, OUTPUT);
  pinMode(OUT4, OUTPUT);
#ifndef TOUCH
  pinMode(IN0, INPUT_PULLUP);
  pinMode(IN1, INPUT_PULLUP);
  pinMode(IN2, INPUT_PULLUP);
  pinMode(IN3, INPUT_PULLUP);
  pinMode(IN4, INPUT_PULLUP);

  previousinput = getIOs();  // init IO-State
#else
  previousinput = getTouch();    // init Touch-State
#endif

  tickString.reserve(200);  // prereserve string space for faster execution
  tickLine.reserve(200);    // prereserve string space for faster execution

  pluginCallbacks[DHSNumPlugins] = tickCallback;  // set web socket callback for tick processing
  DHSNumPlugins++;

  tickClient = 1;                        // start as mesh client
  loadSettings((char*)"/ap.cfg");  // load current tick configuration
}

bool tickC::touchR(uint8_t Port) {
  int touchVal1, touchVal2, i = 0;

  touchVal1 = touchRead(Port);
  do {
    touchVal2 = touchRead(Port);
    i++;
  } while ((i < 5) && (abs(touchVal1 - touchVal2) > TOUCH_HYST));

  // Serial.printf("%d : %d\n",Port,touchVal2);

  if (touchVal2 < touchCompare) return true;
  else return false;
}

uint8_t tickC::getTouch() {  // get ticks from Touch-Device
  uint8_t input;
  // get inputs
  input = touchR(IN4);
  input = (input << 1) | touchR(IN3);
  input = (input << 1) | touchR(IN2);
  input = (input << 1) | touchR(IN1);
  input = (input << 1) | touchR(IN0);

  //Serial.printf("%d\n",input);
  return input;
}

uint8_t tickC::getIOs() {  // get ticks from IO-Device
  uint8_t input;
  // get inputs
  input = 1 - digitalRead(IN4);
  input = (input << 1) | (1 - digitalRead(IN3));
  input = (input << 1) | (1 - digitalRead(IN2));
  input = (input << 1) | (1 - digitalRead(IN1));
  input = (input << 1) | (1 - digitalRead(IN0));

  return input;
}


char tickC::getCharacter() {  // get input from IO-device

#ifndef TOUCH
  uint8_t input = getIOs();
#else
  uint8_t input = getTouch();
#endif

  if (input == previousinput) return (0);  // no finger change, dont wait

  if (input == 0) {  // all fingers up, dont wait
    previousinput = input;
    return (0);
  }

  if (tickMode & 1)
    if (input < previousinput) {  // only fingers removed, dont wait
      previousinput = input;
      return (0);
    }

  if (tickMode & 2) {      // old version (slower)
    unsigned long Millis;  // time
    previousinput = input;
    Millis = millis();

    do {  // wait for stable input
      delay(1);
#ifndef TOUCH
      input = getIOs();
#else
      input = getTouch();
#endif

      if (input != previousinput) {  // input changed?
        previousinput = input;       // reset input
        Millis = millis();           // reset timer
      }
    } while (millis() - Millis < stabTime);  // wait for input stabilization
  } else {
    delay(stabTime);  // wait stab time for all fingers to be placed

#ifndef TOUCH
    input = getIOs();  // read final input
#else
    input = getTouch();  // read final input
#endif

    previousinput = input;
  }

  if (input > 0) return tickChar[charSet][input];  // return tick character from current char set
  return 0;
}


void tickC::setCharacter(char c) {  // output character to IO-Device
  unsigned long currentMillis;
  unsigned long previousMillis = 0;  // last time
  int i;
  bool b0, b1, b2, b3, b4;

  for (i = 0; i < 32; i++)
    if (tickChar[charSet][i] == c) break;
  if (i == 32) {  // char not found
    Serial.print("?");
    return;
  }

  b0 = i & 1;
  b1 = i & 2;
  b2 = i & 4;
  b3 = i & 8;
  b4 = i & 16;

  uint8_t x = digitalRead(LED);
  if (x) digitalWrite(LED, 0);  // swith LED
  else digitalWrite(LED, 1);

  digitalWrite(OUT0, b0);  // pulse on
  digitalWrite(OUT1, b1);  // pulse on
  digitalWrite(OUT2, b2);  // pulse on
  digitalWrite(OUT3, b3);  // pulse on
  digitalWrite(OUT4, b4);  // pulse on
  previousMillis = millis();
  do {
    currentMillis = millis();
  } while (currentMillis - previousMillis < pulseDuration);  // wait interval time
  previousMillis = currentMillis;

  digitalWrite(OUT0, 0);  // pulse off
  digitalWrite(OUT1, 0);  // pulse off
  digitalWrite(OUT2, 0);  // pulse off
  digitalWrite(OUT3, 0);  // pulse off
  digitalWrite(OUT4, 0);  // pulse off

  digitalWrite(LED, x);
}


void tickC::lineCommand(String c) {  // process commands
  int pos;

  switch (c[0]) {  // process command
    case 's':
      stabTime = atoi((char*)&c[2]);
      Serial.printf("stabTime: %d\n", stabTime);
      break;
    case 'd':
      pulseDuration = atoi((char*)&c[2]);
      Serial.printf("pulseDuration: %d\n", pulseDuration);
      break;
    case 'x':
      outSpeed = atoi((char*)&c[2]);
      Serial.printf("outSpeed: %d\n", outSpeed);
      break;
    case 'o':
      for (int i = 2; i < c.length(); i++) {
        processTick(c[i], 7);
        delay(outSpeed);
      }
      break;  // output line on server only
    case 'r':
      sendWorld("Rebooting...\n", 4);
      ESP.restart();
      break;  // reset target
    case 'g':
      c[1] = '/';
      loadSettings((char*)&c[1]);
      break;  // load current configuration from file system
    case 'p':
      c[1] = '/';
      saveSettings((char*)&c[1]);
      Serial.printf("Config stored in %s\n", (char*)&c[1]);
      break;  // save current configuration in file system
    case 'c':
      pos = atoi((char*)&c[4]);  // set character in character set. Example: c ! 43
      if (pos / 32 < CHARSETNUM) {
        tickChar[pos / 32][pos % 32] = c[2];
        Serial.printf("Set %d, Pos %d: %c\n", pos / 32, pos % 32, c[2]);
      }
      break;
    case 't':
      tickMode = atoi((char*)&c[2]);
      Serial.printf(" tickMode: %d\n", tickMode);
      break;  // change tickMode
    case 'q':
      if (c.length() > 2) IN0 = atoi((char*)&c[2]);  // set IO-Ports Example q 15 04 18
      if (c.length() > 5) IN1 = atoi((char*)&c[5]);
      if (c.length() > 8) IN2 = atoi((char*)&c[8]);
      if (c.length() > 11) IN3 = atoi((char*)&c[11]);
      if (c.length() > 14) IN4 = atoi((char*)&c[14]);
      if (c.length() > 17) OUT0 = atoi((char*)&c[17]);
      if (c.length() > 20) OUT1 = atoi((char*)&c[20]);
      if (c.length() > 23) OUT2 = atoi((char*)&c[23]);
      if (c.length() > 26) OUT3 = atoi((char*)&c[26]);
      if (c.length() > 29) OUT4 = atoi((char*)&c[29]);
      pinMode(OUT0, OUTPUT);  // init IO-Pins
      pinMode(OUT1, OUTPUT);
      pinMode(OUT2, OUTPUT);
      pinMode(OUT3, OUTPUT);
      pinMode(OUT4, OUTPUT);
      pinMode(IN0, INPUT_PULLUP);
      pinMode(IN1, INPUT_PULLUP);
      pinMode(IN2, INPUT_PULLUP);
      pinMode(IN3, INPUT_PULLUP);
      pinMode(IN4, INPUT_PULLUP);
      previousinput = getIOs();  // init IO-State
      Serial.printf("In0 %2d In1 %2d In2 %2d In3 %2d In4 %2d  Out0 %2d Out1 %2d Out2 %2d Out3 %2d Out4 %2d\n", IN0, IN1, IN2, IN3, IN4, OUT0, OUT1, OUT2, OUT3, OUT4);
      break;
    case 'w': // change WiFi channel
      strlcpy(channel,(char*)&c[2],32);
      Serial.printf(" Cahnnel: %s\n", channel);
      if (!EEPROM.begin(256)) {// size in uint8_t
        Serial.println("Failed to initialise EEPROM");
      }
      else {
        EEPROM.writeULong(0,123456789);
        EEPROM.writeString(4, ssid);
        EEPROM.writeString(4+32, password);
        EEPROM.writeString(4+32+32, channel);
        EEPROM.end();
      }
      sendWorld("Rebooting...\n", 4);
      ESP.restart();
      break;  // change tickMode
  }
}

void tickC::tickCommand(char c) {  // process tick command
  uint8_t i = 0, u = 0;

  switch (c) {  // process command
    case 'w':
      if (ConnectMesh) {
        Serial.println("Connected to Mesh, switch to WiFi not possible");  // only lokal execution to be done
        break;
      }
      Serial.println("WiFi activated");  // do not use if client is part of mesh-network! only lokal execution to be done
      WiFiInitAccessPoint();
      break;
    case 'm':
      outMode = (outMode + 1);  // change outMode
      if (raiseHandMode) outMode = outMode % 3;
      else outMode = outMode % 2;
      Serial.printf(" outMode: %d ", outMode);
      break;
      //    case 'r': raiseHandMode = !raiseHandMode; // change raiseHandMode
      //              outMode = 2 * raiseHandMode; // no character output
      //              Serial.printf(" raiseHandMode %d ",raiseHandMode);
      //              break;
      //    case 'h': ; // allow next communication from raise hand to be implemented
      //              Serial.print(" raiseHand sheduler ");
      //              break;
    case 'n':
      sendWorld("\n", 4);  // send CR/LF to websocket
      tickLine = tickString.substring(0, tickString.length() - 2);
      tickString = "";
      dispText = "          ";  // empty line
      Serial.print(" Stored line: ");
      Serial.println(tickLine);
      break;
    case 'z':
      tickString = tickString.substring(0, tickString.length() - 2) + " ";  // add space character to text line
      if ((outMode == 0) || (outMode == 1)) sendWorld(" ", 4);              // output space to websocket
      break;
    case 'e':
      if (tickLine.length() > 2) {
        comMode = 0;
        tick->lineCommand(tickLine);
      }  // process tickLine without cr/lf
      break;

    default: Serial.print("?");  // command not found
  }
}


void tickC::processTick(char c, uint8_t send) {  // process tick input

  tickString += c;  // add it to the inputString:

  if ((outMode == 0) || ((outMode == 1) && (c != COMMANDCHAR) && (c != CHARSETCHAR) && (comMode == 0))) {  // send character to world if outmode = sendall (0) or no command and outMode = 1
    sendWorld(c, send);                                                                                    // output char to world
  }
  if (outMode < 2) switch (c) {  // communication alowed

      case COMMANDCHAR:
        if (charSet == 0) {               // normal char set -> normal command
          comMode++;                      // switch com mode on (=1) or lock com mode (=2)
          if (comMode == 3) comMode = 0;  // switch com mode off if previously locked
        } else charSetLock = 1;           // commandchar after charset change -> lock charset
        break;

      case CHARSETCHAR:
        if (charSetLock) {  // char set locked -> unlock and reset to normal charset
          charSetLock = 0;
          charSet = 0;
        } else charSet = (charSet + 1) % CHARSETNUM;  // loop through character sets
        Serial.printf(" CharSet %d ", charSet);
        break;

      default:
        if ((comMode > 0) && (outMode < 2)) tickCommand(c);  // process tick command if communication alowed
        if (comMode == 1) comMode = 0;                       // if not locked switch com mode back to normal mode
        if (charSetLock == 0) charSet = 0;                   // switch back to normal charset after every character if not locked
    }
  //   else { // raise hand mode
  //    Serial.printf(" raiseHand ");
  //    webSocket.sendTXT(">1"); // client raise hand
  //    outMode=0; // reset of outmode -> in future done by server! sheduler to be implemented.
  //   }
}

void tickC::display(String s) {
  if ((s[0] == 10) || (s[0] == 13)) dispText = "";  //newline
  else dispText += s;

  if (dispText.length() > 10) dispText = dispText.substring(dispText.length() - 10);

  oled.setCursor(0, 48);   // set position to display
  oled.println(dispText);  // set text
  oled.display();          // display on OLED
}

void tickC::sendWorld(String c, uint8_t send) {
  if (send & 2) {
    mesh.sendBroadcast(c);  // output to world
    mesh.update();
  }

  if ((ConnectType != 0) && (send & 4)) {
    String x = "mesh " + c;
    webSocket.broadcastTXT(x);
    webSocket.loop();  // process webSocket messages
  }
  Serial.print(c);  // output to serial
  display(c);
}

void tickC::sendWorld(char c, uint8_t send) {
  String s(c);
  if (send & 2) {
    mesh.sendBroadcast(s);  // output to world
    mesh.update();
  }
  display(s);
  if ((ConnectType != 0) && (send & 4)) {
    s = "mesh " + s;
    webSocket.broadcastTXT(s);
    webSocket.loop();  // process webSocket messages
  }

  if (send & 1) setCharacter(c);  // output char to tick device
  Serial.print(c);                // output to serial
}