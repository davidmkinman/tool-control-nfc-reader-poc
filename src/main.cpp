#include <AWS_IOT.h>
#include "aws_iot_log.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <SD.h>
#include <SPI.h>
#include <String.h>
#include <FS.h>
#include <SPIFFS.h>


#include "Wire.h" 
#include "hd44780.h"  // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

//#include <SPI.h>
#include "PN532.h"
#include "Classic.h"
#include "config.h"
#include "iotcomms.h"
#include "tools.h"

#define PN532DEBUG
#define PN532DEBUGPRINT Serial

extern AWS_IOT awsIot;
hd44780_I2Cexp lcd;  // lcd instance
extern StaticJsonDocument<LOCAL_SETTING_BUFFER_SIZE> localSettings;
extern StaticJsonDocument<SHADOW_BUFFER> shadowSettings;
extern StaticJsonDocument<TOKEN_DOC_SIZE> tokens;
extern StaticJsonDocument<CONFIG_DOC_SIZE> config;
extern bool hasConfigChanged;
extern bool hasTokensChanged;

// mqtt
int tick=0,msgCount=0,msgReceived = 0;
int totalMsg = 0;
char msgTopic[128];
char payload[16192];
char rcvdPayload[16192];
char deviceId[DEVICE_ID_LENGTH];


// nfc
Classic nfc;
char cardInReader[15];   // UID of card in reader  


// device
char        deviceName[21];
int         timeout;
bool       gb_InitSuccess  = false; // true if the PN532 has been initialized successfully
int32_t    door_open_timer = 0;       // this is a count down timer at zero close the door +ve value opne it
bool       is_machine_enabled = false;  //this is the current state of the machine
enum cmdMode unitCmdMode = TOOL_CONTROL;  // what cmd is the unit processing
byte       open_door  = LOW;
byte       close_door = HIGH;

// void InitReader(bool);
// bool ReadCard(byte, kCard);



