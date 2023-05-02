#include "DHSWiFi.h"

#include <LittleFS.h>
#include <DNSServer.h>  // DNS-Server for fallback Access-Point
#include <ESPmDNS.h>    // mDNS Server for name.local access of device
#include "painlessMesh.h"
#include "EEPROM.h"

#include "DHSWebsocket.h" // own websocket functions
#include "tick.h"         // own tick processing functions
#include <Adafruit_SSD1306.h>

#define OTA_PART_SIZE 1024  // How many uint8_ts to send per OTA data packet

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
painlessMesh mesh;        // Mesh-Network class
DNSServer dnsServer;

//default IP-Address
IPAddress myIP(10, 0, 0, 1);  // empty start IP-Address

//default WLAN network
char ssid[32] = DHSWIFI_SSID;  // WLAN SSID
char password[32] = DHSWIFI_PASSWORD;      // WLAN Password
char channel[32] = DHSWIFI_CHANNEL;      // WLAN Password

uint8_t ConnectType = 0;  // connection type 0= mesh client, 1= WLAN, 2=acess Point
uint8_t ConnectMesh = 0;  // status of mesh connection

SimpleList<uint32_t> nodes;

extern Scheduler userScheduler;  // to control own task
extern Adafruit_SSD1306 oled;

File f; // global define to allow continous mesh sw upload from opened f
std::shared_ptr<Task> FW_Update;

uint32_t startTime = 0;

extern tickC* tick;  // process tick class

void receivedCallback(uint32_t from, String& msg) { // message from mesh network
  if (msg.length() > 2) tick->lineCommand(msg); // if more than 1 character -> process line command
  else tick->processTick(msg.c_str()[0], 5); // process serial tick input without sending back to mesh-network

//  Serial.printf("Mesh %u: %s\n", from, msg);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("Mesh-Connection from %u\n", nodeId);

  nodes = mesh.getNodeList();
  digitalWrite(LED, nodes.size() > 0 );
  oled.setCursor(0, 16);       // set position to display
  oled.print(nodes.size()); // set connect number
  oled.print(" ");
  oled.display();              // display on OLED
      
  ConnectMesh=1;
}
void onDroppedConnectionCallback( uint32_t nodeId) {
  Serial.println("Mesh droped connection");
  nodes = mesh.getNodeList();
  digitalWrite(LED, nodes.size() > 0 );
  oled.setCursor(0, 16);       // set position to display
  oled.print(nodes.size()); // set connect number
  oled.print(" ");
  oled.display();              // display on OLED

  if ((ConnectType ==0 ) && (nodes.size() == 0)) { // all conections lost, restart (to avoid reconnect bug?)
    ESP.restart();
    // mesh.stop();
  }
}

void onChangedConnectionsCallback() {
  Serial.println("Mesh changed connection");
  nodes = mesh.getNodeList();
  digitalWrite(LED, nodes.size() > 0 );
  oled.setCursor(0, 16);       // set position to display
  oled.print(nodes.size()); // set connect number
  oled.print(" ");
  oled.display();       
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
//  Serial.println("Trying to Reconnect");
//  WiFi.begin(ssid, password);
}

