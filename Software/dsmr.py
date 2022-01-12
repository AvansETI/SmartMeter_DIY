from __future__ import annotations
from abc import ABC, abstractmethod
import re
import json

class DSMR_Parser(object):
    """
    P1 datagram parser for Dutch Smart Meter Readings
    """
    def __init__(self, datagram) -> None:
        self._strategy = DSMR_UNKNOWN()
        self._datagram = datagram
        if re.search(r'1-3:0\.2\.8\(42\)', self._datagram) != None:
            self._strategy = DSMR_40()
        elif re.search(r'1-3:0\.2\.8\(50\)', self._datagram) != None:
            self._strategy = DSMR_50()
        else:
            self._strategy = DSMR_V2()

    @property
    def strategy(self) -> Strategy:
        return self._strategy

    # @property
    # def datagram(self) :
    #     return self._datagram

    @strategy.setter
    def strategy(self, strategy: Strategy) -> None:
        self._strategy = strategy

    # @datagram.setter
    # def datagram(self, datagram):
    #     self._datagram = datagram

    def parse(self) -> ():
        return self._strategy.parse(self._datagram)

class Strategy(ABC):
    @abstractmethod
    def parse(self, datagram):
        pass

class DSMR_UNKNOWN(Strategy):
    def parse(self, datagram):
        return {}

