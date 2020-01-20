#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <AWS_IOT.h>
#include "aws_iot_log.h"
#include "config.h"
#include "iotcomms.h"
#include "tools.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ota.h>


WiFiMulti wifiMulti;
AWS_IOT awsIot;
#define FIRMWARE_URL "https://dl.bintray.com/inmandmk/esp32/slms-tool-control-nfc-unit/firmware_v_1.bin"


int status = WL_IDLE_STATUS;
bool startUp;

StaticJsonDocument<LOCAL_SETTING_BUFFER_SIZE> localSettings;
StaticJsonDocument<TOKEN_DOC_SIZE> tokens;
StaticJsonDocument<CONFIG_DOC_SIZE> config;
bool hasConfigChanged;
bool hasTokensChanged;
char deviceId[DEVICE_ID_LENGTH];
char deviceName[21];
extern int timeout;

extern int tick,msgCount,msgReceived;
extern int totalMsg;
extern char payload[16192];
extern char rcvdPayload[16192];
extern char msgTopic[128];


char baseSubTopic[128];
String baseTopic;

void setupWifi() {
    // extract list of wifi networks to connect too
    JsonArray wifiConfig = localSettings["wifi"].as<JsonArray>();
    for (int i=0; i<wifiConfig.size(); i++) {
        const char *wifiSsid = wifiConfig[i]["wifiSsid"];
        const char *wifiPassword = wifiConfig[i]["wifiPassword"];
        Serial.print("Added Ssid: ");
        Serial.println(wifiSsid);
        wifiMulti.addAP(wifiSsid, wifiPassword);
    }

    Serial.println("Connecting Wifi...");
    if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
}


void setupAws() {

  if (awsIot.connect( localSettings["mqttAddress"],
                      deviceId,
                      openFile(CA_FILENAME).c_str(),
                      openFile(CLIENT_CERT_FILENAME).c_str(),
                      openFile(CLIENT_KEY_FILENAME).c_str() ) == 0) {
    Serial.println("Connected to AWS");
    delay(2000);
    
    baseTopic = "devices/";
    baseTopic += deviceId;
    baseTopic +=  "/#";
    if ( 0 == awsIot.subscribe(baseTopic.c_str(),callBackHandler) ) {
      Serial.printf("Subscribe |%s| Successfull\n", baseSubTopic);
    } else {
      Serial.printf("Subscribe |%s| failed\n", baseSubTopic);
      Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
    }

  } else {
    Serial.println("AWS connection failed, Check the HOST Address");
    delay(1000);
    while(1);
  }
  delay(1000);
}

void callBackHandler(char *topicName, int payloadLen, char *payLoad) {
  // the topicname contains topicname plus the payload!
  char topic[128], curTopic[128];
  int curTopicLen;

  // handle the firemware update queue ota
  curTopicLen = sprintf(curTopic,"%s/%s/%s","devices",deviceId,"ota");
  strncpy(topic, topicName, curTopicLen);
  if (strcmp(topic, curTopic) == 0 ) {
    Serial.println("received OTA command");
    deserializeJson(config, payLoad, payloadLen);
    // String binMD5;
    String binURL = String( "http://dl.bintray.com/inmandmk/esp32/slms-tool-control-nfc-unit/firmware_v_1.bin");
    Serial.println("received OTA command");
    firmwareUpdate(binURL, 443);

  }

  // handle authorisation tokens queue
  curTopicLen = sprintf(curTopic,"%s/%s/%s","devices",deviceId,"tokens");
  strncpy(topic, topicName, curTopicLen);
  if (strcmp(topic, curTopic) == 0 ) {
    deserializeJson(tokens, payLoad, payloadLen);
    hasTokensChanged = true;
  }
  // handle device config queue
  curTopicLen = sprintf(curTopic,"%s/%s/%s","devices",deviceId,"config");
  strncpy(topic, topicName, curTopicLen);
  if (strcmp(topic, curTopic) == 0 ) {
    Serial.println("received config command");
    deserializeJson(config, payLoad, payloadLen);
    hasConfigChanged = true;
    strncpy(deviceName, (const char *)config["name"],20); 
  }



}

void loadConfig() {
  Serial.printf("Opening file: %s\r\n", CONFIG_FILE);

  File filePtr = SPIFFS.open(CONFIG_FILE);
  if(!filePtr || filePtr.isDirectory()){
    Serial.println("- failed to open file for reading");
    requestConfigUpdate();
    return;
  } 
  auto error = deserializeJson(config, filePtr);
  if (error) {
    Serial.print(F("deserializeJson() of config  failed with code \n"));
    Serial.println(error.c_str());
    return;
  }
  strncpy(deviceName, (const char *)config["name"],20); 
  filePtr.close();
}