void WiFiInitAccessPoint() {
  uint8_t i = 0;

  // activate IP-Connection for Root Module
  //****************************************
  Serial.printf("Try connecting to WLAN-Rooter %s\n",ssid);
  mesh.stationManual(ssid, password);
  mesh.setHostname(DEVICE_NAME);
  WiFi.setAutoReconnect(true);  
  WiFi.begin(ssid, password);  // start connection imediatly
   
  ConnectType = 1;

  do {
    digitalWrite(LED, 1);  // swith LED on
    mesh.update();
    if (WiFi.status() == WL_CONNECTED) break;
    Serial.print(".");
    delay(10);
    digitalWrite(LED, 0);  // swith LED off
    for (uint16_t u=0; u<99; u++) {
      mesh.update();
      delay(10);
    }
    i++;
  } while (i < 40); // try to connect for 60 seconds
  
  Serial.println();
    
  if (i == 40) {  // if no connection, switch to AP-mode
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(myIP, myIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(DEVICE_NAME,NULL,atoi(channel));
    ConnectType = 2;
    const uint8_t DNS_PORT = 53;
    dnsServer.start(53, "*", myIP);
  }
  else {
    myIP = IPAddress(mesh.getStationIP()); // get current IP
    MDNS.addService("http", "tcp", 80);
    if (!MDNS.begin(DEVICE_NAME)) {
      Serial.println("Error setting up MDNS responder!");
    } 
    else Serial.printf("mDNS responder started for %s.local\n", DEVICE_NAME);

    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  }

  mesh.setRoot(true);

  tick->tickClient=0; // mesh server with WiFi connection

  if (ConnectType == 1) Serial.printf("Connect to Web-Server %s.local or IP-Adress %s\n",DEVICE_NAME,myIP.toString().c_str());
  else Serial.printf("Connect to WiFi Acces-Point %s, call of any URL will be redirected to start-page (IP-Adress: %s)\n", DEVICE_NAME,myIP.toString().c_str());
}

// scan for wifi Networks in range
uint8_t scanNetworks() {
    uint8_t f_ssid = 0;
    uint8_t f_mesh = 0;
    
    int n = WiFi.scanNetworks();
    if (n == 0) Serial.println("no networks found");
    else {
        for (int i = 0; i < n; ++i) {
//            Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
//            Serial.printf("%4d", WiFi.RSSI(i));
//            Serial.printf("%2d", WiFi.channel(i));
          if (strcmp(WiFi.SSID(i).c_str(),ssid) == 0) f_ssid = WiFi.channel(i);
          if (strcmp(WiFi.SSID(i).c_str(),DEVICE_NAME) == 0) f_mesh = WiFi.channel(i);
        }
    }
    WiFi.scanDelete();

    if (f_mesh) { // mesh network found, connect to it's channel
      sprintf(channel,"%d",f_mesh);
      Serial.printf("Mesh Network found on Channel %d, connecting to it\n",f_mesh);
      return 0;
    }
    if (f_ssid) {
      sprintf(channel,"%d",f_ssid);
      if (f_ssid != atoi(channel)) Serial.println("Mesh default and WLan Rooter on different channels, WLan Rooter channel used to enable reconfiguration");
      Serial.printf("WLan Rooter found on Channel %d, open Mesh and WLan connection on this channel\n",f_ssid);
      return 1;     
    }
    Serial.printf("No Network found,open Mesh and Access-Point on default Channel %s\n",channel);
    return 2;
}


// init mesh network and WLAN
void initWifi() {
  WiFi.persistent(false); // don't write to flash
  
  if (!LittleFS.begin()) {
    Serial.println(F("File-System Error, try formating "));
    if (LittleFS.format()) {
      Serial.print(F(" Reinitializing file system "));
      if (!LittleFS.begin()) Serial.println(F("Error "));
    } else Serial.println(F("Error"));
  }

  if (!EEPROM.begin(256)) {// size in uint8_t
    Serial.println("Failed to initialise EEPROM");
  }
  else {
    if (EEPROM.readULong(0) != 123456789) Serial.println("No configuration stored, use defaults!");
    else {
      EEPROM.readString(4, ssid, 32);
      EEPROM.readString(4+32, password, 32);
      EEPROM.readString(4+32+32, channel, 32);
      EEPROM.end();      
    }         
  }

  scanNetworks(); // to do: continue with rigth network from scan
  
  //   mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(0);  // set before init() so that you can see startup messages

  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.initOTAReceive(MESH_PREFIX);
  mesh.onDroppedConnection(&onDroppedConnectionCallback);
  mesh.onChangedConnections(&onChangedConnectionsCallback);
  mesh.setContainsRoot(true);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, atoi(channel));
  
  Serial.printf("Current Mesh node Id is %u on channel %s\n",mesh.getNodeId(),channel);
  nodes = mesh.getNodeList();
 
  startTime = millis();
}

void WiFiLoop() {
  mesh.update();

  uint32_t now = millis();

/*  if ((ConnectMesh) && (now % 5000) == 0) { // check mesh connection every 5 seconds
    nodes = mesh.getNodeList();
    digitalWrite(LED, nodes.size() > 0 );
    if (nodes.size()) startTime = now; // connected than reset WiFi startTimer
    oled.setCursor(0, 16);       // set position to display
    oled.print(nodes.size()); // set connect number
    oled.print(" ");
    oled.display();              // display on OLED    
  }
*/
  if (ConnectType == 2) dnsServer.processNextRequest(); // process DNS-Server if startet as access point 

  if ((ConnectType == 0) && (nodes.size() == 0)) {
  
    if ((now - startTime) % 1000 < (500 + ConnectMesh * 300)) digitalWrite(LED, 1);  // swith LED on
    else   digitalWrite(LED, 0);  // swith LED off

    if ((now - startTime) > 40000) { // wait 40 Sec. or 5 Min. if mesh connection lost
      WiFiInitAccessPoint();
/*
      Serial.println("No mesh network found, open access-point");
      //// AP only tbd. Search for WiFi with SCAN on boot, than decide if rooter connection or AP (AP after rooter start not possible!)
      WiFi.softAPConfig(myIP, myIP, IPAddress(255, 255, 255, 0));
      //WiFi.softAP(DEVICE_NAME,NULL,atoi(channel));
      ConnectType = 2;
      const uint8_t DNS_PORT = 53;
      dnsServer.start(53, "*", myIP);
      startTime = now;
*/
    }
  }    
}

