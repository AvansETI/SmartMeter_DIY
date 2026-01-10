/*-------------------------------------------------------------------------
  The MIT License (MIT)
  Copyright © 2025 Avans Hogeschool Lectoraat Smart Energy
  
  Permission is hereby granted, free of charge, to any person obtaining a 
  copy of this software and associated documentation files (the “Software”), 
  to deal in the Software without restriction, including without limitation 
  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the 
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in 
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
  THE SOFTWARE.
  
  -------------------------------------------------------------------------
  
  Arduino sketch to mqtt Dutch Smart Meter P1 datagrams.

  See for more information: https://github.com/AvansETI/SmartMeter_DIY.
  Flash latest firmware version for the Wemos D1 mini lite (ESP8266): https://avanseti.github.io/SmartMeter_DIY/.

  V1.0: Initial: dkroeske(dkroeske@gmail.com), august 2019
  V1.1: Reroute pinning PCB, updated bootsequence
  V1.2: Updated to latest version ArduinoJson library (feb 2020)
  V1.3: Updated to latest PubSubClient (jan 2021)
  V1.4: Changed server location (sendlab.nl), removed credentials
  V1.5: Added webserver to get insight into the smartmeter readings, added
        TCP/IP service (port 3141) to get actual P1 message and fixed mDNS
        so devices can be found by diy_smartmeter.local on your network (ms: jan 2025)
  V1.6: Migrated from SPIFSS (deprecated) to LittleFS and changed WiFiServer.availble (deprecated) to .accept. 
        Migrated ArduinoJSON 6 to 7: https://arduinojson.org/v7/how-to/upgrade-from-v6/
        Added extra information to serial about the mDNS service and updated libraries.
  V1.7: Improved TCP data server and added the configuration for more than one client to connect.
  V2.0: Adapted the source code to be compiled for the Wemos S2 mini (Lolin S2 mini / ESP32S2) as well the current ESP32S2.
        Added the option to anonimize (zero all equipment IDs) of the P1 data that is send to the MQTT server and TCP clients.
        Removed EMON and ETI wordings and go for consistent DIY_SMARTMETER.
        Implemented the dashboard, p1 data server and hardware functionality into seperate library files for readability.

  Installation Arduino IDE:
  - How to get the Wemos installed in the Ardiuno IDE: https://siytek.com/wemos-d1-mini-arduino-wifi/
  - Install library WiFiManager by tablatronics: https://github.com/tzapu/WiFiManager
  - Install library JsonArduino by Banoit Blanchon: https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
  - Install library knolleary/PubSubClient by Nick O'Leary: https://github.com/knolleary/pubsubclient
 
  Happy Coding
  -------------------------------------------------------------------------*/
#define VERSION "2.0"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>

#elif defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#endif

// All includes that are used for both ESP8266 and ESP32
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <PubSubClient.h>

#include "config.h" // Configuration parameters
#include "include/hardware.hpp"
#include "include/dashboard.hpp"
#include "include/p1dataserver.hpp"

/************************************************************************************/
/************************************************************************************/
/* DEFINES                                                                          */
/************************************************************************************/
/************************************************************************************/
#define P1_TELEGRAM_SIZE           2048 // Can be deleted?
#define P1_MAX_DATAGRAM_SIZE       2048

#define DEFAULT_P1_BAUDRATE        "115200"
#define DEFAULT_MQTT_USERNAME      "smartmeter"
#define DEFAULT_MQTT_PASSWORD      "se_smartmeter"
#define DEFAULT_MQTT_TOPIC         "smartmeter/raw"
#define DEFAULT_MQTT_REMOTE_HOST   "mqtt.sendlab.nl"
#define DEFAULT_MQTT_REMOTE_PORT   "11883"
#define DEFAULT_MQTT_ANONIMIZE_P1  "1"
#define DEFAULT_TCP_ANONIMIZE_P1   "1"

// Do not know where these come from!
#define MQTT_MSGBUF_SIZE          2048
#define MQTT_RETRY_TIMEOUT        60000
#define MQTT_TOPIC_UPDATE_RATE_MS 20000

/************************************************************************************/
/************************************************************************************/
/* DEBUGGING                                                                        */
/************************************************************************************/
/************************************************************************************/
#define DEBUG // Comment to disable debug messages

#ifdef DEBUG
 #ifdef ESP8266
  #define DEBUG_PRINTF(format, ...) (Serial1.printf(format, __VA_ARGS__)) // ESP8266 Serial1 is used
 #else // ESP32
  #define DEBUG_PRINTF(format, ...) (Serial.printf(format, __VA_ARGS__)) // ESP32 Serial0 is used (multiple serials)
 #endif
