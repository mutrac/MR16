"""
Main file for the MR16 vehicle system
* On-board diagnostics (OBD)
* Controller Message Query (CMQ)
* Heads-up Display (HUD)
"""

__author__ = "Trevor Stanhope"
__version__ = 0.1

from CMQ import MIMO, CMQ
from HUD import SafeMode
from OBD import WatchDog
from V6 import V6
import json

def test_CMQ():
    mimo = MIMO() # Start the serial network
    cmq = CMQ(mimo) # Start the MQ client
    try:
        cmq.run()
    except KeyboardInterrupt:
        pass
    except Exception as error:
        pretty_print('ERROR', str(error))

def test_HUD():
    with open('config/HUD_debug.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load settings file
    display = SafeMode(config)
    while True:
        try:
            e = {'mcu': 'TCS', 'w_eng': 3600 } 
            display.update_labels(e)
        except KeyboardInterrupt:
            break
            
def test_OBD():
    with open('config/OBD_v1.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load config file
    daemon = WatchDog(config) # start watchdog
    daemon.run()

def test_V6():
    with open('config/v6_v1.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load config file
    cam = V6(config) # start watchdog
    cam.speed()

if __name__ == '__main__':
    test_OBD() # OBD needs to run before CMQ to receive notifications
    test_CMQ()
    #test_HUD()
    #test_V6()