// initialize web server
void initServer() {
  server.begin();  // Web server start
 
  server.on("/", HTTP_GET, handleRoot); // redirect root access to index or update page
  server.on("/Config.html", HTTP_GET, [](AsyncWebServerRequest *request) { request->send_P(200, "text/html", config_html, processor); }); // config page from flash (works with empty filesystem)
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request)     { request->send_P(200, "text/html", config_html, processor); }, FileUpload); // file upload data transfer
  server.on("/delete", HTTP_POST, handleDelete); // file delete formdata
  server.on("/fwupdate", HTTP_POST, handleFWUpdate); // fw update data transfer
  server.on("/credentials", HTTP_POST, handleCredentials); // set WLAN credentials formdata
  server.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request) { request->redirect("/index.html"); delay(100); ESP.restart();}); // reset processor
  server.on("/rootupload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200); }, RootUpload); //  handle upload own root firmware file

  server.serveStatic("/", LittleFS, "/");  //.setCacheControl("max-age=31536000"); // serve files from filesystem (use browser cache)

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (LittleFS.exists("/index.html")) {
      request->redirect("/index.html");
    } 
    else request->send(404, "text/plain", "Not found");
  });
}

void handleRoot(AsyncWebServerRequest *request) {
  if (LittleFS.exists("/index.html")) request->redirect("/index.html"); // if index file exist go there
  else request->redirect("Config.html"); // go to upload page
}

// handles uploads
void FileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  String logmessage;

  if (!index) {
    logmessage = "Upload Start: " + String(filename);
    // open the file on first call and store the file handle in the request object
    request->_tempFile = LittleFS.open("/" + filename, "w");
    Serial.println(logmessage);
    WiFiLoop(); 
    webSocket.loop();     
  }

  if (len) {
    WiFiLoop(); 
    webSocket.loop();  
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
  }

  if (final) {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
    // close the file handle as the upload is now done
    request->_tempFile.close();
    Serial.println(logmessage);
  }
}

void RootUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    Serial.printf("Update: %s\n", filename.c_str());
    request->redirect("/index.html?Config");
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {  //start with max available size
      Update.printError(Serial);
    }
  }

  if (len) {
    WiFiLoop(); 
    webSocket.loop();  
    /* flashing firmware to ESP*/
    if (Update.write(data, len) != len) Update.printError(Serial);
  }

  if (final) {
    if (Update.end(true)) {  //true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", index + len);
      ESP.restart();
    } else Update.printError(Serial);
  }
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  String returnText = "";
  int counter = 1;
  File root = LittleFS.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) { // if ishtml: table html header
    returnText += "<table><tr> <th> Nr. </th> <th> Name </th> <th> Size </th> <th align='center'> &nbsp;";
//    returnText += " | &nbsp; </th> <th> Nr. </th> <th> Name </th> <th> Size </th>"; // 2. Spalte
    returnText+= "</tr>";
  }
  while (foundfile) { // loop through files on filesystem
    if (ishtml) { // if ishtml: generate table
//      if (counter % 2 == 1) returnText += "<tr>"; // 2. Spalte
      returnText += "<td>" + String(counter) + "</td><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
//      if (counter % 2 == 1) returnText += "<th align='center'> | </th>"; // 2. Spalte
//      else returnText+="</tr>"; // 2. Spalte 
      returnText+="</tr>"; // 1 Spalte
    } else { // build normal text list
      returnText += String(foundfile.name()) + "\n";
    }
    foundfile = root.openNextFile();
    counter++; // count files
  }
  if (ishtml) { // if ishtml: close table
    if (counter % 2 == 0) returnText+="</tr>";      
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t uint8_ts) {
  if (uint8_ts < 1024) return String(uint8_ts) + " B";
  else if (uint8_ts < (1024 * 1024)) return String(uint8_ts / 1024.0) + " KB";
  else if (uint8_ts < (1024 * 1024 * 1024)) return String(uint8_ts / 1024.0 / 1024.0) + " MB";
  else return String(uint8_ts / 1024.0 / 1024.0 / 1024.0) + " GB";
}

// delete file
void deleteFile(int number) {
  int counter = 1;
  File foundfile;
  File root = LittleFS.open("/");
  do { // search file with given number from list
    foundfile = root.openNextFile();
    if (foundfile && (number == counter)) {
      String logmessage = "/";
      logmessage += foundfile.name();
      foundfile.close();
      LittleFS.remove(logmessage);
      logmessage += " deleted";
      Serial.println(logmessage);
      break;
    }
    counter++;
  } while (foundfile);
  foundfile.close();
  root.close();
}