void requestConfigUpdate() {
  char  ttopic[128];
  Serial.println("Send getConfigUpdate cmd");
  sprintf(ttopic,"%s/%s","topics/cmds/getConfig",deviceId);
  if ( awsIot.publish(ttopic,"{}" ) == 0) {
    Serial.printf("Get config %s suceeded\n", ttopic);
  } else {
    Serial.printf("Publish to %s failed\n", ttopic);
  } 
}

void writeConfigToFlash() {
  hasConfigChanged = false;
  //SPIFFS.remove(CONFIG_FILE);
  File filePtr=SPIFFS.open(CONFIG_FILE, FILE_WRITE);
  if(!filePtr || filePtr.isDirectory()){
    Serial.printf("- failed to open file %s for reading\n", CONFIG_FILE);
    deserializeJson(config, F("{\"id\":\"Unconfigured\",\"name\":\"    Unconfigured    \"}"));
  }
  if (serializeJson(config, filePtr)) {
    Serial.printf("Config File was written (%s)\n",CONFIG_FILE);
  } else {
    Serial.printf("File write failed (%s)\n",CONFIG_FILE);
  }
  filePtr.close();

}

void loadTokens() {
  Serial.printf("Opening file: %s\r\n", TOKEN_LIST);

  File filePtr = SPIFFS.open(TOKEN_LIST);
  if(!filePtr || filePtr.isDirectory()){
    Serial.println("- failed to open file for reading");
    requestTokenUpdate();
    return;
  }
  auto error = deserializeJson(tokens, filePtr);
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
  }
  filePtr.close();
  requestTokenUpdate();
}

void requestTokenUpdate() {
  char  topic[128];
  Serial.println("Send getTokenUpdate cmd");
  sprintf(topic,"%s/%s","topics/cmds/getTokens",deviceId);
  if ( awsIot.publish(topic,"{}" ) == 0) {
    Serial.printf("Get tokens %s suceeded\n", topic);
  } else {
    Serial.printf("Publish to %s failed\n", topic);
  } 
}

void writeTokensToFlash() {
  hasTokensChanged = false;
  //SPIFFS.remove(TOKEN_LIST);
  File filePtr=SPIFFS.open(TOKEN_LIST, FILE_WRITE);
  if(!filePtr || filePtr.isDirectory()){
    Serial.printf("- failed to open file %s for reading\n", TOKEN_LIST);
  }
  if (serializeJson(tokens, filePtr)) {
    Serial.printf("File was written (%s)\n",TOKEN_LIST);
  } else {
    Serial.printf("File write failed (%s)\n",TOKEN_LIST);
  }
  filePtr.close();

  Serial.println( (const char*) tokens["thingtype"]);
}



TaskHandle_t iotcommLoopHandle = NULL;

void iotcommLoop(void * pvParameters) {
  for (;;){
    if (wifiMulti.run() != WL_CONNECTED) {
      Serial.println("Wifi not connected, connecting");
      setupWifi();
      setupAws();
      delay(500);
      if ( startUp == true) {
        startUp = false;
        Serial.println("download config");
        loadConfig();
        loadTokens();
      }
    }
    delay(1000);
    if ( hasConfigChanged ) { Serial.println("lets write config.json"); writeConfigToFlash(); }
    if ( hasTokensChanged ) { Serial.println("lets write tokens.json"); writeTokensToFlash(); }

  }
}

void iotcommSetup() {
  hasConfigChanged = false;
  hasTokensChanged = false;
  snprintf(deviceId, DEVICE_ID_LENGTH, "ESP32-%04X%08X", (uint16_t)(ESP.getEfuseMac() >> 32), (uint32_t)ESP.getEfuseMac());
  loadConfig();
  loadTokens();
  xTaskCreatePinnedToCore(
    iotcommLoop,          /* Task function. */
    "iotcommLoop",        /* String with name of task. */
    5000,                 /* Stack size in bytes. */
    NULL,                 /* Parameter passed as input of the task */
    1,                    /* Priority of the task. */
    &iotcommLoopHandle,   /* Task handle. */
    1
  );                   /* Which ESP32 core */
  startUp = true;
}


