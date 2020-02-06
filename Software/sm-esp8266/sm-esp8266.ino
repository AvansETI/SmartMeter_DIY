/*-------------------------------------------------------------------------
  Arduino sketch to mqtt Dutch Smart Meter P1 datagrams.

  See  for more information.

  V1.0: Initial: dkroeske(dkroeske@gmail.com), august 2019
  V1.1: Reroute pinning PCB, updated bootsequence
  V1.2: Updated to latest version ArduinoJson library (feb 2020)

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

#include "PubSubClient.h"
// MAKE SURE: in PubSubClient.h change MQTT_MAX_PACKET_SIZE to 2048 !! //

#define DEBUG

#ifdef DEBUG
 #define DEBUG_PRINTF(format, ...) (Serial1.printf(format, __VA_ARGS__))
#else
 #define DEBUG_PRINTF
#endif

//
// WeMos  ESP8266 Use Warning
// D0     GPIO16
// D1     GPIO5   SCL
// D2     GPIO4   SDA
// D3     GPIO0       Must be PULLED HIGH during boot (Pulled up on WeMos board)
// D4     GPIO2       Must be PULLED HIGH during boot (Pulled up on WeMos board)
// D5     GPIO14  SCL
// D6     GPIO12  MISO  
// D7     GPIO13
// D8     GPIO15      Boot mode, must be LOW during flash boot
// A0             Analog

#define RST_PIN         D2  // Wemos D2 (GPIO4)
#define RGB_R_PIN       D6  // Wemos D6 (GPIO12)
#define RGB_G_PIN       D1  // Wemos D1 (GPIO5)
#define RGB_B_PIN       D5  // Wemos D5 (GPIO14)

// Minimun delay between mqtt publish events. Prevents mqtt spam e.g. DSMR 5.0 updates every second!
#define MQTT_TOPIC_UPDATE_RATE_MS  20000

// Local variables
uint32_t cur=0, prev=0;
WiFiManager wifiManager;

typedef enum {
  RED = 0, GREEN, BLUE
} RGB_COLOR_ENUM;

typedef enum {
  ON = 0, OFF
} RGB_STATE_ENUM;

// Application configs struct. 
bool shouldSaveConfig;

#define MQTT_USERNAME_LENGTH       32
#define MQTT_PASSWORD_LENGTH       32
#define MQTT_ID_TOKEN_LENGTH       64
#define MQTT_TOPIC_STRING_LENGTH   64
#define MQTT_REMOTE_HOST_LENGTH   128
#define MQTT_REMOTE_PORT_LENGTH    10
#define P1_BAUDRATE_LENGTH         10


typedef struct {
   char     mqtt_username[MQTT_USERNAME_LENGTH];
   char     mqtt_password[MQTT_PASSWORD_LENGTH];
   char     mqtt_id[MQTT_ID_TOKEN_LENGTH];
   char     mqtt_topic[MQTT_TOPIC_STRING_LENGTH];
   char     mqtt_remote_host[MQTT_REMOTE_HOST_LENGTH];
   char     mqtt_remote_port[MQTT_REMOTE_PORT_LENGTH];
   char     p1_baudrate[P1_BAUDRATE_LENGTH];
} APP_CONFIG_STRUCT;

APP_CONFIG_STRUCT app_config;

WiFiClient wifiClient;

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
  pinMode(RST_PIN, INPUT_PULLUP); // Reset - Use internal pullup
  pinMode(RGB_R_PIN, OUTPUT);     // Red RGB led
  pinMode(RGB_G_PIN, OUTPUT);     // Green RGB led
  pinMode(RGB_B_PIN, OUTPUT);     // Blue RGB led
  
  // Init with red led
  smartLedInit();

  // Say Hello to user
  for(uint8_t idx = 0; idx < 2; idx++ ) {
    smartLedFlash(BLUE);
    delay(150);
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
       smartLedFlash(BLUE);
       delay(250);
    }
    ESP.reset();
  }

  // Read config file or generate default
  if( !readAppConfig(&app_config) ) {
    strcpy(app_config.mqtt_username, "smartmeter");
    strcpy(app_config.mqtt_password, "se_smartmeter");
    strcpy(app_config.mqtt_remote_host, "sendlab.avansti.nl");
    strcpy(app_config.mqtt_remote_port, "11883");
    strcpy(app_config.p1_baudrate, "115200");
    writeAppConfig(&app_config);
  }

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
  WiFiManagerParameter custom_mqtt_remote_port("mqtt_remote_port", "Port", app_config.mqtt_remote_port, MQTT_REMOTE_PORT_LENGTH);
  WiFiManagerParameter custom_p1_baudrate("p1_baudrate", "Baudrate", app_config.p1_baudrate, P1_BAUDRATE_LENGTH);

  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_remote_host);
  wifiManager.addParameter(&custom_mqtt_remote_port);
  wifiManager.addParameter(&custom_p1_baudrate);
  
  // Add the unit ID to the webpage
  char fd_str[128]="<p>Your EMON ID: <b>";
  strcat(fd_str, app_config.mqtt_topic);
  strcat(fd_str, "</b> Make a SCREENSHOT - you will need this info later!</p>");
  WiFiManagerParameter mqqt_topic_text(fd_str);
  wifiManager.addParameter(&mqqt_topic_text);

  // Blue led on. Will go GREEN if WiFi network is available or 
  // stays BLUE when WiFi credentials are needed.
  smartLedColor(BLUE, ON);
  
  if( !wifiManager.autoConnect("ETI EMON config")) {
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
    strcpy(app_config.p1_baudrate, custom_p1_baudrate.getValue());
    writeAppConfig(&app_config);
  }

  // Always print config to terminal before swapping serial port
  Serial.begin(115200, SERIAL_8N1);

  Serial.printf("\n");
  Serial.printf("************ DIY Smartmeter KIT********************\n");
  Serial.printf("ESP8266 info\n");
  Serial.printf("\tSDK Version     : %s\n", ESP.getSdkVersion() );
  Serial.printf("\tCore Version    : %s\n", ESP.getCoreVersion().c_str() );
  Serial.printf("\tCore Frequency  : %d Mhz\n", ESP.getCpuFreqMHz());
  Serial.printf("\tLast reset      : %s\n", ESP.getResetReason().c_str() );

  Serial.printf("MQTT settings\n");
  Serial.printf("\tmqtt_username   : %s\n", app_config.mqtt_username);
  Serial.printf("\tmqtt_password   : %s\n", app_config.mqtt_password);
  Serial.printf("\tmqtt_id         : %s\n", app_config.mqtt_id);
  Serial.printf("\tmqtt_topic      : %s\n", mqtt_topic);
  Serial.printf("\tmqtt_remote_host: %s\n", app_config.mqtt_remote_host);
  Serial.printf("\tmqtt_remote_port: %s\n", app_config.mqtt_remote_port);

  Serial.printf("DSMR settings\n");
  Serial.printf("\tP1 Baudrate     : %s baud\n", app_config.p1_baudrate);

  Serial.printf("***************************************************\n\n");

  Serial.flush();

  //
  if( !MDNS.begin("DIY-EMON_V12") ) {
  } else {
    MDNS.addService("diy_emon_v12", "tcp", 10000);
  }

  // Set P1 port baudrate. DSMR V2 uses 9600 baud. Otherwise 115200 baud
  long baudrate = atol(app_config.p1_baudrate);
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
    DEBUG_PRINTF("\n\r%s\n\r", "Debug mode ON ..." );
  #endif
 
  // Allow bootloader to connect: do not remove!
  delay(2000);
  
  // Relocate Serial Port
  Serial.swap();
  
  // Initialise FSM
  initFSM(STATE_START, EV_IDLE);
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
   char tmp[30];
   strcpy(topic_string,"EMON19V01");
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
       File configFile = SPIFFS.open("/config.json","r");
       if( configFile ) {

          size_t size = configFile.size();
          if (size > 1024) {
            Serial.println("Config file size is too large");
          }

          std::unique_ptr<char[]> buf(new char[size]);
          configFile.readBytes(buf.get(), size);
        
          StaticJsonDocument<512> doc;
          DeserializationError error = deserializeJson(doc, buf.get());
          
          if( error == DeserializationError::Ok ) {
             strcpy(app_config->mqtt_username, doc["MQTT_USERNAME"]);
             strcpy(app_config->mqtt_password, doc["MQTT_PASSWORD"]);
             strcpy(app_config->mqtt_remote_host, doc["MQTT_HOST"]);
             strcpy(app_config->mqtt_remote_port, doc["MQTT_PORT"]);
             strcpy(app_config->p1_baudrate, doc["P1_BAUDRATE"]);
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

  deleteAppConfig(); // Delete config file if exists

  StaticJsonDocument<512> doc;
  doc["MQTT_USERNAME"] = app_config->mqtt_username;
  doc["MQTT_PASSWORD"] = app_config->mqtt_password;
  doc["MQTT_HOST"] = app_config->mqtt_remote_host;
  doc["MQTT_PORT"] = app_config->mqtt_remote_port;
  doc["P1_BAUDRATE"]= app_config->p1_baudrate;
  
  File configFile = SPIFFS.open("/config.json","w+");
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
  if( SPIFFS.begin() ) {
    if( SPIFFS.exists("/config.json") ) {
      if( SPIFFS.remove("/config.json") ) {
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
  digitalWrite(RGB_R_PIN, 1);
  digitalWrite(RGB_G_PIN, 1);
  digitalWrite(RGB_B_PIN, 1);
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
    JsonObject root = doc.to<JsonObject>();
    
    JsonObject datagram = root.createNestedObject("datagram");
    datagram["p1"] = p1_buf;

    datagram["signature"] = app_config.mqtt_id;

    JsonObject s0 = datagram.createNestedObject("s0");
    s0["unit"] = "W";
    s0["label"] = "e-car charger";
    s0["value"] = 0;
    
    JsonObject s1 = datagram.createNestedObject("s1");
    s1["unit"] = "W";
    s1["label"] = "solar panels";
    s1["value"] = 0;
    
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