#else
 #define DEBUG_PRINTF
#endif

/************************************************************************************/
/************************************************************************************/
/* HARDWARE PIN CONFIGURTION                                                        */
/************************************************************************************/
/************************************************************************************/
#if defined(ESP8266)
//
// WeMos  ESP8266 Use Warning
// D0     GPIO16
// D1     GPIO5   SCL
// D2     GPIO4   SDA
// D3     GPIO0   Must be PULLED HIGH during boot (Pulled up on WeMos board)
// D4     GPIO2   Must be PULLED HIGH during boot (Pulled up on WeMos board)
// D5     GPIO14  SCL
// D6     GPIO12  MISO  
// D7     GPIO13
// D8     GPIO15  Boot mode, must be LOW during flash boot
// A0             Analog

#define RST_PIN         D2  // Wemos D2 (GPIO4)
#define RGB_R_PIN       D6  // Wemos D6 (GPIO12)
#define RGB_G_PIN       D1  // Wemos D1 (GPIO5)
#define RGB_B_PIN       D5  // Wemos D5 (GPIO14)

#elif defined(ESP32)
//
// WeMos  ESP32S2 Use Warning (pin compatible with Wemos D1 mini lite)
// IO15           Onboard led
// IO18           Onboard pull-up
// 
// Pin mapping (only outside pins listed)
// WEMOS D1 mini	WEMOS S2 mini
// RST        		EN
// A0		          3
// D0		          5
// D5		          7
// D6		          9
// D7		          11
// D8		          12
// 3V3		        3V3
// TX		          39
// RX		          37
// D1		          35
// D2		          33
// D3		          18
// D4		          16
// GND		        GND
// 5V		          VBUS
//
#define RST_PIN         33 // Wemos GPIO33
#define RGB_R_PIN       9  // Wemos GPIO9
#define RGB_G_PIN       35 // Wemos GPIO35
#define RGB_B_PIN       7  // Wemos GPIO7
#define SM_RXD          11 // Wemos GPIO11
#endif

/************************************************************************************/
/************************************************************************************/
/* APPLICATION CONFIGURATION                                                        */
/************************************************************************************/
/************************************************************************************/
#define MQTT_USERNAME_LENGTH       32
#define MQTT_PASSWORD_LENGTH       32
#define MQTT_ID_TOKEN_LENGTH       64
#define MQTT_TOPIC_STRING_LENGTH   64
#define MQTT_REMOTE_HOST_LENGTH    128
#define MQTT_REMOTE_PORT_LENGTH    10
#define P1_BAUDRATE_LENGTH         10
#define MQTT_ANONIMIZE_P1_LENGTH   32
#define TCP_ANONIMIZE_P1_LENGTH    32

typedef struct {
  char     mqtt_username[MQTT_USERNAME_LENGTH];
  char     mqtt_password[MQTT_PASSWORD_LENGTH];
  char     mqtt_id[MQTT_ID_TOKEN_LENGTH];
  char     mqtt_topic[MQTT_TOPIC_STRING_LENGTH];
  char     mqtt_remote_host[MQTT_REMOTE_HOST_LENGTH];
  char     mqtt_remote_port[MQTT_REMOTE_PORT_LENGTH];
  char     p1_baudrate[P1_BAUDRATE_LENGTH];
  char     mqtt_anonimize_p1[MQTT_ANONIMIZE_P1_LENGTH];
  char     tcp_anonimize_p1[TCP_ANONIMIZE_P1_LENGTH];
} APP_CONFIG_STRUCT;

/************************************************************************************/
/************************************************************************************/
/* GLOBAL VARIABLES                                                                 */
/************************************************************************************/
/************************************************************************************/
uint32_t cur=0, prev=0;      // Maybe can be deleted?
bool shouldSaveConfig;       // Maybe can be deleted?

APP_CONFIG_STRUCT appConfig; // Application config variable

WiFiManager wifiManager;     // Wi-Fi manager for easy configuration
WiFiClient mqttWifiClient;   // Wi-Fi client to connect to a Wi-Fi access point

PubSubClient mqttClient("", 0, mqttWifiClient); // Only with some dummy values seems to work ... instead of mqttClient();
uint32_t mqttTimer = 0;      // Timer used to reconnect to the mqtt server, when disconnected (#26)
char mqtt_topic[128];        // Can be deleted?

char p1_buf[P1_MAX_DATAGRAM_SIZE]; // Datagram P1 buffer, Complete P1 telegram
char *p1;

