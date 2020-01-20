#include "config.h"
#include "lcd.h"
#include "Wire.h"
#include "ArduinoJson.h"
#include "hd44780.h"  // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

hd44780_I2Cexp lcd;  // lcd instance

extern char deviceId[DEVICE_ID_LENGTH];
extern char deviceName[21];
extern StaticJsonDocument<CONFIG_DOC_SIZE> config;


void setupLcd() {
  Wire.begin(21,22);
  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.backlight();
  lcd.noLineWrap();
  lcd.setCursor(0,4);
  lcd.print(deviceId);

}

void waitingSplashScreen() {
  lcd.setCursor(0,0); lcd.print(deviceName);
  lcd.setCursor(0,1); lcd.print(" waiting for token  ");
  lcd.setCursor(0,2); lcd.print(BLANK_LINE);
  lcd.setCursor(0,3); lcd.print(BLANK_LINE);
}

void toolOnSplashScreen() {
  lcd.setCursor(0,0); lcd.print(deviceName);
  lcd.setCursor(0,1); lcd.print("   Authorsed Token   ");
  lcd.setCursor(0,2); lcd.print("      Powered On     ");
  lcd.setCursor(0,3); lcd.print(BLANK_LINE);
}

void UnauthrisedTokenScreen() {
  lcd.setCursor(0,0); lcd.print(deviceName);
  lcd.setCursor(0,1); lcd.print("   Unathorsed Token  ");
  lcd.setCursor(0,2); lcd.print(BLANK_LINE);
  lcd.setCursor(0,3); lcd.print(BLANK_LINE);
}


void countDownScreen(int count) {
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
  lcd.setCursor(0,1); lcd.print(" No Token  No Token ");
  lcd.setCursor(0,2); lcd.print(msg.c_str());
  lcd.setCursor(0,1); lcd.print(msg.c_str());
  lcd.setCursor(0,0); lcd.print(deviceName);
}

void tokenMgmtInstScreen() {
  lcd.setCursor(0,0); lcd.print("Adding New Operators");
  lcd.setCursor(0,1); lcd.print("Present new tokens  ");
  lcd.setCursor(0,2); lcd.print("then finish with    ");
  lcd.setCursor(0,3); lcd.print("inductor token      ");
}

void presentNewTokenScreen() {
  lcd.setCursor(0,0); lcd.print(BLANK_LINE);
  lcd.setCursor(0,1); lcd.print("Present new token   ");
  lcd.setCursor(0,2); lcd.print("Present new token   ");
  lcd.setCursor(0,3); lcd.print(BLANK_LINE);
}

void removeTokenScreen() {
  lcd.setCursor(0,0); lcd.print(BLANK_LINE);
  lcd.setCursor(0,1); lcd.print("Remove token        ");
  lcd.setCursor(0,2); lcd.print("Remove token        ");
  lcd.setCursor(0,3); lcd.print(BLANK_LINE);
}

void tokenAcceptedScreen() {
  lcd.setCursor(0,0); lcd.print(BLANK_LINE);
  lcd.setCursor(0,1); lcd.print("Token Accepted      ");
  lcd.setCursor(0,2); lcd.print("Token Accepted      ");
  lcd.setCursor(0,3); lcd.print(BLANK_LINE);
}

void inductionEndedScreen() {
  lcd.setCursor(0,0); lcd.print(BLANK_LINE);
  lcd.setCursor(0,1); lcd.print("Induction Ended     ");
  lcd.setCursor(0,2); lcd.print("Induction Ended     ");
  lcd.setCursor(0,3); lcd.print(BLANK_LINE);
}
