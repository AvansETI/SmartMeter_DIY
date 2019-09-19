/*-------------------------------------------------------------------------
  Arduino sketch to mqtt Dutch Smart Meter P1 datagrams.

  See  for more information.

  Initial: dkroeske(dkroeske@gmail.com), august 2019

  Happy Coding
  
  -------------------------------------------------------------------------
  
  The MIT License (MIT)
  Copyright © 2019 <copyright Diederich Kroeske>
  
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

  -------------------------------------------------------------------------*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <FS.h>

#include <PubSubClient.h>
// MAKE SURE: in PubSubClient.h change MQTT_MAX_PACKET_SIZE to 2048 !! //

//
// situation | Mode
// ----------|-----
// Normal    | GPIO2 is used to control the NeoPixel
// Debug     | GPIO2 is connected to Serial1 to print debug messages
#define DEBUG

#ifdef DEBUG
 #define DEBUG_PRINTF(format, ...) (Serial1.printf(format, __VA_ARGS__))
#else
 #define DEBUG_PRINTF
#endif

// WiFi RESET pin
#define RST_PIN         4

#define DATAGRAM_UPDATE_RATE_MS  30000

// Local variables
uint32_t cur=0, prev=0;
WiFiManager wifiManager;

typedef struct {
  uint8_t red, green, blue;
} RGB_STRUCT;

// Application configs struct. 
bool shouldSaveConfig;

#define MQTT_USERNAME_LENGTH       32
#define MQTT_PASSWORD_LENGTH       32
#define MQTT_ID_TOKEN_LENGTH       64
#define MQTT_TOPIC_STRING_LENGTH   64
#define MQTT_REMOTE_HOST_LENGTH   128
#define MQTT_REMOTE_PORT_LENGTH    10

typedef struct {
   char     mqtt_username[MQTT_USERNAME_LENGTH];
   char     mqtt_password[MQTT_PASSWORD_LENGTH];
   char     mqtt_id[MQTT_ID_TOKEN_LENGTH];
   char     mqtt_topic[MQTT_TOPIC_STRING_LENGTH];
   char     mqtt_remote_host[MQTT_REMOTE_HOST_LENGTH];
   char     mqtt_remote_port[MQTT_REMOTE_HOST_LENGTH];
} APP_CONFIG_STRUCT;

APP_CONFIG_STRUCT app_config;

#ifdef SECURE
WiFiClientSecure wifiClient;
#else
WiFiClient wifiClient;
#endif

// Only with some dummy values seems to work ... instead of mqttClient();
PubSubClient mqttClient("", 0, wifiClient);

#define P1_TELEGRAM_SIZE   1024

// Datagram P1 buffer 
#define P1_MAX_DATAGRAM_SIZE 1024
char p1_buf[P1_MAX_DATAGRAM_SIZE]; // Complete P1 telegram
char *p1;

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

// Prototype state functions. 
void s0(void);
void s1(void);
void s2(void);
void s3(void);
void s4(void);
void err(void);

// Define FSM (states, events)
typedef enum { EV_TRUE = 0, EV_ERR, EV_TELE, EV_ILOG } ENUM_EVENT;
typedef enum { STATE_S0 = 0, STATE_S1, STATE_S2, STATE_S3, STATE_S4, STATE_ERR } ENUM_STATE;

/* Define fsm transition */
typedef struct {
   void (*f)(void);
   ENUM_STATE nextState;
} STATE_TRANSITION_STRUCT;

typedef enum {NO_ERR = 0, ERR_NETWORK, ERR_INVALID_CREDENTIALS } API_ERR_ENUM;
API_ERR_ENUM api_err = NO_ERR; 

// FSM definition (see statemachine diagram)
//
//       | EV_TRUE   EV_ERR   EV_TELE  EV_ILOG   
// -------------------------------------------------------------
// S0    | S1        ERROR    ERROR    ERROR
// S1    | S3        ERROR    ERROR    S2
// S2    | S3        ERROR    ERROR    ERROR
// S3    | ERROR     ERROR    S4       ERROR
// S4    | S3        ERROR    ERROR    ERROR
// ERROR | S0        ERROR    ERROR    ERROR

