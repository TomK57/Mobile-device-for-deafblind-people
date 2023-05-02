#ifndef DHSWiFi_h
#define DHSWiFi_h

#include <ESPAsyncWebServer.h>
#include "painlessMesh.h"  // Mesh-Network Library

//----------------------------to be changed for each project -------------------------------------------

// fallback WLAN credentials if not stored in EEPROM
#define DHSWIFI_SSID "FRITZ!Box 7530 PU"
#define DHSWIFI_PASSWORD "5EEA7B7DC"
#define DHSWIFI_CHANNEL "1"

// mesh network data
#define DEVICE_NAME "deafblind"  // own device for WLan or Access-Point
#define MESH_PREFIX "deafblind" // own mesh network name, also used for mesh FW-Update File-Name
#define MESH_PASSWORD "dnilbfaed" // mesh password
#define MESH_PORT 5555

// onboard LED port
#define LED 15 // 15 for ESP32S2 2 for ESP32

//--------------------------------------------------------------------------------------------------------
const char config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>DHS Filehandling</title>
</head>
<body>
 <div>
  <h2>DHS Filehandling</h2>
  <p>Flash Free: %FREESPIFFS% Used: %USEDSPIFFS% Total: %TOTALSPIFFS%</p>
  <p><h3>File content</h3></p>
  <h4><p>%FILELIST%</p></h4>
</div><div>
  <p><h3>File Upload</h3></p>
  <form method="POST" action="/upload" enctype="multipart/form-data"><input type="file" name="data" multiple /><input type="submit" name="upload" value="Upload" title="Upload File"></form>
  <p><h3>File Delete</h3></p>
  <form method="POST" action="/delete" enctype="multipart/form-data"><input type="number" name="number" min="-1" /><input type="submit" name="delete" value="Delete" title="Delete File"></form>
</div><div>
  <p><h3>FW-Update</h3></p>
  <form method="POST" action="/fwupdate" enctype="multipart/form-data"><input type="number" name="number" min="1" /><input type="submit" name="fwupdate" value="Client-Update" title="FW-Update"></form>
  <p><h3>Root FW-Upload</h3></p>
  <form method="POST" action="/rootupload" enctype="multipart/form-data"><input type="file" name="data"/><input type="submit" name="rootupdate" value="Root-Update" title="Upload Root-Firmware"></form>
</div><div>
  <p><h3>Set WLAN Credentials</h3></p>
  <form method="POST" action="/credentials" enctype="multipart/form-data">
  <label for="ssid">SSID:</label><br>
  <input type="text" name="ssid" /><br>
  <label for="password">Password:</label><br>
  <input type="password" name="password" /><br>
  <label for="channel">Chanel:</label><br>
  <input type="number" name="channel" min="1" max="13"/><br>
  <input type="submit" name="credentials" value="Save" title="Save Credentials"></form>
  <p><h3>Reset</h3></p>
  <form method="POST" action="/reset" enctype="multipart/form-data">
  <input type="submit" name="reset" value="Reset" title="Reset"></form>
</div>
</body>
</html>
)rawliteral";

extern char ssid[32];  // WLAN SSID
extern char password[32];      // WLAN Password
extern char channel[32];      // WLAN Password

String listFiles(bool ishtml = false);
String humanReadableSize(const size_t uint8_ts);
String processor(const String &var);

void initServer();
void handleRoot(AsyncWebServerRequest *request);
void FileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void RootUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void handleDelete(AsyncWebServerRequest *request);
void handleFWUpdate(AsyncWebServerRequest *request);
void handleCredentials(AsyncWebServerRequest *request);
void genFiles();
void drawGraph(AsyncWebServerRequest *request);

void meshCallback(char* Item, char* Data);

void initWifi();
void WiFiInitAccessPoint(void);
void WiFiLoop();
#endif
