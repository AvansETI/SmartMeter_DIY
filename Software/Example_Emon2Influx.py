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