STATE_TRANSITION_STRUCT fsm[6][4] = {
   { {s1,   STATE_S1},  {err, STATE_ERR}, {err, STATE_ERR}, {err, STATE_ERR} }, // State S0
   { {s3,   STATE_S3},  {err, STATE_ERR}, {err, STATE_ERR}, {s2,   STATE_S2} }, // State S1
   { {s3,   STATE_S3},  {err, STATE_ERR}, {err, STATE_ERR}, {err, STATE_ERR} }, // State S2
   { {s3,   STATE_S3},  {err, STATE_ERR}, {s4,  STATE_S4},  {err, STATE_ERR} }, // State S3
   { {s3,   STATE_S3},  {err, STATE_ERR}, {err, STATE_ERR}, {err, STATE_ERR} }, // State S4
   { {s0,   STATE_S0},  {err, STATE_ERR}, {err, STATE_ERR}, {err,  STATE_ERR} } // State ERROR
};

// State holder
ENUM_STATE state = STATE_ERR;
ENUM_EVENT event = EV_TRUE;

// mqtt topic strings: emon/<uid>/msg
char mqtt_topic_msg[128];


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
   // Define I/O and attach ISR
   pinMode(RST_PIN, INPUT);

   // Init with red led
   smartLedInit();
   for(int idx = 0; idx < 5; idx++ ) {
      smartLedFlash(10);
      delay(100);
   }

  // Setup unique mqtt id and mqtt topic string
  create_unique_mqtt_topic_string(app_config.mqtt_topic);
  create_unigue_mqtt_id(app_config.mqtt_id);
  sprintf(mqtt_topic_msg,"mon/%s/msg",app_config.mqtt_topic);

   // Perform factory reset switches
   // is pressed during powerup
   if( 0 == digitalRead(RST_PIN) ) {
      wifiManager.resetSettings();
      deleteAppConfig();
      while(0 == digitalRead(RST_PIN)) {
         smartLedFlash(10);
         delay(500);
      }
      ESP.reset();
   }

  // Read config file or generate default
  if( !readAppConfig(&app_config) ) {
    strcpy(app_config.mqtt_username, "");
    strcpy(app_config.mqtt_password, "");
    strcpy(app_config.mqtt_remote_host, "test.mosquitto.org");
    strcpy(app_config.mqtt_remote_port, "1883");
    writeAppConfig(&app_config);
  }

   //
   smartLedShowColor({0,255,0});
   wifiManager.setMinimumSignalQuality(20);
   wifiManager.setTimeout(300);
   wifiManager.setSaveConfigCallback(saveConfigCallback);
   shouldSaveConfig = false;

  // Adds some parameters to the default webpage
  WiFiManagerParameter wmp_text("<br/>MQTT setting:</br>");
  wifiManager.addParameter(&wmp_text);
  WiFiManagerParameter custom_mqtt_username("mqtt_username", "Username", app_config.mqtt_username, MQTT_USERNAME_LENGTH);
  WiFiManagerParameter custom_mqtt_password("mqtt_password", "Password", app_config.mqtt_password, MQTT_PASSWORD_LENGTH);
  WiFiManagerParameter custom_mqtt_remote_host("mqtt_remote_host", "Host", app_config.mqtt_remote_host, MQTT_REMOTE_HOST_LENGTH);
  WiFiManagerParameter custom_mqtt_remote_port("mqtt_port", "Port", app_config.mqtt_remote_port, MQTT_REMOTE_PORT_LENGTH);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_remote_host);
  wifiManager.addParameter(&custom_mqtt_remote_port);

  // Add the unit ID to the webpage
  char fd_str[128]="<p>Your EMON ID: <b>";
  strcat(fd_str, app_config.mqtt_topic);
  strcat(fd_str, "</b>Make SCREENSHOT - you will need this info later!</p>");
  WiFiManagerParameter mqqt_topic_text(fd_str);
  wifiManager.addParameter(&mqqt_topic_text);
   
   if( !wifiManager.autoConnect("Emon configuratie")) {
      delay(1000);
      ESP.reset();
   }

  //
  // Update config if needed
  //
  if(shouldSaveConfig) {
    strcpy(app_config.mqtt_username, custom_mqtt_username.getValue());
    strcpy(app_config.mqtt_password, custom_mqtt_password.getValue());
    strcpy(app_config.mqtt_remote_host, custom_mqtt_remote_host.getValue());
    strcpy(app_config.mqtt_remote_port, custom_mqtt_remote_port.getValue());
    writeAppConfig(&app_config);
  }
   
   //
   if( !MDNS.begin("DIY-EMON_V10") ) {
   } else {
      MDNS.addService("diy_emon_v10", "tcp", 10000);
   }

   //
   //Serial.begin(9600, SERIAL_7E1);
   Serial.begin(115200, SERIAL_8N1);

   #ifdef DEBUG
   Serial1.begin(115200, SERIAL_8N1);
   Serial1.printf("\n\r... in debug mode ...\n\r");
   #endif

   delay(1000);

   // Relocate Serial Port
   Serial.swap();

   // Debug
   DEBUG_PRINTF("SDK Version: %s\n\r", ESP.getSdkVersion() );
   DEBUG_PRINTF("CORE Version: %s\n\r", ESP.getCoreVersion().c_str() );
   DEBUG_PRINTF("RESET: %s\n\r", ESP.getResetReason().c_str() );
  Serial.printf("OAT ready\n");
  Serial.printf("mqtt_username: %s\n", app_config.mqtt_username);
  Serial.printf("mqtt_password: %s\n", app_config.mqtt_password);
  Serial.printf("mqtt_id: %s\n", app_config.mqtt_id);
  Serial.printf("mqtt_topic MSG: %s\n", mqtt_topic_msg);
  Serial.printf("mqtt_remote_host: %s\n", app_config.mqtt_remote_host);
  Serial.printf("mqtt_remote_port: %s\n", app_config.mqtt_remote_port);
   smartLedShowColor({0,255,0});

   delay(1000);

   DEBUG_PRINTF("%s:freq: %d Mhz\n\r", __FUNCTION__, ESP.getCpuFreqMHz());
   delay(1000);
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