bool readCard() {
  boolean success;
  byte uid[] = { 0, 0, 0, 0, 0, 0, 0 };	// Buffer to store the returned UID
  byte uidLength;				// Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  enum eCardType ecard = CARD_Unknown;
  bool cardPresent = true;
  success = nfc.ReadPassiveTargetID(&uid[0], &uidLength, &ecard);

  if (success) {
	// Wait 1 second before continuing
    int e;
    if (uidLength == 7) {
      e = sprintf(cardInReader,"%02X%02X%02X%02X%02X%02X%02X", uid[0],uid[1],uid[2],uid[3],uid[4],uid[5],uid[6]);
    } else if (uidLength == 4) {
      e = sprintf(cardInReader,"000000%02X%02X%02X%02X", uid[0],uid[1],uid[2],uid[3]);
    } else {
      strncpy(cardInReader,"None", 15);
      cardPresent = false;
    }
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
  nfc.SwitchOffRfField();
  return cardPresent;
}

bool tokenPermCheck(const char* cardPerms) {
  char token[15];
  for ( int i = 0; i < tokens["authorisedtokens"][cardPerms].size(); i++) {
    strncpy(token,tokens["authorisedtokens"][cardPerms][i], 15);
    Serial.printf("%s -> %s\n", cardPerms, token);

    if (strcmp(token, cardInReader) == 0 ) {
      return true;
    }
  }
  return false;
}
void toolOn() {

}
void toolOff() {

}

void toolState() {

}

void isAdmin() {}


void resetLcd() {
  lcd.clear();
  const char* deviceName =  config["name"];
  lcd.setCursor(0,0);lcd.print(deviceName);
}


void isInductor() {
  char inductorToken[15];
  StaticJsonDocument<500> payload;
  payload["authorisingtoken"]=cardInReader;
  payload["permission"]="operator";
  JsonArray newTokens = payload.createNestedArray("newtokens");
  strncpy(inductorToken, cardInReader, 15);
  if (unitCmdMode == TOOL_CONTROL) {
    unitCmdMode = ADD_OPERATOR;
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Adding New Operators");
    lcd.setCursor(0,1); lcd.print("Present new tokens  ");
    lcd.setCursor(0,2); lcd.print("then finish with    ");
    lcd.setCursor(0,3); lcd.print("inductor token      ");
    delay(3000);
    char lastToken[15];
    strncpy(lastToken, cardInReader, 15);
    lcd.clear(); 
    for (int i = 0; i < 10; i++) {

      // Lets Wait for a new token
      do {
        if (!readCard()) {
          lcd.setCursor(0,1); lcd.print("Waiting for Token   ");
          lcd.setCursor(0,2); lcd.print("Waiting for Token   ");
          Serial.println("Waiting for Token");
        } else {
          lcd.setCursor(0,1); lcd.print("Please Remove Token "); 
          lcd.setCursor(0,2); lcd.print("Please Remove Token ");
          Serial.println("Please Remove Token");
        }
      }  while (!readCard() || strcmp(lastToken, cardInReader) == 0);
      strncpy(lastToken, cardInReader, 15);
      // We have a new token, lets process it
      if (strcmp(cardInReader, inductorToken)  == 0) {
        // The inductor has ended the induction session
        break;
      }
      newTokens.add(cardInReader);

      lcd.clear(); 
      lcd.setCursor(0,1); lcd.print("Token Accepted      ");
      lcd.setCursor(0,2); lcd.print("Token Accpeted      ");
      Serial.println("Token accepted");
      delay(3000);
    }
    lcd.setCursor(0,1); lcd.print("End of Induction    "); 
    lcd.setCursor(0,2); lcd.print("End of Induction    ");
    Serial.println("End of Induction    ");

    // post the new tokens to AWS
    char  topic[128];
    char  payloadMsg[1024];
    serializeJson(payload, payloadMsg);
    Serial.println("Send newTokens cmd");
    sprintf(topic,"%s/%s","topics/cmds/newTokens",deviceId);
    Serial.println(topic);
    if (awsIot.publish(topic,payloadMsg) == 0)
    {        
        Serial.println("new Token push successful");
    }
    else
    {
        Serial.println("new Token push");
    }
    delay(2000);
    resetLcd();
  }
  unitCmdMode = TOOL_CONTROL;

}



const char * countDownLcd(int count) {
  String msg;
  int i = 0;
  while ( i++ < 10 ) {
    if ( i > count ) {
      msg += "  ";
    } else {
      msg += count;
      msg += count;
    }
  }
  lcd.setCursor(0,3); lcd.print(msg.c_str());
  lcd.setCursor(0,2); lcd.print(msg.c_str());
  //lcd.setCursor(0,1); lcd.print(msg.c_str());
  lcd.setCursor(0,1); lcd.print("No Token Turn Off in");
  lcd.setCursor(0,0);lcd.print(deviceName);

}


void isOperator() {
  unitCmdMode = TOOL_CONTROL;
  resetLcd();

  lcd.setCursor(0,1); lcd.print(" Power ON Power ON  ");
  lcd.setCursor(0,2); lcd.print(" Power ON Power ON  ");
  char currentOperator[15];
  strncpy(currentOperator, cardInReader, 15);
  while (readCard() && strcmp(currentOperator,cardInReader) == 0) {
    Serial.println("Keep device on");
  }
  timeout = 10;

}

void loop() {
  
  lcd.setCursor(0,0);lcd.print(deviceName);
  if ( hasConfigChanged ) { Serial.println("lets write config.json"); writeConfigToFlash(); }
  if ( hasTokensChanged ) { Serial.println("lets write tokens.json"); writeTokensToFlash(); }
  if (readCard()) {
    Serial.println(cardInReader);

    if (tokenPermCheck("admin")) {
      Serial.print("Admin Token present "); Serial.println(cardInReader);
        isAdmin();
    }
    if (tokenPermCheck("inductor")) {
      Serial.print("Inductor Token present "); Serial.println(cardInReader);
        isInductor();
    }
    if (tokenPermCheck("operator")) {
      Serial.print("Operator Token present "); Serial.println(cardInReader);
        isOperator();
    }
  }
	delay(1000);
  if ( timeout-- > 10 ) {
    Serial.printf("Timing out %u seconds\n", timeout);
  } else if ( timeout > 1 ) {
    countDownLcd(timeout);
    Serial.printf("Timing out %u seconds\n", timeout);
  } else if (timeout == 1) {
    Serial.println("turning off tool");
    resetLcd();
    lcd.setCursor(0,1); lcd.print("Power OFF Power OFF ");
    lcd.setCursor(0,2); lcd.print(" Waiting For Token  ");
    toolOff();
  } else {
    timeout = 0;
    toolOff();
  }

}

void setup() {
    int status;
    Serial.begin(9600);
    Wire.begin(21,22);

    snprintf(deviceId, DEVICE_ID_LENGTH, "ESP32-%04X%08X", (uint16_t)(ESP.getEfuseMac() >> 32), (uint32_t)ESP.getEfuseMac());
    Serial.println(deviceId);

    status = lcd.begin(LCD_COLS, LCD_ROWS);
    lcd.backlight();
    lcd.noLineWrap();
    lcd.setCursor(0,4);
    lcd.print(deviceId);

    hasConfigChanged = false;
    hasTokensChanged = false;
    loadConfigToGlobalVars();
    setupWifi();
    setupAws();
    loadConfig();
    loadTokens();

    nfc.InitSoftwareSPI(SPI_CLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_PIN, RESET_PIN);
    //nfc.InitHardwareSPI(5,34);
    nfc.begin();
    byte IC, VersionHi, VersionLo, Flags;
    if (nfc.GetFirmwareVersion(&IC, &VersionHi, &VersionLo, &Flags)) {
      Serial.printf( "Chip: PN5%02X, Firmware version: %d.%d\r\n", IC, VersionHi, VersionLo);
      Serial.printf( "Supports ISO 14443A:%s, ISO 14443B:%s, ISO 18092:%s\r\n", (Flags & 1) ? "Yes" : "No",
                                                                                (Flags & 2) ? "Yes" : "No",
                                                                                (Flags & 4) ? "Yes" : "No");
    }   
    // disable PN532 waiting forever for a card 
    if (!nfc.SetPassiveActivationRetries()) {
      Serial.println("SetPassiveActivationRetries Failed");
    }
  
    // configure board to read RFID tags
    if (!nfc.SamConfig()) {
      Serial.println("SamConfig Failed");
    }
    const char* name =  config["name"];
    lcd.setCursor(0,0);lcd.print(name);
    Serial.println("End of Setup");
    lcd.setCursor(0,1);lcd.print("Setup Ending         ");
    resetLcd();

}