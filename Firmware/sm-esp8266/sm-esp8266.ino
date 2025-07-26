/*-------------------------------------------------------------------------
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
  
  -------------------------------------------------------------------------
  
  Arduino sketch to mqtt Dutch Smart Meter P1 datagrams.

  See  for more information.

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

  Installation Arduino IDE:
  - How to get the Wemos installed in the Ardiuno IDE: https://siytek.com/wemos-d1-mini-arduino-wifi/
  - Install library WiFiManager by tablatronics: https://github.com/tzapu/WiFiManager
  - Install library JsonArduino by Banoit Blanchon: https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
  - Install library knolleary/PubSubClient by Nick O'Leary: https://github.com/knolleary/pubsubclient
 
  Happy Coding
  -------------------------------------------------------------------------*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Ticker.h>

#include "PubSubClient.h"

// Homeserver credentials
#include "MqttSendlab.h"

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

// Wifi client used for the MQTT library
WiFiClient mqttWifiClient;

// Only with some dummy values seems to work ... instead of mqttClient();
PubSubClient mqttClient("", 0, mqttWifiClient);
uint32_t mqttTimer = 0; // Time used to reconnect to the mqtt server, when disconnected (#26)
#define MQTT_RETRY_TIMEOUT 5000 // Every 5 seconds the mqtt server will try to reconnect (#26)

#define P1_TELEGRAM_SIZE   2048

// Datagram P1 buffer 
#define P1_MAX_DATAGRAM_SIZE 2048
char p1_buf[P1_MAX_DATAGRAM_SIZE]; // Complete P1 telegram
char *p1;

// TCP/IP server to implement the P1 datagram provider variables
WiFiServer tcpServer(3141); // TCP/IP server
WiFiClient tcpServerClient; // TCP/IP connected client, only one client is able to connect to the server

// HTTP Web server variables
#define WEBSERVERDATALENGTH 12*3 // Data points that will be stored
#define WEBSERVERDATASAMPLERATE 1000*60 // Sample rate to collect the data points in ms
ESP8266WebServer server(80);   // WebServer
bool webServerInitialized = false; 
uint16_t webDataPointer = 0; // Pointer to the insert point
uint32_t webserverTimer = 0; // Time used to implement the sample rate
void addWebDataP1(char* p1); // Add data point to the data store from P1 message
void handleRoot(); // Handle root page callback
void handleDataApi(); // Handle data page/api callback
void handleNotFound(); // Handle not found page callback

// Varibles to store the P1 data that is provided to the webpage
char DSMRVersion[5] = "-";
char DSMRTimestamp[14] = "-";
float dataActualPowerConsumption[WEBSERVERDATALENGTH];  // Actual power consumption kW
float dataActualPowerProduction[WEBSERVERDATALENGTH];  // Actual power production kW
float dataEnergyConsumption1[WEBSERVERDATALENGTH]; // Energy consumption 1 kWh
float dataEnergyConsumption2[WEBSERVERDATALENGTH]; // Energy consumption 2 kWh
float dataEnergyProduction1[WEBSERVERDATALENGTH]; // Energy production 1 kWh
float dataEnergyProduction2[WEBSERVERDATALENGTH]; // Energy production 1 kWh

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
    ESP.reset();
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

  // Setup TCP/IP server
  tcpServer.begin();
  
  // Web server initialization
  server.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/data", handleDataApi);        // Call the 'handleDataApi' function when a client requests URI "/data"
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin();                           // Actually start the server

  // Initialize the data stores with zero
  for ( uint16_t i=0; i < WEBSERVERDATALENGTH; i++ ) {
    dataActualPowerConsumption[i] = 0; // Actual power consumpation kW
    dataActualPowerProduction[i] = 0; // Actual power production kW
    dataEnergyConsumption1[i] = 0; // Energy 1 consumption Kwh
    dataEnergyConsumption2[i] = 0; // Energy 2 consumption kWh
    dataEnergyProduction1[i] = 0; // Energy 1 production kWh
    dataEnergyProduction2[i] = 0; // Energy 2 production kWh
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
  Serial.printf("\tIP address      : %s\n", WiFi.localIP().toString().c_str());

  Serial.printf("DSMR settings\n");
  Serial.printf("\tP1 Baudrate     : %s baud\n", app_config.p1_baudrate);

  // Setup mDNS Service
  if ( MDNS.begin("diy_smartmeter") ) { 
    MDNS.addService("http", "tcp", 80);     // Webserver
    MDNS.addService("p1data", "tcp", 3141); // TCP/IP P1 data provider server
    Serial.printf("mDNS\n");
    Serial.printf("\tmDNS URL        : %s\n", "diy_smartmeter.local");
    Serial.printf("\tWeb server      : %s\n", "diy_smartmeter.local:80");
    Serial.printf("\tData server     : %s\n", "diy_smartmeter.local:3141");
  } else {
    Serial.printf("mDNS:   Could not start the mDNS service!\n");
  }

  Serial.printf("***************************************************\n\n");
  Serial.flush();

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
notes:         MS, Not full implementation of FSM; a lot of logic still in loop()
Version :      DMK, Initial code
*******************************************************************/
{

  // Check for IP connection 
  if( WiFi.status() == WL_CONNECTED) {

    // Handle mqtt, if not connected it uses a timer to reconnect every MQTT_RETRY_TIMEOUT ms. (#26)
    if( !mqttClient.connected() && ( mqttTimer == 0 || millis() - mqttTimer > MQTT_RETRY_TIMEOUT ) ) {
      smartLedFlash(RED); // Added to see when MQTT is not connected (#26: causing a delay of 150ms)
      mqtt_connect();
      //delay(250); #26: removed, while it causes problems for the MDNS, HTTP and TCP server updates
      mqttTimer = millis(); // Set timer to reconnect over MQTT_RETRY_TIMEOUT ms (#26)

    } else {
      // Handle MQTT loop
      mqttClient.loop();
    }

    // Handle mDNS service
    MDNS.update();

    // Handle HTTP web server
    server.handleClient(); // Listen for HTTP requests from clients

    // Handle client connection to the TCP/IP server
    if (tcpServer.hasClient() ) {
      if ( !tcpServerClient || !tcpServerClient.connected() ) { // Check if free or disconnected
        if ( tcpServerClient ) {
          tcpServerClient.stop();
        }
        tcpServerClient = tcpServer.accept();
        char t[] = "Smartmeter P1\n";
        tcpServerClient.write(t, strlen(t));
      }
    }
  }

  // Capture P1 messages. If P1 msg is available raise MQTT event
  if( true == capture_p1() ) {
    if ( millis() > webserverTimer + WEBSERVERDATASAMPLERATE ) {
      addWebDataP1(p1_buf);
      webserverTimer = millis();
    }
    if ( tcpServerClient.connected() ) { // Send the P1 data to the connected client
      tcpServerClient.write(p1_buf, strlen(p1_buf));
    }
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
  
  if( LittleFS.begin() ) {
    if( LittleFS.exists("/config.json") ) {
       File configFile = LittleFS.open("/config.json","r");
       if( configFile ) {

          size_t size = configFile.size();
          if (size > 1024) {
            Serial.println("Config file size is too large");
          }

          std::unique_ptr<char[]> buf(new char[size]);
          configFile.readBytes(buf.get(), size);
        
          //StaticJsonDocument<512> doc; // migration
          JsonDocument doc;
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

  //StaticJsonDocument<512> doc; // migration
  JsonDocument doc;
  doc["MQTT_USERNAME"] = app_config->mqtt_username;
  doc["MQTT_PASSWORD"] = app_config->mqtt_password;
  doc["MQTT_HOST"] = app_config->mqtt_remote_host;
  doc["MQTT_PORT"] = app_config->mqtt_remote_port;
  doc["P1_BAUDRATE"]= app_config->p1_baudrate;
  
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
void mqtt_heartbeat(void) {
  DEBUG_PRINTF("%s:\n\r", __FUNCTION__);

  // Throttle mqtt topic speed: check if previous send MQTT
  // is at least MQTT_TOPIC_UPDATE_RATE_MS seconds ago
  //
  uint32_t mqtt_throttle_cur = millis();
  uint32_t mqtt_throttle_elapsed = mqtt_throttle_cur - mqtt_throttle_prev;
  if( mqtt_throttle_elapsed >= MQTT_TOPIC_UPDATE_RATE_MS ) {

    //
    mqtt_throttle_prev = mqtt_throttle_cur; 
  
    // Construct json object and publish
    //DynamicJsonDocument doc(2048); // migration
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    
    //JsonObject datagram = root.createNestedObject("datagram"); // migration
    JsonObject datagram = root["datagram"].to<JsonObject>();
    datagram["p1"] = p1_buf;

    datagram["signature"] = app_config.mqtt_id;

    //JsonObject s0 = datagram.createNestedObject("s0"); // migration
    JsonObject s0 = datagram["s0"].to<JsonObject>();
    s0["unit"] = "W";
    s0["label"] = "e-car charger";
    s0["value"] = 0;
    
    //JsonObject s1 = datagram.createNestedObject("s1"); // migration
    JsonObject s1 = datagram["s1"].to<JsonObject>();
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

/******************************************************************
*
* HTTP Web Server section
*
******************************************************************/

/******************************************************************/
void handleRoot() {
  String rootHtml = R"(
<!doctype html>
<html lang="en" data-bs-theme="dark">
<html>
 <head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
  <title>SmartMeter DIY</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH" crossorigin="anonymous">
  <script src="https://code.jquery.com/jquery-3.7.1.min.js" integrity="sha256-/JqT3SQfawRcv/BIHPThkBvs0OEvtFFmqPF/lYI/Cxo=" crossorigin="anonymous"></script>
  <script src="https://cdn.jsdelivr.net/npm/@popperjs/core@2.11.8/dist/umd/popper.min.js" integrity="sha384-I7E8VVD/ismYTF4hNIPjVp/Zjvgyol6VFvRkX/vR+Vc4jQkC+hVqc2pM8ODewa9r" crossorigin="anonymous"></script>
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz" crossorigin="anonymous"></script>
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.6/dist/chart.umd.min.js" integrity="sha384-Sse/HDqcypGpyTDpvZOJNnG0TT3feGQUkF9H+mnRvic+LjR+K1NhTt8f51KIQ3v3" crossorigin="anonymous"></script>
 </head>
 <body>
  <script>
$(document).ready(function(){
 $.ajax({
    url: "https://raw.githubusercontent.com/AvansETI/SmartMeter_DIY/refs/heads/master/Firmware/sm-esp8266/web/body.html",
    success: function (data) { $('body').append(data); },
    dataType: 'html'
 });
});
  </script>
 </body>
</html>)";
  server.send(200, "text/html", rootHtml);
}

/******************************************************************/
void handleDataApi() {
  String dataJson = "{\"power_consumption\":[";
  for ( uint16_t i=0; i < webDataPointer-1; i++ ) {
    dataJson = dataJson + dataActualPowerConsumption[i] + ",";
  }
  dataJson = dataJson + dataActualPowerConsumption[webDataPointer-1] + "],\"power_production\":[";
  for ( uint16_t i=0; i < webDataPointer-1; i++ ) {
    dataJson = dataJson + dataActualPowerProduction[i] + ",";
  }
  dataJson = dataJson + dataActualPowerProduction[webDataPointer-1] + "],\"energy_consumption1\":[";
  for ( uint16_t i=0; i < webDataPointer-1; i++ ) {
    dataJson = dataJson + dataEnergyConsumption1[i] + ",";
  }
  dataJson = dataJson + dataEnergyConsumption1[webDataPointer-1] + "],\"energy_consumption2\":[";
  for ( uint16_t i=0; i < webDataPointer-1; i++ ) {
    dataJson = dataJson + dataEnergyConsumption2[i] + ",";
  }
  dataJson = dataJson + dataEnergyConsumption2[webDataPointer-1] + "],\"energy_production1\":[";
  for ( uint16_t i=0; i < webDataPointer-1; i++ ) {
    dataJson = dataJson + dataEnergyProduction1[i] + ",";
  }
  dataJson = dataJson + dataEnergyProduction1[webDataPointer-1] + "],\"energy_production2\":[";
  for ( uint16_t i=0; i < webDataPointer-1; i++ ) {
    dataJson = dataJson + dataEnergyProduction2[i] + ",";
  }
  dataJson = dataJson + dataEnergyProduction2[webDataPointer-1] + "],\"DSMRVersion\":\"" + DSMRVersion +
             "\",\"DSMRTimestamp\":\"" + DSMRTimestamp + "\"}";

  server.send(200, "text/json", dataJson);
}

/******************************************************************/
void handleNotFound () {
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

/******************************************************************/
/* Documentation
  - https://github.com/energietransitie/dsmr-info/blob/main/dsmr-p1-specs.csv
  - https://github.com/energietransitie/dsmr-info/blob/main/dsmr-e-meters.csv
  - https://github.com/reneklootwijk/node-dsmr/tree/master
*/
void addWebDataP1(char* p1) {
  char keys[9][10] = {
    "1-3:0.2.8", // DMSR version -> 1-3:0.2.8(50)
    "0-0:1.0.0", // Timestamp    -> 0-0:1.0.0(241221224725W)
    "1-0:1.8.1", // Total consumption tarrif 1 -> 1-0:1.8.1(007812.965*kWh)
    "1-0:1.8.2", // Total consumption tarrif 2 -> 1-0:1.8.2(004695.310*kWh)
    "1-0:2.8.1", // Total production tarrif 1 -> 1-0:2.8.1(002313.919*kWh)
    "1-0:2.8.2", // Total production tarrif 2 -> 1-0:2.8.2(005836.025*kWh)
    "0-0:96.14", // Actual tarrif -> 0-0:96.14.0(0001)
    "1-0:1.7.0", // Actual consumption -> 1-0:1.7.0(00.670*kW)
    "1-0:2.7.0", // Actual production -> 1-0:2.7.0(00.000*kW)  
  };

  if ( webDataPointer == WEBSERVERDATALENGTH ) { // shift the values to the left
    for ( uint16_t i=0; i < WEBSERVERDATALENGTH - 1; i++ ) {
      dataActualPowerConsumption[i] = dataActualPowerConsumption[i+1];
      dataActualPowerProduction[i] = dataActualPowerProduction[i+1];
      dataEnergyConsumption1[i] = dataEnergyConsumption1[i+1];
      dataEnergyConsumption2[i] = dataEnergyConsumption2[i+1];
      dataEnergyProduction1[i] = dataEnergyProduction1[i+1];
      dataEnergyProduction2[i] = dataEnergyProduction2[i+1];
      webDataPointer = WEBSERVERDATALENGTH - 1; // Set pointer to last element
    }
  }

  bool found;
  size_t p1_length = strlen(p1);
  for ( uint16_t i=0; i < p1_length - 10; i++ ) { // Process the datagram
    for (uint8_t k=0; k < 9; k++ ) {
      found = true;
      for ( uint8_t j=0; j < 9; j++ ) { // Search for key
        if ( p1[i+j] != keys[k][j] ) {
          found = false;
          break;
        }
      }
      if ( found ) { // found the key
        char temp[20] = "";
        switch (k) {
          case 0:
            strncpy(DSMRVersion, (const char*) p1+i+9+1, 2); // copy version
            break;
          case 1:
            strncpy(DSMRTimestamp, (const char*) p1+i+9+1, 13); // copy timestamp
            break;
          case 2:
            strncpy(temp, (const char*) p1+i+9+1, 10); // copy consumption tarrif 1
            dataEnergyConsumption1[webDataPointer] = atof(temp);
            break;
          case 3:
            strncpy(temp, (const char*) p1+i+9+1, 10); // copy consumption tarrif 2
            dataEnergyConsumption2[webDataPointer] = atof(temp);
            break;
          case 4:
            strncpy(temp, (const char*) p1+i+9+1, 10); // copy production tarrif 1
            dataEnergyProduction1[webDataPointer] = atof(temp);
            break;
          case 5:
            strncpy(temp, (const char*) p1+i+9+1, 10); // copy production tarrif 2
            dataEnergyProduction2[webDataPointer] = atof(temp);
            break;
          case 6:
            strncpy(temp, (const char*) p1+i+9+3, 4); // actual tarrif
            break;
          case 7:
            strncpy(temp, (const char*) p1+i+9+1, 6); // actual consumption
            dataActualPowerConsumption[webDataPointer] = atof(temp);
            break;
          case 8:
            strncpy(temp, (const char*) p1+i+9+1, 6); // actual production
            dataActualPowerProduction[webDataPointer] = atof(temp);
            break;
        }
      }
    }
  }
  webDataPointer++;
}