Dashboard dashboard;         // Dashboard
P1DataServer p1DataServer;   // P1 Data Server

/************************************************************************************/
/************************************************************************************/
/* FUNCTIONS                                                                        */
/************************************************************************************/
/************************************************************************************/

/************************************************************************************/
void smartLedInit () {
/* 
short:   Initialize by set the led off.
inputs:  -
outputs: -
notes:   Led is not smart, so you know :).
/************************************************************************************/
  digitalWrite(RGB_R_PIN, 1);
  digitalWrite(RGB_G_PIN, 1);
  digitalWrite(RGB_B_PIN, 1);
}

/************************************************************************************/
void hardwareSetup () {
/* 
short:   Initialized the pins and smartled
inputs:  -
outputs: -
notes:   -         
/************************************************************************************/
  // Define I/O and attach ISR
  pinMode(RST_PIN, INPUT_PULLUP); // Reset - Use internal pullup
  pinMode(RGB_R_PIN, OUTPUT);     // Red RGB led
  pinMode(RGB_G_PIN, OUTPUT);     // Green RGB led
  pinMode(RGB_B_PIN, OUTPUT);     // Blue RGB led

  // Init with red led
  smartLedInit();
}

/************************************************************************************/
void smartLedFlash (RGB_COLOR_ENUM color) {
/* 
short:   Flash the led with the given color
inputs:  Color to flash the led 
outputs: -
notes:   Blocking function that takes 150ms time to finish.
/************************************************************************************/
    switch( color ) {
    case RED:
      digitalWrite(RGB_R_PIN, ON);
      delay(50);
      digitalWrite(RGB_R_PIN, OFF);
      break;
    case GREEN:
      digitalWrite(RGB_G_PIN, ON);
      delay(50);
      digitalWrite(RGB_G_PIN, OFF);
      break;
    case BLUE:
      digitalWrite(RGB_B_PIN, ON);
      delay(50);
      digitalWrite(RGB_B_PIN, OFF);
      break;
    default:
      break;
  }
}

/************************************************************************************/
void create_unique_mqtt_topic_string (char *topic_string) {
/* 
short:   Construct unique mqtt_signature    
inputs:  topic_string that is filled with the unique mqtt topic string      
outputs: -
notes:   -
/************************************************************************************/
#if defined(ESP8266)
  char tmp[30];
  strcpy(topic_string,"DIY-SMARTMETER-V2-");
  sprintf(tmp,"-%06X",ESP.getChipId());
  strcat(topic_string,tmp);
  sprintf(tmp,"-%06X",ESP.getFlashChipId()); 
  strcat(topic_string,tmp);

#elif defined(ESP32)
  sprintf(topic_string, "DIY-SMARTMETER-V2-%11llX", ESP.getEfuseMac());
#endif
}

/************************************************************************************/
void create_unigue_mqtt_id (char *signature) {
/* 
short:   Construct unique mqtt_signature    
inputs:  signature that is filled with the unique mqtt id.
outputs: -
notes:   -
/************************************************************************************/
#if defined(ESP8266)
   char tmp[30];
   strcpy(signature,"DIY-SMARTMETER-V2-");
   sprintf(tmp,"-%06X",ESP.getChipId());
   strcat(signature,tmp);
   sprintf(tmp,"-%06X",ESP.getFlashChipId()); 
   strcat(signature,tmp);

#elif defined(ESP32)
  sprintf(signature, "DIY-SMARTMETER-V2-%11llX", ESP.getEfuseMac());
#endif
}

/************************************************************************************/
uint32_t crc32 (const uint8_t *data, size_t length) {
/*
short:   Create a CRC32 fingerprint from data with length length.
inputs:  data that contains the data and length that indicates the data length
outputs: Returns the CRC32
notes:   -
/************************************************************************************/
  uint32_t crc = 0xFFFFFFFF;

  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
      else crc >>= 1;
    }
  }
  return ~crc;
}

/************************************************************************************/
void safeCopy (char *dest, const char *src, size_t maxLen) {
/* 
short:   Safe copy to make sure no buffer overflows can occur.
inputs:  src char array is copied to dest char with the maximum length. 
outputs: -
notes:   -
/************************************************************************************/
  strncpy(dest, src, maxLen - 1);
  dest[maxLen - 1] = '\0';
}

