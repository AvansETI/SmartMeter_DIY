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

#define DEBUG

#ifdef DEBUG
 #ifdef ESP8266
  #define DEBUG_PRINTF(format, ...) (Serial1.printf(format, __VA_ARGS__))
 #else // ESP32
  #define DEBUG_PRINTF(format, ...) (Serial.printf(format, __VA_ARGS__))
 #endif
#else
 #define DEBUG_PRINTF
#endif

// Local variables
uint32_t cur=0, prev=0;
WiFiManager wifiManager;

// Application configs struct. 
bool shouldSaveConfig;

typedef struct {
  char     mqtt_username[MQTT_USERNAME_LENGTH];
  char     mqtt_password[MQTT_PASSWORD_LENGTH];
  char     mqtt_id[MQTT_ID_TOKEN_LENGTH];
  char     mqtt_topic[MQTT_TOPIC_STRING_LENGTH];
  char     mqtt_remote_host[MQTT_REMOTE_HOST_LENGTH];
  char     mqtt_remote_port[MQTT_REMOTE_PORT_LENGTH];
  char     p1_baudrate[P1_BAUDRATE_LENGTH];
  char     mqtt_anonimize_p1[MQTT_ANONIMIZE_P1_LENGTH];
  bool     mqtt_anonimize_p1_bool;
  char     tcp_anonimize_p1[TCP_ANONIMIZE_P1_LENGTH];
  bool     tcp_anonimize_p1_bool;
} APP_CONFIG_STRUCT;

APP_CONFIG_STRUCT app_config;

// Wifi client used for the MQTT library
WiFiClient mqttWifiClient;

// Only with some dummy values seems to work ... instead of mqttClient();
PubSubClient mqttClient("", 0, mqttWifiClient);
uint32_t mqttTimer = 0; // Time used to reconnect to the mqtt server, when disconnected (#26)

// Datagram P1 buffer 
char p1_buf[P1_MAX_DATAGRAM_SIZE]; // Complete P1 telegram
char *p1;

// Dashboard
Dashboard dashboard;

// P1 Data Server
P1DataServer p1DataServer;

/* Prototype FSM functions. */
void start_pre(void);
void start_heartbeat(void);
void start_post(void);

void idle_pre(void);
void idle_heartbeat(void);
void idle_post(void);

void mqtt_pre(void);
void mqtt_heartbeat(void);
void mqtt_post(void);

/* Define FSM (states, events) */
typedef enum { EV_P1_AVAILABLE, EV_IDLE } ENUM_EVENT;
typedef enum { STATE_START, STATE_IDLE, STATE_MQTT } ENUM_STATE;

/* Define FSM transition */
typedef struct {
   void (*pre)(void);
   void (*heartbeat)(void);
   void (*post)(void);
   ENUM_STATE nextState;
} STATE_TRANSITION_STRUCT;

// SmartMeter reader FSM definition (see statemachine diagram)
//
//        | EV_P1_AVAILABLE  EV_IDLE
// -----------------------------------------------------------------
// START  | -                ILDE   Handle STARTUP      
// IDLE   | MQTT             -      Handle IDLE loop
// MQTT   | -                IDLE   Handle Sending P1 message to broker 
STATE_TRANSITION_STRUCT fsm[3][2] = {
  { 
    {start_pre, start_heartbeat, start_post, STATE_START},
    {start_pre, start_heartbeat, start_post, STATE_IDLE}
  },  // State START
  { 
    {idle_pre, idle_heartbeat, idle_post, STATE_MQTT},
    {idle_pre, idle_heartbeat, idle_post, STATE_IDLE}
  },  // State IDLE
  { 
    {mqtt_pre, mqtt_heartbeat, mqtt_post, STATE_MQTT},
    {mqtt_pre, mqtt_heartbeat, mqtt_post, STATE_IDLE}
  },  // State MQTT
};

// State holder
ENUM_STATE state;
ENUM_EVENT event;

// Heartbeat (polling)
#define HEARTBEAT_UPDATE_INTERVAL_SEC 1000 * 1
uint32_t heartbeat_prev=0, mqtt_throttle_prev = 0;

// P1 statemachine
typedef enum { 
   P1_MSG_S0,
   P1_MSG_S1,
   P1_MSG_S2
} ENUM_P1_MSG_STATE;
ENUM_P1_MSG_STATE p1_msg_state = P1_MSG_S0;

// 
typedef struct {
   char p1_telegram[P1_TELEGRAM_SIZE];
} MEASUREMENT_STRUCT;
MEASUREMENT_STRUCT payload = {""};

