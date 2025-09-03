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

  Within this library a web server is implemented that uses a low flash and memory
  footprint.

  V1.0  Initial version
  -------------------------------------------------------------------------*/

// Configuration part of the library
#define WEB_SERVER_PORT            80
#define WEB_SERVER_DATA_LENGTH     12*3    // Data points that will be stored
#define WEB_SERVER_SAMPLE_RATE     1000*60 // Sample rate to collect the data points in ms

// Includes
#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WebServer.h>

#elif defined(ESP32)
#include <WebServer.h>
#endif

const char rootHtml[] = R"(
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

/*
  Class: Dashboard
*/
class Dashboard {
private:
#if defined(ESP8266)
  ESP8266WebServer server;
#elif defined(ESP32)
  WebServer server;
#endif
  uint16_t dataPointer; // Pointer to the insert point
  uint32_t serverTimer; // Time used to implement the sample rate

  // Varibles to store the P1 data that is provided to the webpage
  char DSMRVersion[5];
  char DSMRTimestamp[14];
  float actualPowerConsumption[WEB_SERVER_DATA_LENGTH];  // Actual power consumption kW
  float actualPowerProduction[WEB_SERVER_DATA_LENGTH];  // Actual power production kW
  float energyConsumption1[WEB_SERVER_DATA_LENGTH]; // Actual energy consumption 1 kWh
  float energyConsumption2[WEB_SERVER_DATA_LENGTH]; // Actual energy consumption 2 kWh
  float energyProduction1[WEB_SERVER_DATA_LENGTH]; // Actual energy production 1 kWh
  float energyProduction2[WEB_SERVER_DATA_LENGTH]; // Actual energy production 1 kWh

public:

  /******************************************************************/
  Dashboard (): server(WEB_SERVER_PORT)
  /* 
  short:   Constructor of the class.    
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    this->dataPointer = 0;
    this->serverTimer = 0;

    strcpy(DSMRVersion, "-");
    strcpy(DSMRTimestamp, "-");
    
    // Initialize the data stores with zero
    for ( uint16_t i=0; i < WEB_SERVER_DATA_LENGTH; i++ ) {
      this->actualPowerConsumption[i] = 0; // Actual power consumpation kW
      this->actualPowerProduction[i] = 0; // Actual power production kW
      this->energyConsumption1[i] = 0; // Energy 1 consumption Kwh
      this->energyConsumption2[i] = 0; // Energy 2 consumption kWh
      this->energyProduction1[i] = 0; // Energy 1 production kWh
      this->energyProduction2[i] = 0; // Energy 2 production kWh
    }
  }

  /******************************************************************/
  void begin ()
  /* 
  short:   Arduino convention to setup the library using begin().
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
           Add new data api's to the webserver to serve firmware/SmartMeter 
           DIY information (info), actual power/energy (actual), power 
           history (power), energy history (energy)
  *******************************************************************/
  {
    this->server.on("/", std::bind(&Dashboard::handleRoot, this));               // Call the 'handleRoot' function when a client requests URI "/"
    this->server.on("/data", std::bind(&Dashboard::handleDataApi, this));        // Call the 'handleDataApi' function when a client requests URI "/data"
    this->server.on("/info", std::bind(&Dashboard::handleInfoApi, this));
    this->server.on("/actual", std::bind(&Dashboard::handleActualApi, this));
    this->server.on("/power", std::bind(&Dashboard::handlePowerApi, this));
    this->server.on("/energy", std::bind(&Dashboard::handleEnergyApi, this));
    this->server.onNotFound(std::bind(&Dashboard::handleNotFound, this));        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
    this->server.begin(); // Actually start the server
  }

