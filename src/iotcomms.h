



void setupWifi(void);  
void setupAws(void);
void callBackHandler(char *, int , char *);
void loadConfig(void);
void requestConfigUpdate(void);
void writeConfigToFlash(void);
void loadTokens(void);
void requestTokenUpdate(void);
void writeTokensToFlash(void);
void iotcommLoop(void);
void iotcommSetup(void);


// WiFi and MQTT configuration 
static auto constexpr TOPIC_NAME = "$aws/things/ESP32-249F47A4AE30/shadow/update/documents";
