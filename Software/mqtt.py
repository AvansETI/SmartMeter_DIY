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
from dataclasses import dataclass

import paho.mqtt.client as mqtt
from appconfig import AppConfig
from logger import logger
from datetime import datetime
from influxdb_client import InfluxDBClient, WriteApi, WriteOptions, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from dsmr import *

@dataclass
class Emon(object):
    def __init__(self,
            signature: str,
            power_produced: float,
            power_consumed: float,
            energy_produced: float,
            energy_consumed: float,
            timestamp: datetime,
            gas: float = 0.0):
        self.signature = signature
        self.power_produced = power_produced
        self.power_consumed = power_consumed
        self.energy_produced = energy_produced
        self.energy_consumed = energy_consumed
        self.timestamp = timestamp
        self.gas = gas

# Setup paho mqtt
class MQTT(object):

    def __init__(self, config: AppConfig):
        self.mqttClient = mqtt.Client("", clean_session=True)
        self.appConfig = config

        url = "http://" + config['influxdb']['host'] + ":" + str(config['influxdb']['port'])
        token = self.appConfig['influxdb']['token']
        org = self.appConfig["influxdb"]["org"]
        client = InfluxDBClient(url=url, org=org, token=token)
        self.write_api = client.write_api(write_options=SYNCHRONOUS)

    #
    # Mqtt events
    #
    def on_connect(self, mqttc, obj, flags, rc):
        #
        # Toevoegen subscribe
        # https://stackoverflow.com/questions/59783319/paho-mqtt-client-disconnected-and-no-more-message-incoming-after-reconnection
        #
        self.mqttClient.subscribe("smartmeter/raw", 0)
        if rc==0:
            logger.info(msg="MQTT Succesfully connect to broker")
        else:
            logger.info(msg="MQTT connected Error")

    def on_message(self, mqttc, obj, msg):
        try:
            json_payload = json.loads(msg.payload)

            #
            p1_raw = json_payload["datagram"]["p1"]
            # Parse P1 Message
            p1_decoded = DSMR_Parser(p1_raw).parse()

            # Construct dict
            emon = Emon(signature=json_payload["datagram"]["signature"],
                        power_consumed=p1_decoded["power"][0]["delivered"]["value"],
                        power_produced=p1_decoded["power"][1]["received"]["value"],
                        energy_consumed=p1_decoded["energy"][0]["delivered"]["value"]+p1_decoded["energy"][1]["delivered"]["value"],
                        energy_produced=p1_decoded["energy"][0]["received"]["value"]+p1_decoded["energy"][1]["received"]["value"],
                        timestamp= datetime.utcnow())

            # optional GAS
            if 'gas' in p1_decoded.keys():
                emon.gas = p1_decoded['gas']['delivered']['value']

            # Create influx record (line protocol)
            record = ("dsmr,signature={} "
                      "power_produced={:.4f},power_consumed={:.4f},"
                      "energy_produced={:.4f},energy_consumed={:.4f},"
                      "gas_delivered={:.4f}").format(
                    emon.signature,
                    emon.power_produced, emon.power_consumed,
                    emon.energy_produced, emon.energy_consumed,
                    emon.gas
                )

            logger.debug(msg = record)
            # Write to influx - modify credentials first in config.json
            #self.write_api.write(bucket = self.appConfig['influxdb']['bucket'], record=record, timestamp=emon.timestamp)

        except Exception as str:
            logger.error(msg= ("Emon.save() exception: {0}", str) )

    def on_publish(self, mqttc, obj, mid):
        pass

    def on_subscribe(self, client, userdat, mid, granted_qos):
        pass

    def on_log(self, mqttc, obj, level, msg):
        logger.debug(msg=msg)
        pass

    def on_disconnect(self, mqtt, obj, msg):
        logger.info(msg="MQTT disconnect from broker")

    def run(self):

        # Hook-up all callbacks
        self.mqttClient.on_disconnect = self.on_disconnect
        self.mqttClient.on_message = self.on_message
        self.mqttClient.on_connect = self.on_connect
        self.mqttClient.on_publish = self.on_publish
        self.mqttClient.on_subscribe = self.on_subscribe
        self.mqttClient.on_log = self.on_log

        # Setup mqtt and connect to broker
        self.mqttClient.username_pw_set(username=self.appConfig['mqtt']['username'], password=self.appConfig['mqtt']['password'])
        self.mqttClient.connect(self.appConfig['mqtt']['host'], self.appConfig['mqtt']['port'], 10)

        # Move the subscribe to OnConnect callback .. See
        # https://stackoverflow.com/questions/59783319/paho-mqtt-client-disconnected-and-no-more-message-incoming-after-reconnection
        #self.mqttClient.subscribe("smartmeter/raw", 0)

        self.mqttClient.loop_start()

    def isConnected(self):
        return self.mqttClient.is_connected()

    def stop(self):
        self.mqttClient.loop_stop()
        self.mqttClient.disconnect()
