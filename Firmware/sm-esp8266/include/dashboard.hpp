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
#include <string>

#if defined(ESP8266)
#include <ESP8266WebServer.h>

#elif defined(ESP32)
#include <WebServer.h>
#endif

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
  float energyConsumption1[WEB_SERVER_DATA_LENGTH]; // Energy consumption 1 kWh
  float energyConsumption2[WEB_SERVER_DATA_LENGTH]; // Energy consumption 2 kWh
  float energyProduction1[WEB_SERVER_DATA_LENGTH]; // Energy production 1 kWh
  float energyProduction2[WEB_SERVER_DATA_LENGTH]; // Energy production 1 kWh

public:
  Dashboard (): server(WEB_SERVER_PORT) {
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

  void begin () {
    this->server.on("/", std::bind(&Dashboard::handleRoot, this));               // Call the 'handleRoot' function when a client requests URI "/"
    this->server.on("/data", std::bind(&Dashboard::handleDataApi, this));        // Call the 'handleDataApi' function when a client requests URI "/data"
    this->server.onNotFound(std::bind(&Dashboard::handleNotFound, this));        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
    this->server.begin(); // Actually start the server
  }

  void loop () {
    this->server.handleClient(); // Listen for HTTP requests from clients
  }

  bool connected2SmartMeter () {
    return ( millis() < this->serverTimer + WEB_SERVER_SAMPLE_RATE * 2 );
  }

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
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      dataJson = dataJson + actualPowerConsumption[i] + ",";
    }
    dataJson = dataJson + actualPowerConsumption[this->dataPointer-1] + "],\"power_production\":[";
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      dataJson = dataJson + actualPowerProduction[i] + ",";
    }
    dataJson = dataJson + actualPowerProduction[this->dataPointer-1] + "],\"energy_consumption1\":[";
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      dataJson = dataJson + energyConsumption1[i] + ",";
    }
    dataJson = dataJson + energyConsumption1[this->dataPointer-1] + "],\"energy_consumption2\":[";
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      dataJson = dataJson + energyConsumption2[i] + ",";
    }
    dataJson = dataJson + energyConsumption2[this->dataPointer-1] + "],\"energy_production1\":[";
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      dataJson = dataJson + energyProduction1[i] + ",";
    }
    dataJson = dataJson + energyProduction1[this->dataPointer-1] + "],\"energy_production2\":[";
    for ( uint16_t i=0; i < this->dataPointer-1; i++ ) {
      dataJson = dataJson + energyProduction2[i] + ",";
    }
    dataJson = dataJson + energyProduction2[this->dataPointer-1] + "],\"DSMRVersion\":\"" + DSMRVersion +
              "\",\"DSMRTimestamp\":\"" + DSMRTimestamp +
              "\",\"connected\":" + (this->connected2SmartMeter() ? "1" : "0") + "}";

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
  void processP1(char* p1) {
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
