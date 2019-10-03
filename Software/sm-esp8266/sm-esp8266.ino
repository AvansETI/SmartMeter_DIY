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
#include <Ticker.h>

Ticker sim;

#include <PubSubClient.h>
// MAKE SURE: in PubSubClient.h change MQTT_MAX_PACKET_SIZE to 2048 !! //

//
// situation | Mode
// ----------|-----
// Normal    | GPIO2 is used to control the NeoPixel
// Debug     | GPIO2 is connected to Serial1 to print debug messages
#define DEBUG

#ifdef DEBUG
 //#define DEBUG_PRINTF(format, ...) (Serial1.printf(format, __VA_ARGS__))
 #define DEBUG_PRINTF(format, ...) (Serial.printf(format, __VA_ARGS__))
#else
 #define DEBUG_PRINTF
#endif

// WiFi RESET pin
#define RST_PIN         4   // Wemos D4
#define RGB_R_PIN       15  // Wemos D8
#define RGB_G_PIN       5   // Wemos D1
#define RGB_B_PIN       14  // Wemos D5

#define MQTT_TOPIC_UPDATE_RATE_MS  30000

// Local variables
uint32_t cur=0, prev=0;
WiFiManager wifiManager;

typedef enum {
  RED = 0, GREEN, BLUE
} RGB_COLOR_ENUM;

typedef enum {
  OFF = 0, ON
} RGB_STATE_ENUM;

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
  // Define I/O and attach ISR
  pinMode(RST_PIN, INPUT);      // Reset
  pinMode(RGB_R_PIN, OUTPUT);   // Red RGB led
  pinMode(RGB_G_PIN, OUTPUT);   // Green RGB led
  pinMode(RGB_B_PIN, OUTPUT);   // Blue RGB led
  
  // Init with red led
  smartLedInit();
  for(int idx = 0; idx < 5; idx++ ) {
    smartLedFlash(RED);
    delay(100);
  }

  // Setup unique mqtt id and mqtt topic string
  create_unique_mqtt_topic_string(app_config.mqtt_topic);
  create_unigue_mqtt_id(app_config.mqtt_id);
  sprintf(mqtt_topic,"smartmeter/raw");

   // Perform factory reset switches
   // is pressed during powerup
   if( 0 == digitalRead(RST_PIN) ) {
      wifiManager.resetSettings();
      deleteAppConfig();
      while(0 == digitalRead(RST_PIN)) {
         smartLedFlash(RED);
         delay(500);
      }
      ESP.reset();
   }

  // Read config file or generate default
  if( !readAppConfig(&app_config) ) {
    strcpy(app_config.mqtt_username, "smartmeter");
    strcpy(app_config.mqtt_password, "se_smartmeter");
    strcpy(app_config.mqtt_remote_host, "sendlab.avansti.nl");
    strcpy(app_config.mqtt_remote_port, "11883");
    writeAppConfig(&app_config);
  }

  //
  smartLedColor(GREEN, ON);
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
  strcat(fd_str, "</b> Make a SCREENSHOT - you will need this info later!</p>");
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
  //Serial.swap();
  
  // Debug
  DEBUG_PRINTF("SDK Version: %s\n\r", ESP.getSdkVersion() );
  DEBUG_PRINTF("CORE Version: %s\n\r", ESP.getCoreVersion().c_str() );
  DEBUG_PRINTF("RESET: %s\n\r", ESP.getResetReason().c_str() );
  Serial.printf("OAT ready\n");
  Serial.printf("mqtt_username: %s\n", app_config.mqtt_username);
  Serial.printf("mqtt_password: %s\n", app_config.mqtt_password);
  Serial.printf("mqtt_id: %s\n", app_config.mqtt_id);
  Serial.printf("mqtt_topic: %s\n", mqtt_topic);
  Serial.printf("mqtt_remote_host: %s\n", app_config.mqtt_remote_host);
  Serial.printf("mqtt_remote_port: %s\n", app_config.mqtt_remote_port);
  smartLedColor(GREEN, ON);
  delay(1000);
  
  DEBUG_PRINTF("%s:freq: %d Mhz\n\r", __FUNCTION__, ESP.getCpuFreqMHz());
  delay(1000);

  // Initialise FSM
  initFSM(STATE_START, EV_IDLE);

  // Simulation P1 messages
  sim.attach(5, sim_callback);
}

