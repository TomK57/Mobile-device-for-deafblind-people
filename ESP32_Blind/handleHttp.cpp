#include <Arduino.h>
#include <EEPROM.h>
#include "handleHttp.h"
#include <WebServer.h>
#include <LITTLEFS.h>

extern WebServer server;

extern char ssid[32];
extern char password[32];

File fsUploadFile;

/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0+sizeof(ssid), password);
  char ok[2+1];
  EEPROM.get(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0+sizeof(ssid), password);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

/** Handle root */
void handleRoot() {
  Serial.println(server.hostHeader());

  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.sendHeader(F("Pragma"), F("no-cache"));
  server.sendHeader(F("Expires"), "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("text/html"), ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head></head><body>"
    "<h1>DHS Access Point</h1>"
  );
  server.sendContent(
    "<p>1) <a href='/socket.html'>start websocket communication</a></p>"
    "<p>2) <a href='/index.html'>main page</a></p>"
    "<p>3) <a href='/wifi'>config wifi connection</a></p>"
    "</body></html>"
  );
  server.sendContent(""); // *** END ***
  server.client().stop(); // Stop is needed because we sent no content length

/*    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP())+"/index.html", true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
*/
}

/** Wifi config page handler */
void handleWifi() {
  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("text/html"), ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    String(F("<div style='margin-left:15px'><h3> You are connected through the wifi network: ")) + ssid);
  server.sendContent(
    String(F("<form method='POST' action='wifisave'><h3>Connect to other network:"))+
    String(F("<br><input type='text' placeholder='network' name='n'/>"))+
    String(F("<input type='password' placeholder='password' name='p'/>"))+
    String(F("<input class='button' type='submit' value='Connect'/></form>")));
  server.sendContent(
    String(F("<br><form method='POST' action='upload' enctype='multipart/form-data'>Upload file:"))+
    String(F("<br><input type='file' name='name'/>"))+
    String(F("<input class='button' type='submit' value='Upload'/>"))+
    String(F("</form></h3></div>")));
  server.sendContent(""); // *** END ***
  server.client().stop(); // Stop is needed because we sent no content length
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return F("text/html");
  else if (filename.endsWith(".css")) return F("text/css");
  else if (filename.endsWith(".js")) return F("application/javascript");
  else if (filename.endsWith(".ico")) return F("image/x-icon");
  else if (filename.endsWith(".gz")) return F("application/x-gzip");
  else if (filename.endsWith(".zip")) return F("application/zip");
  return F("text/plain");
}

void handleFileRead() { // send the right file to the client (if it exists)
  String path = server.uri();
  if (path.endsWith("/")) path += F("index.html");         // If a folder is requested, send the index file
  String contentType = getContentType(path);               // Get the MIME type
  String pathWithGz = path + F(".gz");
  if (LITTLEFS.exists(pathWithGz) || LITTLEFS.exists(path)) {  // If the file exists, either as a compressed archive, or normal

    //server.sendHeader(F("Cache-Control"), F("public, max-age=3600"));
    server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));

    if (LITTLEFS.exists(pathWithGz))                         // If there's a compressed version available
      path += F(".gz");                                    // Use the compressed verion
    File file = LITTLEFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println("Send file: " + path);
    return;
  }
  Serial.println("File Not Found: " + path);   // If the file doesn't exist, return false
  server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  return;
}

void handleFileUpload(){ // upload a new file to the LITTLEFS
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    if (upload.filename.length() > 30) {                          // Dateinamen auf 30 Zeichen kürzen
      int x = upload.filename.length() - 30;
      upload.filename = upload.filename.substring(x, 30 + x);
    }
    Serial.print(F("handleFileUpload Name: ")); 
    Serial.println(filename);
    fsUploadFile = LITTLEFS.open(filename, "w");            // Open the file for writing in LITTLEFS (create if it doesn't exist)
    filename = String();
  } 
  else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } 
  else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print(F("handleFileUpload Size: ")); 
      Serial.println(upload.totalSize);
      server.sendHeader("Location","index.html");              // Redirect the client to the wifi page
      server.send(303);
    } 
    else server.send(500, "text/plain", "500: couldn't create file");
  }
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
uint8_t i;
  
  Serial.println(F("wifi save"));
  server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("p").toCharArray(password, sizeof(password) - 1);
  server.sendHeader("Location", "wifi", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  if (strlen(ssid) > 0) { // Request WLAN connect with new credentials if there is a SSID
    WiFi.disconnect();
    WiFi.begin ( ssid, password );
    Serial.print(F("\nConnecting to "));
    Serial.print(ssid);
    delay(1000);
    for (i=0; i<20; i++) {
      if (WiFi.status() == WL_CONNECTED) break;
      Serial.print(".");
      delay(1000);
    }
  }
  if (i<20) { 
    saveCredentials();
    Serial.print(F("\nConnection Ok, new Credentials saved"));
  }
  else {
    ssid[0]=0;
    password[0]=0;
// tbd. open access point
    WiFi.begin("EasyBox-DB4716", "5EEA7B7DC"); // connect to network
    Serial.print(F("\nReconnecting to EasyBox-DB4716"));
    delay(1000);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }
  }
}
