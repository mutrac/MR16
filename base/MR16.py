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

def start_HUD():
    with open('config/HUD_debug.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load settings file
    display = SafeMode(config)
    while True:
        try:
            e = {'mcu': 'TCS', 'w_eng': 3600 } 
            display.update_labels(e)
        except KeyboardInterrupt:
            break
            
def start_CMQ():
    with open('config/CMQ_v1.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load settings file
        mimo = MIMO(config) # Start the serial network
        cmq = CMQ(mimo) # Start the MQ client

def start_OBD():   
    with open('config/OBD_v1.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load config file
    
    daemon = WatchDog(config) # start watchdog
    daemon.run()

def start_V6():
    with open('config/v6_v1.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load config file
    cam = V6(config) # start watchdog
    cam.speed()

if __name__ == '__main__':
    start_CMQ()
    start_OBD()
    start_HUD()
    start_V6()