/************************************************************************************/
void parseConfigLine (char *line, APP_CONFIG_STRUCT *cfg) {
/* 
short:   Parse the string line and store the value in the application configuration.
inputs:  line containing the key=value and cfg pointer to the applucation configuration. 
outputs: -
notes:   -
/************************************************************************************/
  char *key = strtok(line, "=");
  char *value = strtok(NULL, "\n");

  if (!key || !value) return;

  if      (strcmp(key, "MQTT_USERNAME") == 0) safeCopy(cfg->mqtt_username, value, MQTT_USERNAME_LENGTH);
  else if (strcmp(key, "MQTT_PASSWORD") == 0) safeCopy(cfg->mqtt_password, value, MQTT_PASSWORD_LENGTH);
  else if (strcmp(key, "MQTT_ID") == 0)       safeCopy(cfg->mqtt_id, value, MQTT_ID_TOKEN_LENGTH);
  else if (strcmp(key, "MQTT_TOPIC") == 0)    safeCopy(cfg->mqtt_topic, value, MQTT_TOPIC_STRING_LENGTH);
  else if (strcmp(key, "MQTT_HOST") == 0)     safeCopy(cfg->mqtt_remote_host, value, MQTT_REMOTE_HOST_LENGTH);
  else if (strcmp(key, "MQTT_PORT") == 0)     safeCopy(cfg->mqtt_remote_port, value, MQTT_REMOTE_PORT_LENGTH);
  else if (strcmp(key, "P1_BAUDRATE") == 0)   safeCopy(cfg->p1_baudrate, value, P1_BAUDRATE_LENGTH);
  else if (strcmp(key, "MQTT_ANONIMIZE_P1") == 0) safeCopy(cfg->mqtt_anonimize_p1, value, MQTT_ANONIMIZE_P1_LENGTH);
  else if (strcmp(key, "TCP_ANONIMIZE_P1") == 0)  safeCopy(cfg->tcp_anonimize_p1, value, TCP_ANONIMIZE_P1_LENGTH);
}

/************************************************************************************/
void defaultAppConfig (APP_CONFIG_STRUCT *cfg) {
/* 
short:   Create default config and fills the cfg struct.
inputs:  cfg that points to the config to be filled
outputs: -
notes:   -
/************************************************************************************/
  safeCopy(cfg->mqtt_username, DEFAULT_MQTT_USERNAME, MQTT_USERNAME_LENGTH);
  safeCopy(cfg->mqtt_password, DEFAULT_MQTT_PASSWORD, MQTT_PASSWORD_LENGTH);
  safeCopy(cfg->mqtt_topic, DEFAULT_MQTT_TOPIC, MQTT_TOPIC_STRING_LENGTH);
  safeCopy(cfg->mqtt_remote_host, DEFAULT_MQTT_REMOTE_HOST, MQTT_REMOTE_HOST_LENGTH);
  safeCopy(cfg->mqtt_remote_port, DEFAULT_MQTT_REMOTE_PORT, MQTT_REMOTE_PORT_LENGTH);
  safeCopy(cfg->p1_baudrate, DEFAULT_P1_BAUDRATE, P1_BAUDRATE_LENGTH);
  safeCopy(cfg->mqtt_anonimize_p1, DEFAULT_MQTT_ANONIMIZE_P1, MQTT_ANONIMIZE_P1_LENGTH);
  safeCopy(cfg->tcp_anonimize_p1, DEFAULT_TCP_ANONIMIZE_P1, TCP_ANONIMIZE_P1_LENGTH);

  create_unigue_mqtt_id(cfg->mqtt_id);
}

/************************************************************************************/
bool writeAppConfig (APP_CONFIG_STRUCT *cfg) {
/* 
short:   Write config to the device
inputs:  app_config that is used to write to the device
outputs: Returns true when successfull, otherwise false.
notes:   -
/************************************************************************************/
  deleteAppConfig();

  File file = LittleFS.open("/config.txt", "w");
  if (!file) return false;

  char buffer[512];
  buffer[0] = '\0';

  snprintf(buffer + strlen(buffer), sizeof(buffer),
    "MQTT_USERNAME=%s\n"
    "MQTT_PASSWORD=%s\n"
    "MQTT_ID=%s\n"
    "MQTT_TOPIC=%s\n"
    "MQTT_HOST=%s\n"
    "MQTT_PORT=%s\n"
    "P1_BAUDRATE=%s\n"
    "MQTT_ANONIMIZE_P1=%s\n"
    "TCP_ANONIMIZE_P1=%s\n",
    cfg->mqtt_username,
    cfg->mqtt_password,
    cfg->mqtt_id,
    cfg->mqtt_topic,
    cfg->mqtt_remote_host,
    cfg->mqtt_remote_port,
    cfg->p1_baudrate,
    cfg->mqtt_anonimize_p1,
    cfg->tcp_anonimize_p1
  );

  uint32_t crc = crc32((uint8_t*)buffer, strlen(buffer));

  file.print(buffer);
  file.printf("CRC32=%08lX\n", crc);

  file.close();
  return true;
}

