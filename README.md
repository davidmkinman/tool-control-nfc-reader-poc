
This Project is a proof of concept, not currently full functional and if currently working is liable to brake without warning

This is a framework for creating controllers for managing access control for doors and machines. These controllers are not designed to provide failsafe security or safety solutions, they are designed to maximise service availability.

The solution has the following hardware requirements:
  ESP32 
  PN532 (nfc)
  4x20 i2c LCD
  Mechanical relay/mosfet switch

The key features are:

  Works with 4 and 7 byte NFC tokens
  Allowed token list is maintained locally
  MQTT based communication with client authenticated TLS
  Compatible with AWS IOT

The source code has been broken up into a number of files:
  lcd.cpp   This is the code that manages what is displayed on the LCD
  ota.cpp   This code manages downloading and applying firmware updates to the esp32
  tokens.cpp This manages the PN532, using it to read tokens and checking those tokens against an approved list
  iotcomms.cpp This manages MQTT comms, hosts the functions that process MQTT messages, i.e. updated token lists 

  main.cpp This is where door/tool control takes place


  MQTT

  topics/cmds/getTokens/{deviceId}  send a msg to this queue to request token update
  topics/cmds/getConfig/{deviceId}  send §a msg to this queue to request a device config update

  subscribed topics

  This code subscribes to one topic, this is a limitation itation in the AWS IOT library implmentation for ardinuo, the subscribed topic is
  /devices/#

The call back routine handles the following subtopics

/devices/{deviceId}/ota       Carry out OTA firmware update
/devices/{deviceId}/tokens    Authorised token update topic
/devices/{deviceId}/config    Device configuration update topic