  /******************************************************************/
  void loop ()
  /* 
  short:   Method is required to be called in the main loop() of the sketch to handle the library functionality.
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    this->server.handleClient(); // Listen for HTTP requests from clients
  }

  /******************************************************************/
  void handleRoot()
  /* 
  short:   Returns the HTML page when the root page is called. This HTML must be
           limited in Flash and RAM space and should load all complex functionality
           and designs from the Internet. The body shall be loaded from an external
           source like a github repo.
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/  
  {
    server.send(200, "text/html", rootHtml);
  }

  void sendJSON(const char* data) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");  
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200, "text/json", data);
  }

  /******************************************************************/
  void handleDataApi()
  /* 
  short:   Returns a JSON with the data that is collected.
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    char json[1024] = "";
    int offset = sprintf(json, "{\"power_consumption\":[");
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      offset  += sprintf(json + offset,  "%f,", actualPowerConsumption[i]);
    }

    offset    += sprintf(json + offset,  "%f],\"power_production\":[", actualPowerConsumption[this->dataPointer-1]);
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      offset  += sprintf(json + offset,  "%f,", actualPowerProduction[i]);
    }

    offset    += sprintf(json + offset,  "%f],\"energy_consumption1\":[", actualPowerProduction[this->dataPointer-1]);
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      offset  += sprintf(json + offset,  "%f,", energyConsumption1[i]);
    }

    offset    += sprintf(json + offset,  "%f],\"energy_consumption2\":[", energyConsumption1[this->dataPointer-1]);
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      offset  += sprintf(json + offset,  "%f,", energyConsumption2[i]);
    }

    offset    += sprintf(json + offset,  "%f],\"energy_production1\":[", energyConsumption2[this->dataPointer-1]);
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      offset  += sprintf(json + offset,  "%f,", energyProduction1[i]);
    }

    offset    += sprintf(json + offset,  "%f],\"energy_production2\":[", energyProduction1[this->dataPointer-1]);
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      offset  += sprintf(json + offset,  "%f,", energyProduction2[i]);
    }
    offset    += sprintf(json + offset,  "%f],\"DSMRVersion\":\"%s\",\"DSMRTimestamp\":\"%s\"}",
       energyProduction2[this->dataPointer-1], DSMRVersion, DSMRTimestamp);

    sendJSON(json);
  }

  /******************************************************************/
  void handleActualApi()
  /* 
  short:   Returns a JSON with the data that is collected.
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    char json[1024] = "";
    sendJSON(json);
  }

  /******************************************************************/
  void handlePowerApi()
  /* 
  short:   Returns a JSON with the data that is collected.
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    char json[1024] = "";
    sendJSON(json);
  }

  /******************************************************************/
  void handleEnergyApi()
  /* 
  short:   Returns a JSON with the data that is collected.
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    char json[1024] = "";
    sendJSON(json);
  }

  /******************************************************************/
  void handleInfoApi()
  /* 
  short:   Returns a JSON with the data that is collected.
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    char json[1024] = "";
#if defined(ESP8266)
    int offset = sprintf(json, "{\"hardware\": \"ESP8266\",");
    offset    += sprintf(json + offset,  "\"sdk\":\"%s\",", ESP.getSdkVersion());
    offset    += sprintf(json + offset,  "\"core\":\"%s\",", ESP.getCoreVersion().c_str());
    offset    += sprintf(json + offset,  "\"freq\":\"%d\",", ESP.getCpuFreqMHz());
    offset    += sprintf(json + offset,  "\"reset\":\"%s\",", ESP.getResetReason().c_str());

#elif defined(ESP32)
    char resetReason[20];
    getResetReason(resetReason);
    int offset = sprintf(json, "{\"hardware\": \"ESP32S2\",");
    offset    += sprintf(json + offset,   "\"sdk\":\"%s\",", ESP.getSdkVersion());
    offset    += sprintf(json + offset,   "\"core\":\"%s\",", ESP.getCoreVersion());
    offset    += sprintf(json + offset,   "\"freq\":\"%ld\",", ESP.getCpuFreqMHz());
    offset    += sprintf(json + offset,  "\"reset\":\"%s\",", resetReason);
#endif
    offset    += sprintf(json + offset,   "\"id\":\"%s\",", app_config.mqtt_id);
    offset    += sprintf(json + offset,   "\"online\":\"%s\",", "YES"); // @TODO
    offset    += sprintf(json + offset,   "\"ip\":\"%s\",", WiFi.localIP().toString().c_str());
    offset    += sprintf(json + offset,   "\"mqtt_anon\":\"%s\",", app_config.mqtt_anonimize_p1);
    offset    += sprintf(json + offset,   "\"dsmr_baud\":\"%s\",", app_config.p1_baudrate);
    offset    += sprintf(json + offset,   "\"client_auth\":\"%s\",", app_config.sec_authentication);
    offset    += sprintf(json + offset,   "\"tcp_anon\":\"%s\"}", app_config.tcp_anonimize_p1);

    sendJSON(json);
  }

  /******************************************************************/
  void handleNotFound ()
  /* 
  short:   Returns a simple text page that shows that the page is not found.
  inputs:        
  outputs: 
  notes:         
  Version: MS, Initial code
  *******************************************************************/
  {
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
  }

