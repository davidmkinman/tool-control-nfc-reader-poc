#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "tools.h"
#include "config.h"


extern StaticJsonDocument<LOCAL_SETTING_BUFFER_SIZE> localSettings;
extern StaticJsonDocument<TOKEN_DOC_SIZE> tokens;
extern StaticJsonDocument<CONFIG_DOC_SIZE> config;
extern bool hasConfigChanged;
extern bool hasTokensChanged;

// Open file and return contents as a string object
String openFile( String filename) {
    String file;
    File filePtr = SPIFFS.open(filename);
    if(!filePtr || filePtr.isDirectory()){
        Serial.printf("- failed to open file %s for reading", filename.c_str());
        
    }
    file = filePtr.readStringUntil(0);
    filePtr.close();
    return file;
}


void loadConfigToGlobalVars( ) {
    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    Serial.printf("Opening file: %s\r\n", LOCAL_SETTINGS_FILENAME);

    File file = SPIFFS.open(LOCAL_SETTINGS_FILENAME);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return ;
    }
    
    auto error = deserializeJson(localSettings, file);
    if (error) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.c_str());
        return;
    }
    file.close();
}

