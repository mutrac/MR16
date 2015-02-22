"""
base file for the MR16 vehicle system
"""

from CAN.CAN import MIMO
from CAN.CAN import CMQ
from HUD.HUD import SafeMode
from OBD.OBD import WatchDog
import tests
import json

def test_CAN():
    with open('CAN/CAN.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load settings file 
    mimo = MIMO(config) # Start the serial network
    cmq = CMQ(mimo) # Start the MQ client
    try:
        cmq.run()
    except KeyboardInterrupt:
        pass
    except Exception as error:
        pretty_print('ERROR', str(error))

def test_HUD():
    with open('HUD/HUD.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load settings file
    display = SafeMode(config)
    while True:
        try:
            e = {'mcu': 'TCS', 'w_eng': 3600 } 
            display.update_labels(e)
        except KeyboardInterrupt:
            break
            
def test_OBD():
    with open('OBD/OBD.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load config file
    daemon = WatchDog(config) # start watchdog
    daemon.run()

if __name__ == '__main__':
    test_CAN()
