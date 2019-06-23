


void setupWifi(void);  // setup and configure wifi network using local settings
void setupAws(void);   // setup aws iot connectivity
void loadTokens(void); // loadup tokens
void writeTokensToFlash(void);
void loadConfig(void); // load config
void writeConfigToFlash(void);
void requestTokenUpdate(void); // request token list from cloud
void requestConfigUpdate(void); // request config from cloud
void shadowUpdateCallBackHandler (char, int, char );
void updateShadowValues(char*);


// WiFi and MQTT configuration 
static auto constexpr TOPIC_NAME = "$aws/things/ESP32-249F47A4AE30/shadow/update/documents";
