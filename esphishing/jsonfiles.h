#include <ArduinoJson.h>
#include <FS.h> // Biblioteca para manipular SPIFFS

void determineMemory() {
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  total = fs_info.totalBytes;
  used = fs_info.usedBytes;
  freespace = fs_info.totalBytes - fs_info.usedBytes;
}

bool saveSettingsJSON() {
  StaticJsonDocument<500> jsonDoc;
  jsonDoc["whiteHat"] = whiteHat;
  jsonDoc["ssid"] = ssid;
  jsonDoc["password"] = password;
  jsonDoc["channel"] = channel;
  jsonDoc["hidden"] = hidden;
  jsonDoc["local_IP"] = local_IPstr;
  Serial.println(local_IPstr);
  jsonDoc["local_mac"] = local_MACDefault;
  jsonDoc["gateway"] = gatewaystr;
  Serial.println(gatewaystr);
  jsonDoc["subnet"] = subnetstr;
  Serial.println(subnetstr);
  jsonDoc["update_username"] = update_username;
  jsonDoc["update_password"] = update_password;
  jsonDoc["ftp_username"] = ftp_username;
  jsonDoc["ftp_password"] = ftp_password;
  jsonDoc["ftpenabled"] = ftpenabled;
  jsonDoc["esportalenabled"] = esportalenabled;

  File configFile = SPIFFS.open("/json/settings.json", "w");
  if (!configFile) {
    logging("saveSettingsJSON :: Failed to open file for writing");
    return false;
  }

  serializeJson(jsonDoc, configFile);
  configFile.close();
  logging("saveSettingsJSON :: Saving new settings");
  return true;
}

bool createDefaultsSettingsJSON() {
  logging("createDefaultsSettingsJSON :: Creating json config file");
  StaticJsonDocument<500> jsonDoc;
  jsonDoc["whiteHat"] = whiteHatDefault;
  jsonDoc["ssid"] = ssidDefault;
  jsonDoc["password"] = passwordDefault;
  jsonDoc["channel"] = channelDefault;
  jsonDoc["hidden"] = hiddenDefault;
  jsonDoc["local_IP"] = local_IPDefault;
  jsonDoc["local_mac"] = local_MACDefault;
  jsonDoc["gateway"] = gatewayDefault;
  jsonDoc["subnet"] = subnetDefault;
  jsonDoc["update_username"] = update_usernameDefault;
  jsonDoc["update_password"] = update_passwordDefault;
  jsonDoc["ftp_username"] = ftp_usernameDefault;
  jsonDoc["ftp_password"] = ftp_passwordDefault;
  jsonDoc["ftpenabled"] = ftpenabledDefault;
  jsonDoc["esportalenabled"] = esportalenabledDefault;

  File configFile = SPIFFS.open("/json/settings.json", "w");
  if (!configFile) {
    logging("createDefaultsSettingsJSON :: Failed to open file for writing");
    return false;
  }

  serializeJson(jsonDoc, configFile);
  configFile.close();
  logging("createDefaultsSettingsJSON :: json config file DONE");
  return true;
}

bool createAttackSettingsJSON() {
  logging("createDefaultsAttackJSON :: Creating json config file");
  StaticJsonDocument<500> jsonDoc;
  jsonDoc["whiteHat"] = whiteHatAttack;
  jsonDoc["ssid"] = ssidAttack;
  jsonDoc["password"] = passwordAttack;
  jsonDoc["channel"] = channelAttack;
  jsonDoc["hidden"] = hiddenAttack;
  jsonDoc["local_IP"] = local_IPAttack;
  jsonDoc["local_mac"] = local_MACAttack;
  jsonDoc["gateway"] = gatewayAttack;
  jsonDoc["subnet"] = subnetAttack;
  jsonDoc["update_username"] = update_usernameAttack;
  jsonDoc["update_password"] = update_passwordAttack;
  jsonDoc["ftp_username"] = ftp_usernameAttack;
  jsonDoc["ftp_password"] = ftp_passwordAttack;
  jsonDoc["ftpenabled"] = ftpenabledAttack;
  jsonDoc["esportalenabled"] = esportalenabledAttack;

  File configFile = SPIFFS.open("/json/settings.json", "w");
  if (!configFile) {
    logging("createDefaultsAttackJSON :: Failed to open file for writing");
    return false;
  }

  serializeJson(jsonDoc, configFile);
  configFile.close();
  logging("createDefaultsAttackJSON :: json config file DONE");
  return true;
}

bool loadSettingJSON() {
  logging("loadSettingJSON :: Opening config json file");
  File configFile = SPIFFS.open("/json/settings.json", "r");
  if (!configFile) {
    logging("loadSettingJSON :: No json config file - Calling loadDefaults()");
    createDefaultsSettingsJSON();
    configFile = SPIFFS.open("/json/settings.json", "r");
    if (!configFile) {
      logging("loadSettingJSON :: Failed to open file after creating defaults");
      return false;
    }
  }

  StaticJsonDocument<500> jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, configFile);
  if (error) {
    logging("loadSettingJSON :: Failed to parse JSON file");
    return false;
  }

  whiteHat = jsonDoc["whiteHat"];
  strlcpy(ssid, jsonDoc["ssid"] | "", sizeof(ssid));
  strlcpy(password, jsonDoc["password"] | "", sizeof(password));
  channel = jsonDoc["channel"];
  hidden = jsonDoc["hidden"];
  strlcpy(local_IPstr, jsonDoc["local_IP"] | "", sizeof(local_IPstr));
  strlcpy(local_MAC, jsonDoc["local_mac"] | "", sizeof(local_MAC));
  strlcpy(gatewaystr, jsonDoc["gateway"] | "", sizeof(gatewaystr));
  strlcpy(subnetstr, jsonDoc["subnet"] | "", sizeof(subnetstr));
  strlcpy(update_username, jsonDoc["update_username"] | "", sizeof(update_username));
  strlcpy(update_password, jsonDoc["update_password"] | "", sizeof(update_password));
  strlcpy(ftp_username, jsonDoc["ftp_username"] | "", sizeof(ftp_username));
  strlcpy(ftp_password, jsonDoc["ftp_password"] | "", sizeof(ftp_password));
  ftpenabled = jsonDoc["ftpenabled"];
  esportalenabled = jsonDoc["esportalenabled"];

  logging("loadSettingJSON :: config json file DONE");
  return true;
}

bool createSystemJSON() {
  logging("createSystemJSON :: Creating json index file");
  StaticJsonDocument<500> jsonDoc;
  JsonArray stationsArray = jsonDoc.createNestedArray("listofConnectStations");
  jsonDoc["local_IP"] = local_IPstr;
  jsonDoc["local_mac"] = local_MAC;
  jsonDoc["channel"] = channel;

  determineMemory();
  jsonDoc["total"] = total;
  jsonDoc["used"] = used;
  jsonDoc["freespace"] = freespace;

  int numStationsConnected = 0;
  Dir dir = SPIFFS.openDir("/connected/");
  while (dir.next()) {
    String filename = getFilename(dir.fileName(), '/', 2);
    stationsArray.add(filename);
    numStationsConnected++;
  }
  jsonDoc["numStationsConnected"] = numStationsConnected;

  File configFile = SPIFFS.open("/json/system.json", "w");
  if (!configFile) {
    logging("createSystemJSON :: Failed to open file for writing");
    return false;
  }

  serializeJson(jsonDoc, configFile);
  configFile.close();
  logging("createSystemJSON :: json config index DONE");
  return true;
}
