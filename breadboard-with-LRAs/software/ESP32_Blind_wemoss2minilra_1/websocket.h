#ifndef websocket_h
#define websocket_h

#include <Arduino.h>
#include <WebSocketsServer.h>

extern WebSocketsServer webSocket;

void startWebSocket();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void webSendStatus(String s);
#endif
