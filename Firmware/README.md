# SmartMeter Software

You can find here the firmware for the SmartMeter project. It uses a Wemos D1 mini lite
(https://www.wemos.cc/en/latest/d1/d1_mini_lite.html). Note that only the last version is
available that is used by the hardware version 2 and 3.

## ArduinoSketch / sm-esp8266

This software is created using the Arduino IDE (https://www.arduino.cc/en/software). After
installing the IDE you need to add the ESP8266 boards. You can find a tutorial here: 
https://siytek.com/wemos-d1-mini-arduino-wifi/. Furthermore, you need to install some 
libraries:
- Install library WiFiManager by tablatronics: https://github.com/tzapu/WiFiManager
- Install library JsonArduino by Banoit Blanchon: https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
- Install library pubsubclient by Nick O'Leary: https://github.com/knolleary/pubsubclient

when you are finished, you can compile and upload the sketch. In the case, you like more functionality
you can create the software yourself. We appreciate contribution by pull requests! You can become
part of the project as well.

This project is last updated in january 2022

## PlatformIO

Some developers like to use platformIO, so therefore this is also added to the repository. Note that the
ArduinoSketch is the main firmware that contains always the latest changes.