// mqtt topic strings: eti-sm
char mqtt_topic[128];

/******************************************************************/
void saveConfigCallback () 
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
   shouldSaveConfig = true;
}

/******************************************************************/
void setup() 
/* 
short:         initial setup(), runes only one time
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{  
  hardwareSetup();  
  
  // Already initialize the serial, so debugging is possible from this step already
  #if defined(ESP8266)
    Serial.begin(115200, SERIAL_8N1);
  #elif defined(ESP32)
    Serial.begin(115200);
  #endif

  // Say Hello to user
  for(uint8_t idx = 0; idx < 2; idx++ ) {
    smartLedFlash(BLUE);
    delay(150);
  }

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
    strcpy(app_config.mqtt_username, MQTT_USERNAME);
    strcpy(app_config.mqtt_password, MQTT_PASSWORD);
    strcpy(app_config.mqtt_remote_host, MQTT_REMOTE_HOST);
    strcpy(app_config.mqtt_remote_port, MQTT_REMOTE_PORT);
    strcpy(app_config.p1_baudrate, "115200");
    writeAppConfig(&app_config);
  }

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

/******************************************************************/
void loop()
/* 
short:         loop(), runs forever executing FSM
inputs:        
outputs: 
notes:         MS, Not full implementation of FSM; a lot of logic still in loop()
Version :      DMK, Initial code
*******************************************************************/
{
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
void create_unique_mqtt_topic_string(char *topic_string)
/* 
short:      Construct unique mqtt_signature    
inputs:        
outputs: 
notes:         
Version :   DMK, Initial code
*******************************************************************/
{
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

/******************************************************************/
void create_unigue_mqtt_id(char *signature)
/* 
short:      Construct unique mqtt_signature    
inputs:        
outputs: 
notes:         
Version :   DMK, Initial code
*******************************************************************/
{
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


/******************************************************************/
/*
 * Application signature and config
 */
/******************************************************************/

/******************************************************************/
bool readAppConfig(APP_CONFIG_STRUCT *app_config) 
/* 
short:         loop(), runs forever executing FSM
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  bool retval = false;

#if defined(ESP8266)
  if( LittleFS.begin() ) {
#elif defined(ESP32)
  if( LittleFS.begin(true) ) { // ESP32 has a different LittleFS implementation and requires true option, so it formats the FS when it fails
#endif
    if( LittleFS.exists("/config.json") ) {
       File configFile = LittleFS.open("/config.json","r");
       if( configFile ) {

          size_t size = configFile.size();
          if (size > 1024) {
            Serial.println("Config file size is too large");
          }

          std::unique_ptr<char[]> buf(new char[size]);
          configFile.readBytes(buf.get(), size);
        
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, buf.get());
          
          if( error == DeserializationError::Ok ) {
             strcpy(app_config->mqtt_username, doc["MQTT_USERNAME"]);
             strcpy(app_config->mqtt_password, doc["MQTT_PASSWORD"]);
             strcpy(app_config->mqtt_remote_host, doc["MQTT_HOST"]);
             strcpy(app_config->mqtt_remote_port, doc["MQTT_PORT"]);
             strcpy(app_config->p1_baudrate, doc["P1_BAUDRATE"]);
             strcpy(app_config->mqtt_anonimize_p1, doc["mqtt_anonimize_p1"]);
             app_config->mqtt_anonimize_p1_bool = (strcmp(app_config->mqtt_anonimize_p1, "YES") == 0 ? true : false);
             strcpy(app_config->tcp_anonimize_p1, doc["tcp_anonimize_p1"]);
             app_config->tcp_anonimize_p1_bool = (strcmp(app_config->tcp_anonimize_p1, "YES") == 0 ? true : false);
             retval = true;
          }
       }
    } else {
      DEBUG_PRINTF(">%s: config.json does not exists\n", __FUNCTION__);
    }
  } else {
    DEBUG_PRINTF(">%s: ERROR: could not initialize LittleFS\n", __FUNCTION__);
  }
  return retval;
}

/******************************************************************/
bool writeAppConfig(APP_CONFIG_STRUCT *app_config) 
/* 
short:         Write config to FFS
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  bool retval = false;

  deleteAppConfig(); // Delete config file if exists

  //StaticJsonDocument<512> doc; // migration
  JsonDocument doc;
  doc["MQTT_USERNAME"] = app_config->mqtt_username;
  doc["MQTT_PASSWORD"] = app_config->mqtt_password;
  doc["MQTT_HOST"] = app_config->mqtt_remote_host;
  doc["MQTT_PORT"] = app_config->mqtt_remote_port;
  doc["P1_BAUDRATE"]= app_config->p1_baudrate;
  doc["mqtt_anonimize_p1"] = app_config->mqtt_anonimize_p1;
  doc["tcp_anonimize_p1"] = app_config->tcp_anonimize_p1;
  
  File configFile = LittleFS.open("/config.json","w+");
  if( configFile ) {
     serializeJson(doc, configFile);
     configFile.close();
     retval = true;
  }    
  return retval;
}

/******************************************************************/
boolean deleteAppConfig() 
/* 
short:         Erase config to FFS
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  boolean retval = false;
  if( LittleFS.begin() ) {
    if( LittleFS.exists("/config.json") ) {
      if( LittleFS.remove("/config.json") ) {
        retval = true;
      }
    }
  } 
  return retval;
}

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

/******************************************************************
*
* FSM section
*
******************************************************************/

/******************************************************************/
void initFSM(ENUM_STATE new_state, ENUM_EVENT new_event)
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  // Set start state
  state = new_state;
  event = new_event;

  // and call event.pre
  if( fsm[state][event].pre != NULL) {
    fsm[state][event].pre() ;
  } 
}
 
/******************************************************************/
void raiseEvent(ENUM_EVENT new_event)
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  // call event.post
  if( fsm[state][event].post != NULL) {
    fsm[state][event].post() ;
  } 
  
  // Set new state
  ENUM_STATE new_state = fsm[state][new_event].nextState;
  
  // call newstate ev.pre
  if( fsm[new_state][new_event].pre != NULL) {
    fsm[new_state][new_event].pre() ;
  } 
  
  // Set new state
  state = new_state;
  
  // Store new event
  event = new_event;
}

/******************************************************************
*
* FSM callbacks section
*
******************************************************************/

/******************************************************************/
void start_pre(void){
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);
  
  // Enter idle mode. DIsplay GREEN for 2 seconds and go Idle
  smartLedInit();
  smartLedColor(GREEN, ON);
  delay(3000);
  
  raiseEvent(EV_IDLE);
}

