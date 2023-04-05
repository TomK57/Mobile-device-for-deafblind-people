#include "DHSWebsocket.h"
#include "tick.h"

WebSocketsServer webSocket = WebSocketsServer(81);  // websocket server on port 81

extern tickC* tick;

char WS_Status = 0;  // WebSocket status

uint8_t DHSNumPlugins = 0;
bool (*pluginCallbacks[DHS_MAX_PLUGINS])(uint8_t num, char* payload);

bool webSocketCallback(uint8_t num, char* payload) {  // websocket tick processing
  String s;  
    if (strncmp(payload,"get ",4)== 0) { // get command?
    int16_t p=4;
    while ((int16_t) strlen(payload)-p > 0) { // loop over requests
      char c=payload[p];
      switch (c) {
        case 's': s="s ";
                  s+=tick->stabTime;
                  webSocket.sendTXT(num,s.c_str(),s.length());
                  webSocket.loop();  // process webSocket messages
                  break;      
        case 'd': s="d ";
                  s+=tick->pulseDuration;
                  webSocket.sendTXT(num,s.c_str(),s.length());
                  webSocket.loop();  // process webSocket messages
                  break;      
        case 'x': s="x ";
                  s+=tick->outSpeed;
                  webSocket.sendTXT(num,s.c_str(),s.length());
                  webSocket.loop();  // process webSocket messages
                  break;      
      }
      p+=2;
    }      
    return true; // o further procesing of data
  }
  return false;  // continue processing for other callbacks
}

void startWebSocket() {               // Start a WebSocket server
  webSocket.onEvent(webSocketEvent);  // if there's an incomming websocket message, call function 'webSocketEvent'
  webSocket.begin();                  // start the websocket server
  pluginCallbacks[DHSNumPlugins] = webSocketCallback;  // set web socket callback for tick processing
  DHSNumPlugins=1;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {  // When a WebSocket message is received
  switch (type) { 
    case WStype_DISCONNECTED:  // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      WS_Status = 0; // remove connect status 
      break;
    case WStype_CONNECTED:
      { // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        WS_Status = 1; // set connect status
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }      
      break;
    case WStype_TEXT:  // if new text data is received, process command
        if (length>0) {
          payload[length]=0; // ad string terminator
          for (uint8_t i=0; i<DHSNumPlugins; i++) {
            if ( (*pluginCallbacks[i])(num, (char*)payload) ) break; // iterate through plugins until right plugin found          
          }
//          Serial.printf("Socket %d: %s\n", num, payload);
        }
      break;        
  }
}
