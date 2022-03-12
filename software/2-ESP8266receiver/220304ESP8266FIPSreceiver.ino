/* 220304 Kr ESP8266 FIPS UDP receiver and beeper
 *  
 */
//#include <WiFi.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define NAME "TJ"
#define PASS "593LK2459a27b7h%"
#define LOOPTIME 10
#define BUZZERPIN 16
#define PACKETSIZE 32

const char * ssid = "";
const char * password = "";
unsigned int localPort = 45678;      // local port to listen on
unsigned int remoteport = 45678;
char packetBuffer[PACKETSIZE]; //buffer to hold incoming packet
unsigned long looptime;

// create instance
WiFiUDP Udp;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //while(!Serial){;}
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("WiFi Failed");
      while(1) {
          delay(1000);
      }
  }
  Udp.begin(localPort);

  pinMode(BUZZERPIN, OUTPUT);
  digitalWrite(BUZZERPIN, LOW);
#if 1  
  Serial.println("WiFi connected");
#endif

  looptime = millis() + LOOPTIME;
}

void loop() {
  // put your main code here, to run repeatedly:
  // if there's data available, read a packet

  //digitalWrite(BUZZERPIN, HIGH);
  
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remoteIp = Udp.remoteIP();
    Serial.print(remoteIp);
    Serial.print(", port ");
    Serial.println(Udp.remotePort());
    // read the packet into packetBufffer
    int len = Udp.read(packetBuffer, PACKETSIZE);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
    if(packetBuffer[0] == 0) beep(0);
    else beep(1);
    
    //Serial.println("Contents:");
    //Serial.println(packetBuffer);
  }  
  while(millis() < looptime)
    ;
  looptime = looptime + LOOPTIME;  
}

void beep(int state){
    if(state == 0) digitalWrite(BUZZERPIN, LOW);
    else digitalWrite(BUZZERPIN, HIGH);
}
