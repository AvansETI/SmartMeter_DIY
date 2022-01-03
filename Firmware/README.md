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

when you are finished, you can compile and upload the sketch. In the case, you like more functionality
you can create the software yourself. We appreciate contribution by pull requests! You can become
part of the project as well.

## PlatformIO

This software is created using the PlatformIO module of Visual Studio code. It is the same as the sketch above, but in another environment. It depends on what you prefer to use.