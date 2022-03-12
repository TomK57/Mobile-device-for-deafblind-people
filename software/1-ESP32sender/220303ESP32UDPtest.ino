/* 220303 Kr ESP32 send data via UPD protocol
 * 220304 Kr add touchpad function at pin D4 
 * 220306 Kr change esp32-hal-touch.c function touchRead, RTC_IO_TOUCH_CFG_REG, DRANGE 11 -> 01 = 1V  
 *           Changed thershold to 180 after some measurements
 * 22ß311 Kr changed touch count threshold to 2, was too unstable 
 */
#include <WiFi.h>
#include <WiFiUdp.h>

#define NAME "TJ"
#define PASS "593LK2459a27b7h%"
#define THRESHOLD 180 // 220306 Kr was 25, value increased after changing DRANGE and time_measurement values  
#define LOOPTIME 10 // ms
//#define TOUCHPIN1 4  // Touch0 GPIO4
#define TOUCHPIN1 32 // Touch9 GPIO32

const char * ssid = "";
const char * password = "";

unsigned int localPort = 45678;      // local port to listen on
unsigned int remoteport = 45678;
char  ReplyBuffer[] = "acknowledged";       // a string to send back
char buffer1[8];
unsigned long t1, t2, t3, t4;
int touch1, touch2, touch3;
int i;
bool pad;
unsigned long looptime;



// create instance
WiFiUDP Udp;

void setup() {
    Serial.begin(115200);
    //while(!Serial){} 
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed");
        while(1) {
            delay(1000);
        }
    }
    Serial.println("WiFi connected");
    //printWifiStatus();
    // start Udp object
    Udp.begin(localPort);
    i=1000;
    looptime = millis() + LOOPTIME;

}

void loop() {
  // put your main code here, to run repeatedly:
  // send a reply, to the IP address and port that sent us the packet we received

    Serial.print("start loop: ");Serial.println(millis());

    t4 = micros();
    //Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    //Udp.beginPacket("192.168.1.140",remoteport);
    Udp.beginPacket("192.168.1.255",remoteport); // broadcast
    if(checkTouch()) {
      Udp.write(0x01);
      Serial.println("checkTouch: true");
    }
    else {
      Udp.write(0x00);
      Serial.println("checkTouch: false");
    }
    Udp.write(0x20); // send a space character 
    /* 
     *  convert touch values to ASCII characters
     */
    sprintf(buffer1,"%4d",touch1);
    /*
     * data for transmission
    */
    //radiopacket[39] = 0; // string end, the 20th character
    // copy char string to array of uint8_t
    uint8_t txpacket[sizeof(buffer1)];
    int i;
    for(i = sizeof(buffer1)-1;i>=0;i--) {
      // index runs from 39 to 0, not 40 to 1
      txpacket[i] = buffer1[i];
    }
    Udp.write(txpacket, sizeof(txpacket));
    //Udp.write(0x44);    
    //Udp.write(0x45);    
    //Udp.write(0x46);    
    Udp.endPacket();
    t4 = micros() - t4; 
#if 0    
    debugTouch();
#endif
    Serial.print("UDP transmit time [us]: "); Serial.println(t4);
    
    Serial.print("end loop: ");Serial.println(millis());
    while(millis() < looptime)
      ;
    looptime = looptime + LOOPTIME;  
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

bool checkTouch(){
    int touchCount = 0;
    t1 = micros();
    touch1 = touchRead(TOUCHPIN1);
    if(touch1 <= THRESHOLD) touchCount++; // 220304 Kr get value of Touch 9 GPIO32 - Touch 0 pin = GPIO 4
    t1 = micros() - t1;
    t2 = micros();
    touch2 = touchRead(TOUCHPIN1);
    if(touch2 <= THRESHOLD) touchCount++; 
    t2 = micros() - t2;
    t3 = micros();
    touch3 = touchRead(TOUCHPIN1);
    if(touch3 <= THRESHOLD) touchCount++;
    t3 = micros() - t3; 
    if(touchCount >= 2) return(true);
    else return(false);
}

void debugTouch(){
    //Serial.print(i);
    Serial.print("\t");
    Serial.print(touch1);    
    Serial.print("\t"); // or Serial.print(" ")
    Serial.print(t1);
    Serial.print("\t"); // or Serial.print(" ")
    Serial.print(touch2);    // get value of Touch 0 pin = GPIO 4
    Serial.print("\t"); // or Serial.print(" ")
    Serial.print(t2);
    Serial.print("\t"); // or Serial.print(" ")
    Serial.print(touch3);    // get value of Touch 0 pin = GPIO 4
    Serial.print("\t"); // or Serial.print(" ")
    Serial.println(t3);
}
