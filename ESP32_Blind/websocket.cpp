#include <Arduino.h>
#include "websocket.h"

WebSocketsServer webSocket = WebSocketsServer(81);    // websocket server on port 81

extern long interval;  // interval at which to vibrate
extern int pulsecount; // pulsecount
extern int previousinput; // input state


char WS_Status=0; // WebSocket status

void webSendStatus(String s) { // send status message "! ...."
  s="! "+s; 
  webSocket.broadcastTXT(s.c_str());
  Serial.println(s);
}

void startWebSocket() { // Start a WebSocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  webSocket.begin();                          // start the websocket server
  Serial.println(F("WebSocket server started."));
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) { // When a WebSocket message is received

  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      WS_Status=0;
      break;
      
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        WS_Status=1;
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
      
    case WStype_TEXT:                     // if new text data is received
      if (length>=1) {
        String s="[";
        switch (payload[0]) { // process websocket command
          case '+': interval++;
                    webSocket.broadcastTXT("Interval " + String(interval)+ " Pulsecount "+ String(pulsecount)+"\n");
                    break;
          case '-': interval--; 
                    webSocket.broadcastTXT("Interval " + String(interval)+ " Pulsecount "+ String(pulsecount)+"\n");
                    break;
          case 'u': pulsecount++;  
                    webSocket.broadcastTXT("Interval " + String(interval)+ " Pulsecount "+ String(pulsecount)+"\n");
                    break;
          case 'd': pulsecount--; 
                    webSocket.broadcastTXT("Interval " + String(interval)+ " Pulsecount "+ String(pulsecount)+"\n");
                    break;
          case '*': // Tick
                    previousinput=2;
                    break;
          case 'R': // reset target
                    webSocket.broadcastTXT("Reboot...\n");
                    delay(1000);
                    ESP.restart();
                    break;
      }
    }
    break;
  }
}