void handleDelete(AsyncWebServerRequest *request) {
  if (request->hasParam("number", true)) {
    if (f) { // if file is open from mesh FW-Update stop update and close file first
      Serial.println("FW-Update offer canceled");
      FW_Update->disable();
      f.close();
    }

    int nummer = request->getParam("number", true)->value().toInt();
    if (nummer > 0) deleteFile(nummer);
    if (nummer == -1) {
      LittleFS.format();
    }
  }
  if (LittleFS.exists("/index.html")) request->redirect("/index.html?Config"); // if index file exist go there
  else request->redirect("Config.html"); // go to upload page
}

void handleCredentials(AsyncWebServerRequest *request) {
  String ssid, password, channel;
  
  Serial.println("Credentials");
  if (request->hasParam("ssid", true)) {
    Serial.println("Save start");
    ssid = request->getParam("ssid", true)->value();
    if (ssid == "default") { // set back to defaults if given ssid = "default"
      ssid = DHSWIFI_SSID;  // WLAN SSID
      password = DHSWIFI_PASSWORD;  // WLAN Password
      channel = DHSWIFI_CHANNEL;      
    }
    else {
      password = request->getParam("password", true)->value();
      channel = request->getParam("channel", true)->value();
    }
    if (!EEPROM.begin(256)) {// size in uint8_t
      Serial.println("Failed to initialise EEPROM");
    }
    else {
      EEPROM.writeULong(0,123456789);
      EEPROM.writeString(4, ssid);
      EEPROM.writeString(4+32, password);
      EEPROM.writeString(4+32+32, channel);
      EEPROM.end();
      Serial.printf("Saved new SSID: %s and Password\n",ssid.c_str());
    }         
  }
  request->redirect("/index.html?Config");
}

String getFileName(int number) {
  int counter = 1;
  String Name;
  File root = LittleFS.open("/");
  File foundfile = root.openNextFile();
  while (foundfile) {
    if (number == counter) {
      Name = foundfile.name();
      foundfile.close();
      break;
    }
    counter++;
    foundfile = root.openNextFile();
  }
  root.close();
  foundfile.close();
  return Name;
}

void handleFWUpdate(AsyncWebServerRequest *request) {
  if (request->hasParam("number", true)) {
    int nummer = request->getParam("number", true)->value().toInt();
    if (nummer > 0) {
      String Name = "/";
      Name += getFileName(nummer);
      if (Name) {
        if (f) { // already startet before
          FW_Update->disable();
          f.close();
        }
        else mesh.initOTASend(  // not already startet
          [](painlessmesh::plugin::ota::DataRequest pkg,
             char *buffer) {
            Serial.printf("Send mesh FW-Update %d: %0.1f %%\n", pkg.from, (float) pkg.partNo *100.0 / f.size() * OTA_PART_SIZE);
            //fill the buffer with the requested data packet from the node.
            f.seek(OTA_PART_SIZE * pkg.partNo);
            f.readBytes(buffer, OTA_PART_SIZE);
//            if (pkg.partNo == ceil(((float)f.size()) / OTA_PART_SIZE)) mesh.sendBroadcast("FW-Update done "+pkg.from);
            //The buffer can hold OTA_PART_SIZE uint8_ts, but the last packet may
            //not be that long. Return the actual size of the packet.
            return min((unsigned)OTA_PART_SIZE,
                       f.size() - (OTA_PART_SIZE * pkg.partNo));
          },
          OTA_PART_SIZE);
          
        f = LittleFS.open(Name, "r");
        //Calculate the MD5 hash of the firmware we are trying to send. This will be used
        //to validate the firmware as well as tell if a node needs this firmware.
        MD5Builder md5;
        md5.begin();
        md5.addStream(f, f.size());
        md5.calculate();
        //Make it known to the network that there is OTA firmware available.
        //This will send a message every minute for an hour letting nodes know
        //that firmware is available.
        //This returns a task that allows you to do things on disable or more,
        //like closing your files or whatever.
        Name = Name.substring(1, Name.length());
        FW_Update = mesh.offerOTA(Name, "ESP32", md5.toString(),
                                  ceil(((float)f.size()) / OTA_PART_SIZE), false);
        Serial.print(Name);
        Serial.println(" Offer FW-Update aktive");
      }
    }
  }
  request->redirect("/index.html?Config");
}

String processor(const String &var) { // replace special strings with current data in HTML code
  if (var == "FILELIST") return listFiles(true);
  if (var == "FREESPIFFS") return humanReadableSize((LittleFS.totalBytes() - LittleFS.usedBytes()));
  if (var == "USEDSPIFFS") return humanReadableSize(LittleFS.usedBytes());
  if (var == "TOTALSPIFFS") return humanReadableSize(LittleFS.totalBytes());
  return String();
}
