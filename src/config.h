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

#define DOOR_1_PIN       26  //d26
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
#define EEPROM_SIZE  1984

struct kUser
{
    // Constructor
    kUser()
    {
        memset(this, 0, sizeof(kUser));
    }

    // Card ID (4 or 7 bytes), binary
    union 
    {
        uint64_t  u64;      
        byte      u8[8];
    } ID;
   

    // This byte stores eUserFlags (which door(s) to open for this user)
    byte u8_Flags;    
};

enum eUserFlags
{
    DOOR_ONE  = 1,
    ADMIN  = 2,
    SUPER = DOOR_ONE | ADMIN,
};

struct kCard
{
    byte     u8_UidLength;   // UID = 4 or 7 bytes
    byte     u8_KeyVersion;  // for Desfire random ID cards
    bool      b_PN532_Error; // true -> the error comes from the PN532, false -> crypto error
};

enum cmdMode {
    TOOL_CONTROL = 1,   // dfault mode, controling tool power
    ADD_OPERATOR = 2,   // An inductor is adding new operators
    ADD_ADMIN    = 3    // An Admin is adding new inductors
};

enum toolStates {
    TOOL_ON       = 1,   // device powered up
    TOOL_OFF      = 2,   // device powered off
    TOOL_DISABLED = 3    // device disabled by its admins
};



#endif