  /******************************************************************/
  void processP1(char* p1)
  /* 
  short:   Processes the P1 message and collects the data that is required for the web
           server. 
  inputs:  char* p1 that points to the p1 datagram
  outputs: 
  notes:   https://github.com/energietransitie/dsmr-info/blob/main/dsmr-p1-specs.csv
           https://github.com/energietransitie/dsmr-info/blob/main/dsmr-e-meters.csv
           https://github.com/reneklootwijk/node-dsmr/tree/master  
  Version: MS, Initial code
  *******************************************************************/
  {
    if ( millis() < this->serverTimer + WEB_SERVER_SAMPLE_RATE ) return;
    this->serverTimer = millis(); // Reset the sample rate timer

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

    if ( this->dataPointer == WEB_SERVER_DATA_LENGTH ) { // shift the values to the left
      for ( uint16_t i=0; i < WEB_SERVER_DATA_LENGTH - 1; i++ ) {
        this->actualPowerConsumption[i] = this->actualPowerConsumption[i+1];
        this->actualPowerProduction[i] = this->actualPowerProduction[i+1];
        this->energyConsumption1[i] = this->energyConsumption1[i+1];
        this->energyConsumption2[i] = this->energyConsumption2[i+1];
        this->energyProduction1[i] = this->energyProduction1[i+1];
        this->energyProduction2[i] = this->energyProduction2[i+1];
        this->dataPointer = WEB_SERVER_DATA_LENGTH - 1; // Set pointer to last element
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
              strncpy(this->DSMRVersion, (const char*) p1+i+9+1, 2); // copy version
              break;
            case 1:
              strncpy(this->DSMRTimestamp, (const char*) p1+i+9+1, 13); // copy timestamp
              break;
            case 2:
              strncpy(temp, (const char*) p1+i+9+1, 10); // copy consumption tarrif 1
              this->energyConsumption1[this->dataPointer] = atof(temp);
              break;
            case 3:
              strncpy(temp, (const char*) p1+i+9+1, 10); // copy consumption tarrif 2
              this->energyConsumption2[this->dataPointer] = atof(temp);
              break;
            case 4:
              strncpy(temp, (const char*) p1+i+9+1, 10); // copy production tarrif 1
              this->energyProduction1[this->dataPointer] = atof(temp);
              break;
            case 5:
              strncpy(temp, (const char*) p1+i+9+1, 10); // copy production tarrif 2
              this->energyProduction2[this->dataPointer] = atof(temp);
              break;
            case 6:
              strncpy(temp, (const char*) p1+i+9+3, 4); // actual tarrif
              break;
            case 7:
              strncpy(temp, (const char*) p1+i+9+1, 6); // actual consumption
              this->actualPowerConsumption[this->dataPointer] = atof(temp);
              break;
            case 8:
              strncpy(temp, (const char*) p1+i+9+1, 6); // actual production
              this->actualPowerProduction[this->dataPointer] = atof(temp);
              break;
          }
        }
      }
    }
    this->dataPointer++;
  }

};
