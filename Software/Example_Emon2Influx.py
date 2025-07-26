#
#  The MIT License (MIT)
#  Copyright © 2025 Avans Hogeschool Lectoraat Smart Energy
#  
#  Permission is hereby granted, free of charge, to any person obtaining a 
#  copy of this software and associated documentation files (the “Software”), 
#  to deal in the Software without restriction, including without limitation 
#  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
#  and/or sell copies of the Software, and to permit persons to whom the 
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in 
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
#  THE SOFTWARE.
#
# 
# Example how to use dsmr parser with the SeNDlab Influx database
# and MQTT broker
#
#

#!/usr/bin/python3
import time, signal
from logger import logger
from mqtt import MQTT

from appconfig import AppConfig

appconfig = AppConfig()
mqtt = MQTT(config=appconfig)

# Main entry point
def run_program():
    while True:
       if not mqtt.isConnected():
           try:
               mqtt.run()
           except:
                logger.info(msg="Cant't connect to Broker, retry in 30 seconds")
       time.sleep(30.0)
       pass

# Handle system interrupts e.g. ctrl-c
def keyboardInterruptHandler(signal, frame):
    logger.info(msg = "KeyboardInterrupt (ID: {}) has been caught. Cleaning up...".format(signal))
    mqtt.stop()
    exit(0)

if __name__ == '__main__':
    signal.signal(signal.SIGINT, keyboardInterruptHandler)
    run_program()