#ifndef SECURE
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
  
  mqttClient.setClient(wifiClient);
  mqttClient.setServer(host, port);
  if(mqttClient.connect(app_config.mqtt_id)){

    // Subscribe to ../raw and ../msg
    mqttClient.subscribe(mqtt_topic_msg);

    // Set callback
    mqttClient.setCallback(mqtt_callback);
    Serial.printf("%s: MQTT connected to %s:%d\n", __FUNCTION__, host, port);
  } else {
    Serial.printf("%s: MQTT connection ERROR (%s:%d)\n", __FUNCTION__, host, port);
  }
}
#endif

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
   char tmp[30];
   strcpy(topic_string,"BFD01");
   sprintf(tmp,"-%06X",ESP.getChipId());
   strcat(topic_string,tmp);
   sprintf(tmp,"-%06X",ESP.getFlashChipId()); 
   strcat(topic_string,tmp);
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
   char tmp[30];
   strcpy(signature,"2019-ETI-EMON");
   strcat(signature,"-V01");
   sprintf(tmp,"-%06X",ESP.getChipId());
   strcat(signature,tmp);
   sprintf(tmp,"-%06X",ESP.getFlashChipId()); 
   strcat(signature,tmp);
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

   if( SPIFFS.begin() ) {
      if( SPIFFS.exists("/config.json") ) {
         File file = SPIFFS.open("/config.json","r");
         if( file ) {
            size_t size = file.size();
            std::unique_ptr<char[]> buf(new char[size]);

            file.readBytes(buf.get(), size);

            //DynamicJsonBuffer jsonBuffer;
            DynamicJsonDocument json(1024);
            
            //JsonObject& json = jsonBuffer.parseObject(buf.get());
            DeserializationError error = deserializeJson(json, buf.get());
            if( !error ) {
               strcpy(app_config->mqtt_username, json["MQTT_USERNAME"]);
               strcpy(app_config->mqtt_password, json["MQTT_PASSWORD"]);
               strcpy(app_config->mqtt_remote_host, json["MQTT_HOST"]);
               strcpy(app_config->mqtt_remote_port, json["MQTT_PORT"]);
               retval = true;
            }
         }
      }
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

  if( SPIFFS.begin() ) {
      
      // Delete config if exists
      if( SPIFFS.exists("/config.json") ) {
         SPIFFS.remove("/config.json");
      }

      DynamicJsonDocument doc(1024);
      doc["MQTT_USERNAME"] = app_config->mqtt_username;
      doc["MQTT_PASSWORD"] = app_config->mqtt_password;
      doc["MQTT_HOST"] = app_config->mqtt_remote_host;
      doc["MQTT_PORT"] = app_config->mqtt_remote_port;

      File file = SPIFFS.open("/config.json","w");
      if( file ) {
         //serializeJson(doc, file);
         serializeJson(doc, Serial);

         file.close();
         retval = true;
      }
   } 
   return retval;
}