/******************************************************************/
void start_heartbeat(void){
//  DEBUG_PRINTF(">%s:\n\r", __FUNCTION__);
}

/******************************************************************/
void start_post(void){
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);

  // Turn GREEN LED off
  smartLedColor(GREEN, OFF);
}

/******************************************************************/
void idle_pre(void){
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);
}

/******************************************************************/
void idle_heartbeat(void){
//  DEBUG_PRINTF(">%s:\n\r", __FUNCTION__);
}

/******************************************************************/
void idle_post(void){
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);
}


/******************************************************************/
void mqtt_pre(void){
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);
}

/******************************************************************/
void mqtt_heartbeat(void) {
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);

  // Throttle mqtt topic speed: check if previous send MQTT
  // is at least MQTT_TOPIC_UPDATE_RATE_MS seconds ago
  //
  uint32_t mqtt_throttle_cur = millis();
  uint32_t mqtt_throttle_elapsed = mqtt_throttle_cur - mqtt_throttle_prev;
  if( mqtt_throttle_elapsed >= MQTT_TOPIC_UPDATE_RATE_MS ) {

    if ( app_config.mqtt_anonimize_p1_bool ) { // If enabled, anonimize the P1 data before sending it to the MQTT server
      if ( !P1DataServer::anonymizeP1(p1_buf) ) {
        DEBUG_PRINTF(">%s: Parsing equipment ID error\n\r", __FUNCTION__);
      }
    }

    //
    mqtt_throttle_prev = mqtt_throttle_cur; 
  
    // Construct json object and publish
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    
    JsonObject datagram = root["datagram"].to<JsonObject>();
    datagram["p1"]        = p1_buf;
    datagram["signature"] = app_config.mqtt_id;
    datagram["version"]   = VERSION;

    // Currently not used, so delete the objects
    //JsonObject s0 = datagram["s0"].to<JsonObject>();
    //s0["unit"] = "W";
    //s0["label"] = "e-car charger";
    //s0["value"] = 0;
    
    // Currently not used, so delete the objects
    //JsonObject s1 = datagram["s1"].to<JsonObject>();
    //s1["unit"] = "W";
    //s1["label"] = "solar panels";
    //s1["value"] = 0;
    
    String payload = "";
    serializeJson(doc, payload);
    mqttClient.publish(mqtt_topic, payload.c_str());

    // Flash LED
    smartLedFlash(GREEN);
  }

  // Always back to idle
  raiseEvent(EV_IDLE);
}

/******************************************************************/
void mqtt_post(void){
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);
}
