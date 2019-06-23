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



WiFiMulti wifiMulti;
AWS_IOT awsIot;

int status = WL_IDLE_STATUS;


extern StaticJsonDocument<LOCAL_SETTING_BUFFER_SIZE> localSettings;
extern StaticJsonDocument<SHADOW_BUFFER> shadowSettings;
extern StaticJsonDocument<TOKEN_DOC_SIZE> tokens;
extern StaticJsonDocument<CONFIG_DOC_SIZE> config;
extern bool hasConfigChanged;
extern bool hasTokensChanged;

extern int tick,msgCount,msgReceived;
extern int totalMsg;
extern char payload[16192];
extern char rcvdPayload[16192];
extern char msgTopic[128];
extern char deviceId[DEVICE_ID_LENGTH];
extern char deviceName[21];

char baseSubTopic[128];
String baseTopic;
// char topicGettokens[128];
// char topicReceiveConfig[128];
// char topicGetConfig[128];
// //char topicGetShadow[128];
// char topicReceiveShadowGet[128];
// char topicReceiveShadowUpdate[128];
// //char topicUpdateShadow[128];

void setupWifi() {
    // extract list of wifi networks to connect too
    JsonArray wifiConfig = localSettings["wifi"].as<JsonArray>();
    for (int i=0; i<wifiConfig.size(); i++) {
        const char *wifiSsid = wifiConfig[i]["wifiSsid"];
        const char ****REMOVED*** = wifiConfig[i]["***REMOVED***"];
        Serial.print("Added Ssid: ");
        Serial.println(wifiSsid);
        wifiMulti.addAP(wifiSsid, ***REMOVED***);
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

void callBackHandler(char *topicName, int payloadLen, char *payLoad) {
  // the topicname contains topicname plus the payload!
  char topic[128], curTopic[128];
  int curTopicLen;

  curTopicLen = sprintf(curTopic,"%s/%s/%s","devices",deviceId,"tokens");
  strncpy(topic, topicName, curTopicLen);
  if (strcmp(topic, curTopic) == 0 ) {
    deserializeJson(tokens, payLoad, payloadLen);
    hasTokensChanged = true;
  }
  curTopicLen = sprintf(curTopic,"%s/%s/%s","devices",deviceId,"config");
  strncpy(topic, topicName, curTopicLen);
  if (strcmp(topic, curTopic) == 0 ) {
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

  //Serial.println( (const char*) tokens["thingtype"]);
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

void shadowStateCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload,payLoad,payloadLen);
  strcpy(msgTopic,topicName);

  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
}

void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload,payLoad,payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
}


void setupAws() {

  if (awsIot.connect( localSettings["mqttAddress"],
                      deviceId,
                      openFile(CA_FILENAME).c_str(),
                      openFile(CLIENT_CERT_FILENAME).c_str(),
                      openFile(CLIENT_KEY_FILENAME).c_str() ) == 0) {
    Serial.println("Connected to AWS");
    delay(2000);

    // sprintf(topicReceiveShadowGet, "%s/%s/%s", "$aws/things", deviceId, "shadow/get/accepted");
    // if ( 0 == awsIot.subscribe(topicReceiveShadowGet, shadowStateCallBackHandler) ) {
    //   Serial.printf("Subscribe |%s| Successfull\n", topicReceiveShadowGet);
    // } else {
    //   Serial.printf("Subscribe |%s| failed\n", topicReceiveShadowGet);
    //   Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
    // }

    // sprintf(topicReceiveShadowUpdate, "%s/%s/%s", "$aws/things", deviceId, "shadow/update/accepted");
    // if ( 0 == awsIot.subscribe(topicReceiveShadowUpdate,shadowStateCallBackHandler) ) {
    //   Serial.printf("Subscribe |%s| Successfull\n", topicReceiveShadowUpdate);
    // } else {
    //   Serial.printf("Subscribe |%s| failed\n", topicReceiveShadowUpdate);
    //   Serial.println("Subscribe Failed, Check the Thing Name and Certificates");
    // }


        //snprintf(topicReceiveTokens, 128, "devices/%s/tokens", deviceId);
 
    
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

void updateShadowValues(char *shadow) {

    // Serial.printf("Opening file: %s\r\n", LOCAL_SETTINGS_FILENAME);

    // File file = SPIFFS.open(LOCAL_SETTINGS_FILENAME);
    // if(!file || file.isDirectory()){
    //     Serial.println("- failed to open file for reading");
    //     return ;
    // }
  auto error = deserializeJson(shadowSettings, shadow);
  if (error) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
      return;
  }
   // file.close();
}