/******************************************************************/
void deleteAppConfig() 
/* 
short:         Erase config to FFS
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
   if( SPIFFS.begin() ) {
      
      // Delete config if exists
      if( SPIFFS.exists("/config.json") ) {
         SPIFFS.remove("/config.json");
      }
   } 
}



/******************************************************************/
void loop()
/* 
short:         loop(), runs forever executing FSM
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  handleEvent();
}




/*******************************************************************/
void jsonifyPayload(MEASUREMENT_STRUCT *payload, char *body, int lenght )
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{   
//   StaticJsonBuffer<1024> jsonBuffer;
//
//   JsonObject& root = jsonBuffer.createObject();
//   JsonObject& datagram = root.createNestedObject("datagram");
//   datagram["p1"] = String(payload->p1_telegram);
//
//   root.printTo(body, 1024);
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

   if( Serial.available() ) { 
      while( Serial.available() ) { 
         char ch = Serial.read();
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


/******************************************************************/
/*
 * RGB LED section
 */
/******************************************************************/
 
/******************************************************************/
void smartLedShowColor(RGB_STRUCT color)
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
}

/******************************************************************/
void smartLedFlash(uint8_t level)
/* 
short:      Flash current color         
inputs:        
outputs: 
notes:         
Version :   DMK, Initial code
*******************************************************************/
{
}

/******************************************************************/
void smartLedInit()
/* 
short:      Init         
inputs:        
outputs: 
notes:         
Version :   DMK, Initial code
*******************************************************************/
{
}


/******************************************************************/
/*
 * FSM section
 */
/******************************************************************/
 
/******************************************************************/
void raiseEvent(ENUM_EVENT newEvent)
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
   event = newEvent;
}


/******************************************************************/
void handleEvent()
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
   ENUM_EVENT prev = event;
      
   // Call State function
   if( fsm[state][event].f != NULL) {
      fsm[state][event].f() ;
   } 
   
   // Set new state
   state = fsm[state][prev].nextState;
}

/******************************************************************/
void s0(void)
/* 
short:      Login         
inputs:        
outputs:    api_session_token and api_client_id
notes:         
Version :   DMK, Initial code
*******************************************************************/
{
  DEBUG_PRINTF(">%s:\n\r", __FUNCTION__);
  DEBUG_PRINTF("<%s\n\r", __FUNCTION__);
}

/******************************************************************/
void s1(void)
/* 
short:      Request api_logger_id         
inputs:        
outputs:    api_logger_id.
notes:         
Version :   DMK, Initial code
*******************************************************************/
{
  DEBUG_PRINTF(">%s:\n\r", __FUNCTION__);
  DEBUG_PRINTF("<%s\n\r", __FUNCTION__);
}

/******************************************************************/
void s2(void)
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{  
  DEBUG_PRINTF(">%s:\n\r", __FUNCTION__);
  DEBUG_PRINTF("<%s\n\r", __FUNCTION__);
}

/******************************************************************/
void s3(void)
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  DEBUG_PRINTF(">%s:\n\r", __FUNCTION__);
  DEBUG_PRINTF("<%s\n\r", __FUNCTION__);
}

/******************************************************************/
void s4(void)
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  DEBUG_PRINTF(">%s:\n\r", __FUNCTION__);
  DEBUG_PRINTF("<%s\n\r", __FUNCTION__);
}

/******************************************************************/
void err(void)
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  DEBUG_PRINTF(">%s:\n\r", __FUNCTION__);
  DEBUG_PRINTF("<%s\n\r", __FUNCTION__);
}
