#ifndef DHSWebsocket_h
#define DHSWebsocket_h

#include <WebSocketsServer.h>

#define DHS_MAX_PLUGINS 5

extern WebSocketsServer webSocket;
extern uint8_t DHSNumPlugins;
extern bool (*pluginCallbacks[DHS_MAX_PLUGINS])(uint8_t num, char* payload);

void startWebSocket();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

#endif