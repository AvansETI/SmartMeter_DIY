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

  This is a inline header file that contains both declarations and implemenation
  of the library and it is part of the DIY SMARTMETER project. It is developed
  for both the ESP8266 (Wemos D1 mini) and ESP32S2 (Lolin S2 mini). Configuration
  of this library will be done in this library in the top of the file itself.

  Within this library a data TCP/IP server is implemented that provides P1 data.

  V1.0  Initial version
  -------------------------------------------------------------------------*/

// Configuration part of the library
#define P1_DATA_SERVER_PORT                 3141
#define P1_DATA_SERVER_MAX_CLIENTS          2 // Note: Too many clients will have effect on performance (1-5)
#define P1_DATA_SERVER_WRITE_TIMEOUT_MS     1000  // 1 second timeout for writes
#define P1_DATA_SERVER_MAX_WRITE_CHUNK      512   // Maximum bytes to write in one chunk
const char P1_DATA_SERVER_WELCOME_MSG[] PROGMEM = "DIY Smartmeter P1\n";
const char P1_DATA_SERVER_TOO_MANY_CLIENTS_MSG[] PROGMEM = "DIY Smartmeter P1 - too many clients connected.\n";

// Includes
#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>

#elif defined(ESP32)
#include <WiFi.h>
#endif

/*
  Class: Dashboard
*/
class P1DataServer {
private:
  WiFiServer tcpServer; // TCP/IP server
  WiFiClient tcpServerClient[P1_DATA_SERVER_MAX_CLIENTS]; // TCP/IP connected clients

  // Helper method to yield control and feed watchdog
  void yieldAndFeedWDT() {
    yield();
    #if defined(ESP8266)
    ESP.wdtFeed();
    #endif
  }

  // Safe write with timeout and chunking
  bool safeWrite(WiFiClient& client, const char* data, size_t len) {
    if (!client.connected()) {
      return false;
    }
    
    unsigned long startTime = millis();
    size_t written = 0;
    
    while (written < len) {
      if (millis() - startTime > P1_DATA_SERVER_WRITE_TIMEOUT_MS) {  // Check timeout
        return false;
      }
      
      size_t chunkSize = min(len - written, (size_t)P1_DATA_SERVER_MAX_WRITE_CHUNK); // Calculate chunk size
      
      size_t bytesWritten = client.write(data + written, chunkSize); // Try to write chunk
      if (bytesWritten == 0) {
        yieldAndFeedWDT(); // Write failed or buffer full, yield and try again
        delay(1); // Small delay to allow buffer to drain
        continue;
      }
      
      written += bytesWritten;
      yieldAndFeedWDT(); // Yield between chunks
    }
    
    return true;
  }

public:

  /******************************************************************/
  P1DataServer (): tcpServer(P1_DATA_SERVER_PORT)
  /* 
  short:   Constructor of the class.        
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
  }

  /******************************************************************/
  void begin ()
  /* 
  short:   Arduino convention to setup the library using begin().        
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    this->tcpServer.begin();
  }

  /******************************************************************/
  void loop () 
  /* 
  short:   Method is required to be called in the main loop() of the sketch to handle the library functionality.
           It handles the incoming clients and connected clients.    
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    this->yieldAndFeedWDT();

    // Handle the TCP data server clients
    uint8_t i = 0;
    WiFiClient client = this->tcpServer.accept();
    if (client) { // we have a new client
      bool foundAvailableSlot = false;
      while ( !foundAvailableSlot && i < P1_DATA_SERVER_MAX_CLIENTS ) {
        if ( !this->tcpServerClient[i].connected() ) {
          this->tcpServerClient[i] = client;
          this->tcpServerClient[i].setNoDelay(true);

          // Send welcome message safely
          const char* welcomeMsg = P1_DATA_SERVER_WELCOME_MSG;
          size_t msgLen = strlen_P(welcomeMsg);
          
          if (!this->safeWrite(this->tcpServerClient[i], welcomeMsg, msgLen)) {
            this->tcpServerClient[i].stop();
          }
          //this->tcpServerClient[i].write_P(P1_DATA_SERVER_WELCOME_MSG, strlen_P(P1_DATA_SERVER_WELCOME_MSG)); // constant stored in flash!
          foundAvailableSlot = true;
        }
        this->yieldAndFeedWDT(); // Yield during client search
        i++;
      }

      if ( !foundAvailableSlot ) { // No client found, all clients are already connected
        const char* rejectMsg = P1_DATA_SERVER_TOO_MANY_CLIENTS_MSG;
        size_t msgLen = strlen_P(rejectMsg);
        this->safeWrite(client, rejectMsg, msgLen);
        //client.write_P(P1_DATA_SERVER_TOO_MANY_CLIENTS_MSG, strlen_P(P1_DATA_SERVER_TOO_MANY_CLIENTS_MSG));
        client.stop();
      }      
    }
  }

  /******************************************************************/
  void sendP1 (char* p1)
  /* 
  short:   Sends the P1 message to all the connected clients.    
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    if (!p1 || strlen(p1) == 0) {
      return; // Nothing to send
    }

    for ( uint8_t i=0; i < P1_DATA_SERVER_MAX_CLIENTS; i++ ) {
      if ( this->tcpServerClient[i].connected() ) { // Send the P1 data to the connected clients
        if (!safeWrite(tcpServerClient[i], p1, strlen(p1))) {
          tcpServerClient[i].stop();
        }
        //this->tcpServerClient[i].write(p1, strlen(p1));
      }
      yieldAndFeedWDT(); // Yield between clients
    }
  }

  /******************************************************************/
  static bool anonymizeP1(char* p1)
  /* 
  short:      Anonymize P1 data by removing the equipment IDs found in the message         
  inputs:     Pointer to the p1 message   
  outputs:    Returns true when parsed successfull, otherwise false.
  notes:      0-0:96.1.1(**EQUIPMENT-ID**) => :96.1. is always equipment identifiers         
  Version :   MS, Initial code
  *******************************************************************/
  {
    char* indexEqId = strstr(p1, ":96.1.");
    while ( indexEqId != NULL ) { // Found an equipment ID tag

      char* indexStartId = strchr(indexEqId, '(');
      char* indexEndId = strchr(indexEqId, ')');

      if ( indexStartId != NULL && indexEndId != NULL && indexEndId > indexStartId ) { // Found the equipment ID
        for ( uint16_t i=1; i < indexEndId - indexStartId; i++ ) { // Replace equipment ID with zero's
          indexStartId[i] = '0';
        }

      } else {
        return false;
      }

      indexEqId = strstr(indexEndId, ":96.1."); // Find the next equipment identifier
    }

    return true;
  }

};