//
void sim_callback() {
  strcpy(p1_buf,"/XMX5LGBBFG1012471273\r\n\r\n1-3:0.2.8(42)\r\n0-0:1.0.0(190927214707S)\r\n0-0:96.1.1(4530303331303033323530393235343136)\r\n1-0:1.8.1(008771.849*kWh)\r\n1-0:1.8.2(006475.011*kWh)\r\n1-0:2.8.1(003412.635*kWh)\r\n1-0:2.8.2(008741.572*kWh)\r\n0-0:96.14.0(0001)\r\n1-0:1.7.0(00.792*kW)\r\n1-0:2.7.0(00.000*kW)\r\n0-0:96.7.21(00004)\r\n0-0:96.7.9(00003)\r\n1-0:99.97.0(3)(0-0:96.7.19)(181124150040W)(0000001888*s)(180206113955W)(0000004457*s)(170905104526S)(0000003233*s)\r\n1-0:32.32.0(00000)\r\n1-0:32.36.0(00000)\r\n0-0:96.13.1()\r\n0-0:96.13.0()\r\n1-0:31.7.0(004*A)\r\n1-0:21.7.0(00.792*kW)\r\n1-0:22.7.0(00.000*kW)\r\n0-1:24.1.0(003)\r\n0-1:96.1.0(4730303235303033333133333737333135)\r\n0-1:24.2.1(190927210000S)(03749.001*m3)\r\n!1A6A\r\n");
  raiseEvent(EV_P1_AVAILABLE);
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

  // Check for IP connection 
  if( WiFi.status() == WL_CONNECTED) {

    // Handle mqtt
    if( !mqttClient.connected() ) {
      mqtt_connect();
      delay(250);
    } else {
      // Handle MQTT loop
      mqttClient.loop();
    }
  }

  // Capture P1 messages. If P1 msg is available raise MQTT event
  if( true == capture_p1() ) {
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
      fsm[state][event].heartbeat() ;
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
  mqttClient.setServer(host, port );
  if(mqttClient.connect(app_config.mqtt_id, app_config.mqtt_username, app_config.mqtt_password)){

    // Subscribe to mqtt topic
    mqttClient.subscribe(mqtt_topic);

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
void smartLedColor(RGB_COLOR_ENUM color, RGB_STATE_ENUM state)
/* 
short:         
inputs:        
outputs: 
notes:         
Version :      DMK, Initial code
*******************************************************************/
{
  switch( color ) {
    case RED:
      digitalWrite(RGB_R_PIN, state);
      break;
    case GREEN:
      digitalWrite(RGB_G_PIN, state);
      break;
    case BLUE:
      digitalWrite(RGB_B_PIN, state);
      break;
    default:
      break;
  }
}

/******************************************************************/
void smartLedFlash(RGB_COLOR_ENUM color)
/* 
short:      Flash current color         
inputs:        
outputs: 
notes:         
Version :   DMK, Initial code
*******************************************************************/
{
    switch( color ) {
    case RED:
      digitalWrite(RGB_R_PIN, ON);
      delay(100);
      digitalWrite(RGB_R_PIN, OFF);
      break;
    case GREEN:
      digitalWrite(RGB_G_PIN, ON);
      delay(100);
      digitalWrite(RGB_G_PIN, OFF);
      break;
    case BLUE:
      digitalWrite(RGB_B_PIN, ON);
      delay(100);
      digitalWrite(RGB_B_PIN, OFF);
      break;
    default:
      break;
  }
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
  digitalWrite(RGB_R_PIN, 0);
  digitalWrite(RGB_G_PIN, 0);
  digitalWrite(RGB_B_PIN, 0);
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
  // Enter idle mode
  raiseEvent(EV_IDLE);
}

/******************************************************************/
void start_heartbeat(void){
//  DEBUG_PRINTF(">%s:\n\r", __FUNCTION__);
}

/******************************************************************/
void start_post(void){
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);
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
void mqtt_heartbeat(void){
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);

  // Throttle mqqt topic speed: check if previous send MQTT
  // is at least MQTT_TOPIC_UPDATE_RATE_MS seconds ago
  //
  uint32_t mqtt_throttle_cur = millis();
  uint32_t mqtt_throttle_elapsed = mqtt_throttle_cur - mqtt_throttle_prev;
  if( mqtt_throttle_elapsed >= MQTT_TOPIC_UPDATE_RATE_MS ) {
    //
    mqtt_throttle_prev = mqtt_throttle_cur; 
  
    // Construct json object and publish
    DynamicJsonDocument doc(2048);
    doc["signature"] = app_config.mqtt_id;
    doc["datagram"] = p1_buf;
    String payload = "";
    serializeJson(doc, payload);
    mqttClient.publish(mqtt_topic, payload.c_str());
  }

  // Always back to idle
  raiseEvent(EV_IDLE);
}

/******************************************************************/
void mqtt_post(void){
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);
}