/************************************************************************************/
bool readAppConfig (APP_CONFIG_STRUCT *cfg) {
/* 
short:   Read the configuration stored on the device.
inputs:  app_config that is filled with the configuration stored on the device 
outputs: Returns true when successfull, otherwise false.
notes:   -
/************************************************************************************/

  if (!LittleFS.begin()) return false;
  if (!LittleFS.exists("/config.txt")) return false;

  File file = LittleFS.open("/config.txt", "r");
  if (!file) return false;

  char content[512];
  char crcLine[32];
  content[0] = '\0';

  while (file.available()) {
    char line[160];
    int len = file.readBytesUntil('\n', line, sizeof(line) - 1);
    line[len] = '\0';

    if (strncmp(line, "CRC32=", 6) == 0) {
      strcpy(crcLine, line + 6);
    } else {
      strcat(content, line);
      strcat(content, "\n");
      parseConfigLine(line, cfg);
    }
  }

  file.close();

  uint32_t storedCRC = strtoul(crcLine, NULL, 16);
  uint32_t calcCRC   = crc32((uint8_t*)content, strlen(content));

  if (storedCRC != calcCRC) {
    DEBUG_PRINTF("CRC mismatch – config corrupt!");
    return false;
  }

  DEBUG_PRINTF("Config CRC OK");
  return true;
}

/************************************************************************************/
bool deleteAppConfig () {
/* 
short:   Erase config to FFS
inputs:  -
outputs: Returns true when successfull, otherwise false.
notes:   -
/************************************************************************************/
  if (!LittleFS.begin()) return false;
  if (LittleFS.exists("/config.txt")) {
    return LittleFS.remove("/config.txt");
  }
  return false;
}

/************************************************************************************/
/************************************************************************************/
/* SETUP                                                                            */
/************************************************************************************/
/************************************************************************************/

