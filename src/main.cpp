#include <AWS_IOT.h>
#include "aws_iot_log.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <SD.h>
#include <SPI.h>
#include <String.h>
#include <FS.h>
#include <SPIFFS.h>

#include "PN532.h"
#include "Classic.h"
#include "config.h"
#include "iotcomms.h"
#include "tools.h"
#include "lcd.h"
#include "tokens.h"



extern AWS_IOT awsIot;
extern StaticJsonDocument<LOCAL_SETTING_BUFFER_SIZE> localSettings;
extern StaticJsonDocument<TOKEN_DOC_SIZE> tokens;
extern StaticJsonDocument<CONFIG_DOC_SIZE> config;
extern char deviceId[DEVICE_ID_LENGTH];
extern char deviceName[21];

// mqtt
int tick=0,msgCount=0,msgReceived = 0;
int totalMsg = 0;
char msgTopic[128];
char payload[16192];
char rcvdPayload[16192];


// device

int        timeout;
bool       gb_InitSuccess  = false; // true if the PN532 has been initialized successfully
//int32_t    door_open_timer = 0;       // this is a count down timer at zero close the door +ve value opne it
bool       is_machine_enabled = false;  //this is the current state of the machine

void toolOn() {
  digitalWrite(TOOL_1_PIN, TOOL_ON);
}
void toolOff() {
  digitalWrite(TOOL_1_PIN, TOOL_OFF);
}


void loop() {
  // deal with tool state
  if (timeout > 0) {
    toolOn(); 
  } else {
    timeout = 0;
    toolOff();
  }

  // deal with lcd display
  if (timeout > 9) {
    toolOnSplashScreen();
  } else if (timeout < 1) {
    waitingSplashScreen();
  } else {
    countDownScreen(timeout);
    Serial.printf("Timing out %u seconds\n", timeout);
  }

  timeout--;
  delay(1000);
}

void setup() {
    pinMode(TOOL_1_PIN, OUTPUT);
    digitalWrite(TOOL_1_PIN, TOOL_OFF);
    //pinMode(TOOL_2_PIN, OUTPUT);
    //digitalWrite(TOOL_2_PIN, TOOL_OFF);
    int status;
    Serial.begin(115200);
    setupLcd();
    toolOff();
    Serial.println(deviceId);
    loadConfigToGlobalVars();
    iotcommSetup();
    setupTokens();
    const char* name =  config["name"];
    Serial.println("End of Setup");
    waitingSplashScreen();
}