#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LOCAL_SETTINGS_FILENAME  "/localsettings.json"
#define LOCAL_SETTING_BUFFER_SIZE  1024
#define SHADOW_BUFFER  12000
#define TOKEN_LIST "/tokens.json"
#define TOKEN_DOC_SIZE 32384
#define CONFIG_FILE "/config.json"
#define CONFIG_DOC_SIZE 2048
#define MESSAGE_LIST "/messages.json"
#define MESSAGE_DOC_SIZE 2048

#define CA_FILENAME  "/ca.pem"
#define CLIENT_CERT_FILENAME  "/client.crt"
#define CLIENT_KEY_FILENAME  "/client-key.pem"

#define DEVICE_ID_LENGTH 19

/*format if its not setup*/
#define FORMAT_SPIFFS_IF_FAILED true



// LCD geometry
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

#define TOOL_OFF 1
#define TOOL_ON  0
#define TOOL_1_PIN       26  //d26
#define TOOL_2_PIN       25  //d25
#define OPEN_INTERVAL   12000
#define RF_OFF_INTERVAL  1000

#define USE_SOFTWARE_SPI   true
#define USE_HARDWARE_SPI   false
#define USE_HARDWARE_I2C   false
#define RESET_PIN        25  //d34  orange
#define SPI_CLK_PIN      18  //d18
#define SPI_MISO_PIN     19  //d19
#define SPI_MOSI_PIN     23  //d23
#define SPI_CS_PIN       05  //d5
  
#define I2C_SCL_PIN       22  //d22
#define I2C_SDA_PIN       21  //d21


enum cmdMode {
    TOOL_CONTROL = 1,   // dfault mode, controling tool power
    ADD_OPERATOR = 2,   // An inductor is adding new operators
    ADD_ADMIN    = 3    // An Admin is adding new inductors
};




#endif