/************************************************************************************/
void setup () { 
/* 
short:   Arduino setup to initialize the hardware and software components.
inputs:  -
outputs: -
notes:   -     
/************************************************************************************/
  hardwareSetup();  
  
  // First initialize the serial
  #if defined(ESP8266)
    Serial.begin(115200, SERIAL_8N1);
  #elif defined(ESP32)
    Serial.begin(115200);
  #endif

  // Say Hello to user by flashing the led blue
  for(uint8_t idx = 0; idx < 2; idx++ ) {
    smartLedFlash(BLUE);
    delay(150);
  }

  // NOTE: This has to do with configuration, when the configuration is stored, why create everytime a new ID?
  //       First time create the ID and otherwise when the default values for the configuration is required.
  //       So at this point I would read the configuration file and if this could not be done, load the
  //       default application configuration.
  // Setup unique mqtt id and mqtt topic string
  create_unique_mqtt_topic_string(app_config.mqtt_topic);
  create_unigue_mqtt_id(app_config.mqtt_id);
  sprintf(mqtt_topic, MQTT_TOPIC);

  // Perform factory reset switches
  // is pressed during powerup
  if( 0 == digitalRead(RST_PIN) ) {
    wifiManager.resetSettings();
    deleteAppConfig();
    while(0 == digitalRead(RST_PIN)) {
       smartLedFlash(BLUE);
       delay(250);
    }
    resetHardware();
  }

  // Read config file or generate default
  if( !readAppConfig(&app_config) ) {
    defaultAppConfig(&appConfig);
    writeAppConfig(&app_config);
  }

  // Wi-Fi Manager
  wifiManager.setMinimumSignalQuality(20);
  wifiManager.setTimeout(300);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  shouldSaveConfig = false;

  // Adds some parameters to the default webpage
  WiFiManagerParameter wmp_mqtt_text("<b>MQTT settings:</b><br/><br/>");
  wifiManager.addParameter(&wmp_mqtt_text);
  WiFiManagerParameter custom_mqtt_username("mqtt_username", "Username", app_config.mqtt_username, MQTT_USERNAME_LENGTH);
  WiFiManagerParameter custom_mqtt_password("mqtt_password", "Password", app_config.mqtt_password, MQTT_PASSWORD_LENGTH);
  WiFiManagerParameter custom_mqtt_remote_host("mqtt_remote_host", "Host", app_config.mqtt_remote_host, MQTT_REMOTE_HOST_LENGTH);
  WiFiManagerParameter custom_mqtt_remote_port("mqtt_remote_port", "Port", app_config.mqtt_remote_port, MQTT_REMOTE_PORT_LENGTH);
  WiFiManagerParameter custom_p1_baudrate("p1_baudrate", "Baudrate", app_config.p1_baudrate, P1_BAUDRATE_LENGTH);

  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_remote_host);
  wifiManager.addParameter(&custom_mqtt_remote_port);
  wifiManager.addParameter(&custom_p1_baudrate);

  // Adds security parameters to the default webpage
  WiFiManagerParameter wmp_sec_text("<br/><br/><b>Security settings:</b><br/>");
  wifiManager.addParameter(&wmp_sec_text);
  WiFiManagerParameter custom_mqtt_anonymize_p1("mqtt_anonymize_p1", "Anonymize your MQTT P1 data", "YES", MQTT_ANONIMIZE_P1_LENGTH, "checked type=\"checkbox\"", WFM_LABEL_AFTER);
  wifiManager.addParameter(&custom_mqtt_anonymize_p1);
  WiFiManagerParameter custom_tcp_anonymize_p1("tcp_anonymize_p1", "Anonymize your local TCP P1 data", "YES", TCP_ANONIMIZE_P1_LENGTH, "type=\"checkbox\"", WFM_LABEL_AFTER);
  wifiManager.addParameter(&custom_tcp_anonymize_p1);
  
  // Add the unit ID to the webpage
  char fd_str[200]="<br/><br/><b>Your DIY SMARTMETER ID:<br/><br/>";
  strcat(fd_str, app_config.mqtt_topic);
  strcat(fd_str, "</b><br/><br/>Make a SCREENSHOT - you will need this info later!<br/>");
  WiFiManagerParameter mqtt_topic_text(fd_str);
  wifiManager.addParameter(&mqtt_topic_text);

  // Blue led on. Will go GREEN if WiFi network is available or 
  // stays BLUE when WiFi credentials are needed.
  smartLedColor(BLUE, ON);
  
  if( !wifiManager.autoConnect("DIY SMARTMETER config")) {
    delay(1000);
    resetHardware();
  }  

  //
  // Update config if needed
  //
  if(shouldSaveConfig) {
    strcpy(app_config.mqtt_username, custom_mqtt_username.getValue());
    strcpy(app_config.mqtt_password, custom_mqtt_password.getValue());
    strcpy(app_config.mqtt_remote_host, custom_mqtt_remote_host.getValue());
    strcpy(app_config.mqtt_remote_port, custom_mqtt_remote_port.getValue());
    strcpy(app_config.p1_baudrate, custom_p1_baudrate.getValue());
    strcpy(app_config.mqtt_anonimize_p1, (strcmp(custom_mqtt_anonymize_p1.getValue(), "YES") == 0 ? "YES" : "NO"));
    app_config.mqtt_anonimize_p1_bool = (strcmp(app_config.mqtt_anonimize_p1, "YES") == 0 ? true : false);
    strcpy(app_config.tcp_anonimize_p1, (strcmp(custom_tcp_anonymize_p1.getValue(), "YES") == 0 ? "YES" : "NO"));
    app_config.tcp_anonimize_p1_bool = (strcmp(app_config.tcp_anonimize_p1, "YES") == 0 ? true : false);
    writeAppConfig(&app_config);
  }

  dashboard.begin();
  p1DataServer.begin();

  // Always print config to terminal before swapping serial port
#if defined(ESP8266)
  Serial.printf("\n");
  Serial.printf("************ DIY Smartmeter ********************\n");
  Serial.printf("ESP8266 info\n");
  Serial.printf("\tSDK Version       : %s\n", ESP.getSdkVersion() );
  Serial.printf("\tCore Version      : %s\n", ESP.getCoreVersion().c_str() );
  Serial.printf("\tCore Frequency    : %d Mhz\n", ESP.getCpuFreqMHz());
  Serial.printf("\tLast reset        : %s\n", ESP.getResetReason().c_str() );

#elif defined(ESP32)
  char resetReason[20];
  getResetReason(resetReason);
  
  Serial.printf("\n");
  Serial.printf("************ DIY Smartmeter ********************\n");
  Serial.printf("ESP32S2 info\n");
  Serial.printf("\tSDK Version        : %s\n", ESP.getSdkVersion() );
  Serial.printf("\tCore Version       : %s\n", ESP.getCoreVersion() );
  Serial.printf("\tCore Frequency     : %ld Mhz\n", ESP.getCpuFreqMHz());
  Serial.printf("\tLast reset         : %s\n", resetReason );
