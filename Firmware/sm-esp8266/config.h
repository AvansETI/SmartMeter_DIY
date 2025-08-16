/*
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
*/

// Remote MQTT server configuration
#define MQTT_USERNAME             "smartmeter"
#define MQTT_PASSWORD             "se_smartmeter"
#define MQTT_REMOTE_HOST          "mqtt.sendlab.nl"
#define MQTT_REMOTE_PORT          "11883"
#define MQTT_TOPIC                "smartmeter/raw"
#define MQTT_MSGBUF_SIZE          2048
#define MQTT_RETRY_TIMEOUT        60000
#define MQTT_TOPIC_UPDATE_RATE_MS 20000 // Minimun delay between mqtt publish events. Prevents mqtt spam e.g. DSMR 5.0 updates every second!
#define MQTT_ANONIMIZE_P1         "YES" // YES/NO, default YES while this data is going to an external server


// MQTT client configuration
#define P1_TELEGRAM_SIZE           2048
#define P1_MAX_DATAGRAM_SIZE       2048
#define MQTT_USERNAME_LENGTH       32
#define MQTT_PASSWORD_LENGTH       32
#define MQTT_ID_TOKEN_LENGTH       64
#define MQTT_TOPIC_STRING_LENGTH   64
#define MQTT_REMOTE_HOST_LENGTH    128
#define MQTT_REMOTE_PORT_LENGTH    10
#define P1_BAUDRATE_LENGTH         10
#define MQTT_ANONIMIZE_P1_LENGTH   32

// Local services configuration
#define TCP_DATA_SERVER_PORT        3141
#define TCP_DATA_SERVER_MAX_CLIENTS 2       // Too many clients will have effect on performance (1-5)
#define TCP_ANONIMIZE_P1            "NO"    // YES/NO, default NO while this data stays in the local network
#define TCP_ANONIMIZE_P1_LENGTH     32
#define HTTP_SERVER_DATA_LENGTH     12*3    // Data points that will be stored
#define HTTP_SERVER_SAMPLE_RATE     1000*60 // Sample rate to collect the data points in ms
