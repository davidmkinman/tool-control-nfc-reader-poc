#include "PN532.h"
#include "Classic.h"
#include "config.h"
#include "lcd.h"
#include <SPI.h>
#include <ArduinoJson.h>
#include <AWS_IOT.h>
#include "aws_iot_log.h"

// nfc
Classic nfc;
char cardInReader[15];   // UID of card in reader  
extern int timeout;
extern AWS_IOT awsIot;
extern char deviceId[DEVICE_ID_LENGTH];

enum cmdMode unitCmdMode = TOOL_CONTROL;  // what cmd is the unit processing
extern StaticJsonDocument<LOCAL_SETTING_BUFFER_SIZE> localSettings;
extern StaticJsonDocument<TOKEN_DOC_SIZE> tokens;
extern StaticJsonDocument<CONFIG_DOC_SIZE> config;


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
    //Serial.printf("%s -> %s\n", cardPerms, token);

    if (strcmp(token, cardInReader) == 0 ) {
      return true;
    }
  }
  return false;
}


void isAdmin() {}



void isInductor() {
  char inductorToken[15];
  StaticJsonDocument<500> payload;
  payload["authorisingtoken"]=cardInReader;
  payload["permission"]="operator";
  JsonArray newTokens = payload.createNestedArray("newtokens");
  strncpy(inductorToken, cardInReader, 15);
  if (unitCmdMode == TOOL_CONTROL) {
    unitCmdMode = ADD_OPERATOR;
    tokenMgmtInstScreen();
    delay(3000);
    char lastToken[15];
    strncpy(lastToken, cardInReader, 15);
 
    for (int i = 0; i < 10; i++) {

      // Lets Wait for a new token
      do {
        if (!readCard()) {
          presentNewTokenScreen();
          Serial.println("Waiting for Token");
        } else {
          removeTokenScreen();
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

      tokenAcceptedScreen();
      Serial.println("Token accepted");
      delay(3000);
    }
    inductionEndedScreen();
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
    waitingSplashScreen();
  }
  unitCmdMode = TOOL_CONTROL;

}





void isOperator() {
  unitCmdMode = TOOL_CONTROL;

  char currentOperator[15];
  strncpy(currentOperator, cardInReader, 15);
  //while (readCard() && strcmp(currentOperator,cardInReader) == 0) {
  //  Serial.println("Keep device on");
  //}
  timeout = 14;

}







TaskHandle_t tokensLoopHandle = NULL;

void tokensLoop(void * pvParameters) {
  for (;;){
    if (readCard()) {
      Serial.println(cardInReader);

      if (tokenPermCheck("admin")) {
        Serial.print("Admin Token present "); Serial.println(cardInReader);
        isAdmin();
      }
      else if (tokenPermCheck("inductor")) {
        Serial.print("Inductor Token present "); Serial.println(cardInReader);
        isInductor();
      }
      else if (tokenPermCheck("operator")) {
        Serial.print("Operator Token present "); Serial.println(cardInReader);
        isOperator();
      }
      else {
        Serial.print("Unauthrised Token Present "); Serial.println(cardInReader);
        UnauthrisedTokenScreen();
      }
    }
	delay(1000);

  }
}

void setupTokens() {
  nfc.InitSoftwareSPI(SPI_CLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_PIN, RESET_PIN);
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
  xTaskCreatePinnedToCore(
    tokensLoop,          /* Task function. */
    "tokensLoop",        /* String with name of task. */
    5000,                /* Stack size in bytes. */
    NULL,                /* Parameter passed as input of the task */
    1,                   /* Priority of the task. */
    &tokensLoopHandle,   /* Task handle. */
    1                    /* Which ESP32 core */
  );                  