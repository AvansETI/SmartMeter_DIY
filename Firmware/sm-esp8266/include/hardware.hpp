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

  Within this library hardware functionality will be implemented.

  V1.0  Initial version
  -------------------------------------------------------------------------*/


typedef enum {
  RED = 0, GREEN, BLUE
} RGB_COLOR_ENUM;

typedef enum {
  ON = 0, OFF
} RGB_STATE_ENUM;

/******************************************************************/
void resetHardware () 
/* 
short: Performs a hardware reset of the chip.        
inputs:        
outputs: 
notes:         
Version: MS, Initial code
*******************************************************************/
{
#if defined(ESP8266)
  ESP.reset();
#elif defined(ESP32)
  esp_restart();
#endif
}

#if defined(ESP32)
/******************************************************************/
void getResetReason(char* s)
/* 
short: Get the reset reason of the chip.        
inputs: char pointer       
outputs: char pointer filled with reason
notes: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/reset_reason.html
Version: MS, Initial code
*******************************************************************/
{
  switch ( esp_reset_reason() ) {
    case 1:  sprintf(s, "POWERON_RESET"); break;          /**<1,  Vbat power on reset*/
    case 3:  sprintf(s, "SW_RESET"); break;               /**<3,  Software reset digital core*/
    case 4:  sprintf(s, "OWDT_RESET"); break;             /**<4,  Legacy watch dog reset digital core*/
    case 5:  sprintf(s, "DEEPSLEEP_RESET"); break;        /**<5,  Deep Sleep reset digital core*/
    case 6:  sprintf(s, "SDIO_RESET"); break;             /**<6,  Reset by SLC module, reset digital core*/
    case 7:  sprintf(s, "TG0WDT_SYS_RESET"); break;       /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8:  sprintf(s, "TG1WDT_SYS_RESET"); break;       /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9:  sprintf(s, "RTCWDT_SYS_RESET"); break;       /**<9,  RTC Watch dog Reset digital core*/
    case 10: sprintf(s, "INTRUSION_RESET"); break;        /**<10, Instrusion tested to reset CPU*/
    case 11: sprintf(s, "TGWDT_CPU_RESET"); break;        /**<11, Time Group reset CPU*/
    case 12: sprintf(s, "SW_CPU_RESET"); break;           /**<12, Software reset CPU*/
    case 13: sprintf(s, "RTCWDT_CPU_RESET"); break;       /**<13, RTC Watch dog Reset CPU*/
    case 14: sprintf(s, "EXT_CPU_RESET"); break;          /**<14, for APP CPU, reset by PRO CPU*/
    case 15: sprintf(s, "RTCWDT_BROWN_OUT_RESET"); break; /**<15, Reset when the vdd voltage is not stable*/
    default: sprintf(s, "NO_MEAN");
  }
}
#endif

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