#endif

  Serial.printf("MQTT settings\n");
  Serial.printf("\tmqtt_username     : %s\n", app_config.mqtt_username);
  Serial.printf("\tmqtt_password     : %s\n", app_config.mqtt_password);
  Serial.printf("\tmqtt_id           : %s\n", app_config.mqtt_id);
  Serial.printf("\tmqtt_topic        : %s\n", mqtt_topic);
  Serial.printf("\tmqtt_remote_host  : %s\n", app_config.mqtt_remote_host);
  Serial.printf("\tmqtt_remote_port  : %s\n", app_config.mqtt_remote_port);
  Serial.printf("\tIP address        : %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("\tmqtt_anonimize_p1 : %s\n", app_config.mqtt_anonimize_p1);

  Serial.printf("DSMR settings\n");
  Serial.printf("\tP1 Baudrate       : %s baud\n", app_config.p1_baudrate);

  // Setup mDNS Service
  if ( MDNS.begin("diy_smartmeter") ) { 
    MDNS.addService("http", "tcp", WEB_SERVER_PORT);     // Webserver
    MDNS.addService("p1data", "tcp", P1_DATA_SERVER_PORT); // TCP/IP P1 data provider server
    Serial.printf("mDNS\n");
    Serial.printf("\tmDNS URL          : %s\n", "diy_smartmeter.local");
    Serial.printf("\tWeb server        : %s (anonimize P1 data: %s)\n", "diy_smartmeter.local:80", app_config.tcp_anonimize_p1);
    Serial.printf("\tData server       : %s\n", "diy_smartmeter.local:3141");
  } else {
    Serial.printf("mDNS:   Could not start the mDNS service!\n");
  }

  Serial.printf("***************************************************\n\n");
  Serial.flush();

  long baudrate = atol(app_config.p1_baudrate);
#if defined(ESP8266)
  // Set P1 port baudrate. DSMR V2 uses 9600 baud. Otherwise 115200 baud
  switch(baudrate){
    case 9600:
      Serial.begin(9600, SERIAL_7E1);
      break;

    default:
      Serial.begin(115200, SERIAL_8N1);
      break;
  }

  #ifdef DEBUG
    Serial1.begin(115200, SERIAL_8N1);
  #endif
 
  // Allow bootloader to connect: do not remove!
  delay(2000);
  
  // Relocate Serial Port
  Serial.swap();

#elif defined(ESP32)
  switch(baudrate){
    case 9600:
      Serial1.begin(9600, SERIAL_7E1, SM_RXD, 10); // GPIO10 is not used in this setup
      break;

    default:
      Serial1.begin(115200, SERIAL_8N1, SM_RXD, 10); // GPIO10 is not used in this setup
      break;
  }
#endif
  
#ifdef DEBUG
  DEBUG_PRINTF("\n\r%s\n\r", "Debug mode ON ..." );
#endif

  // Initialise FSM
  initFSM(STATE_START, EV_IDLE);
}

/************************************************************************************/
/************************************************************************************/
/* LOOP                                                                             */
/************************************************************************************/
/************************************************************************************/

/************************************************************************************/
void loop () {
/* 
short:   loop(), runs forever executing FSM
inputs:  -
outputs: -
notes:   -
/************************************************************************************/
  // Check for IP connection 
  if( WiFi.status() == WL_CONNECTED) {
    // Handle mqtt, if not MQTT server is available it uses a timer to reconnect every MQTT_RETRY_TIMEOUT ms. 
    // Otherwise, it freezes all other services that are running on the CPU. (#26)
    if( !mqttClient.connected() && ( mqttTimer == 0 || millis() > mqttTimer + MQTT_RETRY_TIMEOUT ) ) {
      smartLedFlash(RED); // Added to see when MQTT is not connected (#26: causing a delay of 150ms)
      mqtt_connect();
      //delay(250); #26: removed, while it causes problems for the MDNS, HTTP and TCP server updates
      mqttTimer = millis(); // Set timer to reconnect over MQTT_RETRY_TIMEOUT ms (#26)

    } else {
      // Handle MQTT loop
      mqttClient.loop();
    }

#if defined(ESP8266)
    // Handle mDNS service, the ESP32S2 does this automatically.
    MDNS.update();
#endif

    dashboard.loop();
    p1DataServer.loop();
  }

  // Capture P1 messages. If P1 msg is available raise MQTT event
  if( true == capture_p1() ) {
    if ( app_config.tcp_anonimize_p1_bool ) { // If enabled, anonimize the P1 data before sending it over the TCP server
      if ( !P1DataServer::anonymizeP1(p1_buf) ) {
        DEBUG_PRINTF(">%s: Parsing equipment ID error\n\r", __FUNCTION__);
      }
    }

    dashboard.processP1(p1_buf);
    p1DataServer.sendP1(p1_buf);

    raiseEvent(EV_P1_AVAILABLE);
  }

  // 
  // Handle heartbeat (Ticker.h causes crashes)
  //
  uint32_t heartbeat_cur = millis();
  uint32_t heartbeat_elapsed = heartbeat_cur - heartbeat_prev;
  if( heartbeat_elapsed >= HEARTBEAT_UPDATE_INTERVAL_SEC ) {
    
    //
    heartbeat_prev = heartbeat_cur; 
    
    // Call the heartbeat fp
    if( fsm[state][event].heartbeat != NULL) {
      fsm[state][event].heartbeat();
    } 
  }
}