class DSMR_40(Strategy):
    def parse(self, datagram):

        # info
        version = re.search(r'1-3:0\.2\.8\(([0-9]+)\)', datagram).group(1)

        # Manufacturer
        manufacturer = re.search(r'([A-Z]{3})[0-9]{1}([a-zA-Z0-9]+)', datagram).group()

        # Power DELIVERED by client
        power_delivered = re.search(r'1-0:1\.7\.0\(([0-9]*\.[0-9]*)\*(kW)\)', datagram).group(1,2)

        # Power RECEIVED by client
        power_received = re.search(r'1-0:2\.7\.0\(([0-9]*\.[0-9]*)\*(kW)\)', datagram).group(1,2)

        # Energy DELIVERED tariff 1 TO client
        energy_to_t1 = re.search(r'1-0:1\.8\.1\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1,2)
        energy_to_t2 = re.search(r'1-0:1\.8\.2\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1,2)

        # Energy DELIVERED tariff 2 BY client
        energy_by_t1 = re.search(r'1-0:2\.8\.1\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1,2)
        energy_by_t2 = re.search(r'1-0:2\.8\.2\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1,2)

        # Current TARIFF
        tariff = re.search(r'0-0:96\.14\.0\(([0-9]*)\)', datagram).group(1)

        # Equipment ID
        equipment_id = re.search(r'0-0:96\.1\.1(\(([0-9]{1,96})\))', datagram).group(2)

        # Bundle all in dictionary
        retval = {
            'manufacturer': manufacturer,
            'version' : version,
            'equipment_id': equipment_id,
            'tariff': int(tariff),
            'power' : [
                {'delivered' : {'value': float(power_delivered[0]), 'unit': power_delivered[1] }},
                {'received': {'value': float(power_received[0]), 'unit': power_received[1] }}
            ],
            'energy': [
                {'tariff' : 1,
                    'delivered': {'value': float(energy_by_t1[0]), 'unit': energy_by_t1[1]},
                    'received': {'value': float(energy_to_t1[0]), 'unit': energy_to_t1[1]}
                },
                {'tariff' : 2,
                    'delivered': {'value': float(energy_by_t2[0]), 'unit': energy_by_t2[1]},
                    'received': {'value': float(energy_to_t2[0]), 'unit': energy_to_t2[1]}
                },
            ]
        }

        # Optionals
        # Gas DELIVERED TO client (from clients view: consumed)
        gas_delivered = re.search(r'0-1:24\.2\.1\((0*[0-w]*)\)\(*(0*[0-9]*\.[0-9]*)\*(m3)\)', datagram)
        if gas_delivered is not None:
            gas_delivered = gas_delivered.group(1, 2, 3)
            gas_dict = {
                        "delivered": {'value': float(gas_delivered[1]), 'unit': gas_delivered[2]},
                        "received": {'value': 0.0, 'unit': 'm3'}
            }
            retval['gas'] = gas_dict

        return retval


class DSMR_50(Strategy):
    def parse(self, datagram):

        # info
        version = re.search(r'1-3:0\.2\.8\(([0-9]+)\)', datagram).group(1)

        # Manufacturer
        manufacturer = re.search(r'([ /].*)', datagram).group()

        # Power DELIVERED by client
        power_delivered = re.search(r'1-0:1\.7\.0\(([0-9]*\.[0-9]*)\*(kW)\)', datagram).group(1,2)

        # Power RECEIVED by client
        power_received = re.search(r'1-0:2\.7\.0\(([0-9]*\.[0-9]*)\*(kW)\)', datagram).group(1,2)

        # Energy DELIVERED tariff 1 TO client
        energy_to_t1 = re.search(r'1-0:1\.8\.1\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1,2)
        energy_to_t2 = re.search(r'1-0:1\.8\.2\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1,2)

        # Energy DELIVERED tariff 2 BY client
        energy_by_t1 = re.search(r'1-0:2\.8\.1\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1,2)
        energy_by_t2 = re.search(r'1-0:2\.8\.2\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1,2)

        # Current TARIFF
        tariff = re.search(r'0-0:96\.14\.0\(([0-9]*)\)', datagram).group(1)

        # Equipment ID
        equipment_id = re.search(r'0-0:96\.1\.1(\(([0-9]{1,96})\))', datagram).group(2)

        # Bundle all in mandatory dictionary
        retval = {
            'manufacturer': manufacturer,
            'version': version,
            'equipment_id': equipment_id,
            'tariff': int(tariff),
            'power':  [
                {'delivered': {'value': float(power_delivered[0]), 'unit': power_delivered[1]}},
                {'received': {'value': float(power_received[0]), 'unit': power_received[1]}}
            ],
            'energy': [
                {'tariff': 1,
                 'delivered': {'value': float(energy_by_t1[0]), 'unit': energy_by_t1[1]},
                 'received': {'value': float(energy_to_t1[0]), 'unit': energy_to_t1[1]}
                 },
                {'tariff': 2,
                 'delivered': {'value': float(energy_by_t2[0]), 'unit': energy_by_t2[1]},
                 'received': {'value': float(energy_to_t2[0]), 'unit': energy_to_t2[1]}
                },
            ]
        }

        # Optionals
        # Gas DELIVERED TO client (from clients view: consumed)
        gas_delivered = re.search(r'0-1:24\.2\.1\((0*[0-w]*)\)\(*(0*[0-9]*\.[0-9]*)\*(m3)\)', datagram)
        if gas_delivered is not None:
            gas_delivered = gas_delivered.group(1, 2, 3)
            gas_dict = {
                        "delivered": {'value': float(gas_delivered[1]), 'unit': gas_delivered[2]},
                        "received": {'value': 0.0, 'unit': 'm3'}
            }
            retval['gas'] = gas_dict

        return retval


class DSMR_V2(Strategy):
    def parse(self, datagram):

        # Manufacturer
        manufacturer = re.search(r'([ /].*)', datagram).group()

        # Power DELIVERED by client
        power_delivered = re.search(r'1-0:1\.7\.0\(([0-9]*\.[0-9]*)\*(kW)\)', datagram).group(1, 2)

        # Power RECEIVED by client
        power_received = re.search(r'1-0:2\.7\.0\(([0-9]*\.[0-9]*)\*(kW)\)', datagram).group(1, 2)

        # Energy DELIVERED tariff 1 TO client
        energy_to_t1 = re.search(r'1-0:1\.8\.1\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1, 2)
        energy_to_t2 = re.search(r'1-0:1\.8\.2\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1, 2)

        # Energy DELIVERED tariff 2 BY client
        energy_by_t1 = re.search(r'1-0:2\.8\.1\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1, 2)
        energy_by_t2 = re.search(r'1-0:2\.8\.2\(0*([0-9]*\.[0-9]*)\*(kWh)\)', datagram).group(1, 2)

        # Current TARIFF
        tariff = re.search(r'0-0:96\.14\.0\(([0-9]*)\)', datagram).group(1)

        # Equipment ID
        equipment_id = re.search(r'0-0:96\.1\.1(\(([0-9A-F]{1,96})\))', datagram).group(2)

        # Bundle all in dictionary
        return {
            'manufacturer': manufacturer,
            'version': 'V2',
            'equipment_id': 'equipment_id',
            'tariff': int(tariff),
            'power': [
                {'delivered': {'value': float(power_delivered[0]), 'unit': power_delivered[1]}},
                {'received': {'value': float(power_received[0]), 'unit': power_received[1]}}
            ],
            'energy': [
                {'tariff': 1,
                 'delivered': {'value': float(energy_by_t1[0]), 'unit': energy_by_t1[1]},
                 'received': {'value': float(energy_to_t1[0]), 'unit': energy_to_t1[1]}
                 },
                {'tariff': 2,
                 'delivered': {'value': float(energy_by_t2[0]), 'unit': energy_by_t2[1]},
                 'received': {'value': float(energy_to_t2[0]), 'unit': energy_to_t2[1]}
                 },
            ]
        }

# if __name__ == '__main__':
#     datagram = "/KFM5KAIFA-METER\r\n\r\n1-3:0.2.8(42)\r\n0-0:1.0.0(200213170457W)\r\n0-0:96.1.1(4530303236303030303333333338343136)\r\n1-0:1.8.1(015001.164*kWh)\r\n1-0:1.8.2(012236.435*kWh)\r\n1-0:2.8.1(000942.859*kWh)\r\n1-0:2.8.2(002395.253*kWh)\r\n0-0:96.14.0(0002)\r\n1-0:1.7.0(00.299*kW)\r\n1-0:2.7.0(00.000*kW)\r\n0-0:96.7.21(00001)\r\n0-0:96.7.9(00001)\r\n1-0:99.97.0(2)(0-0:96.7.19)(180712201124S)(0000004179*s)(000101000006W)(2147483647*s)\r\n1-0:32.32.0(00000)\r\n1-0:52.32.0(00000)\r\n1-0:72.32.0(00000)\r\n1-0:32.36.0(00000)\r\n1-0:52.36.0(00000)\r\n1-0:72.36.0(00000)\r\n0-0:96.13.1()\r\n0-0:96.13.0()\r\n1-0:31.7.0(000*A)\r\n1-0:51.7.0(000*A)\r\n1-0:71.7.0(001*A)\r\n1-0:21.7.0(00.101*kW)\r\n1-0:41.7.0(00.038*kW)\r\n1-0:61.7.0(00.158*kW)\r\n1-0:22.7.0(00.000*kW)\r\n1-0:42.7.0(00.000*kW)\r\n1-0:62.7.0(00.000*kW)\r\n0-1:24.1.0(003)\r\n0-1:96.1.0(4730303332353631323831363736343136)\r\n0-1:24.2.1(200213170000W)(06136.485*m3)\r\n!2236\r\n"
#     for x in range(1):
#         parsed = DSMR_Parser(datagram).parse()
#         jsonStr = json.dumps( parsed)
#         print(jsonStr)