/******************************************************************
*
* MQTT section
*
******************************************************************/

/******************************************************************/
void mqtt_callback(char* topic, byte* payload, unsigned int length)
/* 
short   : mqtt callback                  
inputs  :        
outputs : 
notes   :         
Version : DMK, Initial code
*******************************************************************/
{
}

/******************************************************************/
void mqtt_connect() 
/* 
short:      Connect to MQTT server UNSECURE
inputs:        
outputs: 
notes:         
Version :   DMK, Initial code
*******************************************************************/
{
  char *host = app_config.mqtt_remote_host;
  int port = atoi(app_config.mqtt_remote_port);
  
  mqttClient.setClient(mqttWifiClient);
  mqttClient.setServer(host, port );
  mqttClient.setBufferSize(MQTT_MSGBUF_SIZE);
  if(mqttClient.connect(app_config.mqtt_id, app_config.mqtt_username, app_config.mqtt_password)){

    // Subscribe to mqtt topic
    mqttClient.subscribe(mqtt_topic);

    // Set callback
    mqttClient.setCallback(mqtt_callback);

    // Set timer to zero (#26)
    mqttTimer = 0;

    DEBUG_PRINTF("%s: MQTT connected to %s:%d\n", __FUNCTION__, host, port);
  } else {
    DEBUG_PRINTF("%s: MQTT connection ERROR (%s:%d)\n", __FUNCTION__, host, port);
  }
}




/******************************************************************/
/*
 * Application signature and config
 */
/******************************************************************/



/******************************************************************/
/*
 * P1 (Smart meter) section
 */
/******************************************************************/

/*******************************************************************/
void p1_store(char ch)
/* 
short:         
inputs:        
outputs:       
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
   if( (p1 - p1_buf) < P1_MAX_DATAGRAM_SIZE ) {
      *p1 = ch;
      p1++; 
   } else {
      DEBUG_PRINTF("%s:P1 buffer overflow\n\r", __FUNCTION__); 
   }
}

/*******************************************************************/
void p1_reset()
/* 
short:   
inputs:        
outputs:       
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
   p1 = p1_buf;
   *p1='\0';
}

/*******************************************************************/
bool capture_p1() 
/* 
short:§        Try to capture single P1 telegram         
inputs:        
outputs:       
notes:         blocking on captureLine(..) function
Version :      DMK, Initial code
*******************************************************************/
{
   bool retval = false;

#if defined(ESP8266)
   if( Serial.available() ) { 
      while( Serial.available() ) {
         char ch = Serial.read();

#elif defined(ESP32)
    if( Serial1.available() ) {
      while( Serial1.available() ) {
         char ch = Serial1.read();
#endif
         switch(p1_msg_state) {
            //
            case P1_MSG_S0:
               if( ch == '/' ) {
                  p1_msg_state = P1_MSG_S1;
                  p1_reset();
                  p1_store(ch);
               }
            break;             

            //
            case P1_MSG_S1:
               p1_store(ch);
               if( ch == '!' ) {
                  p1_msg_state = P1_MSG_S2;
               }
            break;

            //
            case P1_MSG_S2:
               p1_store(ch);
               if( ch == '\n' ) {
                  p1_store('\0');  // Add 0 terminator
                  p1_msg_state = P1_MSG_S0;
                  retval = true;
               }              
            break;
            
            //
            default:
               DEBUG_PRINTF("%s:Oeps, something bad happend\n\r", __FUNCTION__); 
               retval = false;
            break; 
         }
      }
   }
   return retval;
}
