#!/usr/bin/env python
"""
MR16
MuTrac Release Candidate 2016
"""

# Dependencies
import HUD
import CAN
import OBD
import sys, os, json, time
import zmq
from datetime import datetime

# Class
class MR16:

        
    def __init__(self, hud_config, can_config, obd_config):
        self.display = HUD.SafeMode(hud_config) 
        self.network = CAN.MIMO(can_config)
        self.obd = OBD.WatchDog(obd_config)
        
    def run(self):
        while True:
            e = self.network.listen_all()
            if e:
                self.obd.add_log_entry(e)
                self.display.update_labels(e)
    
    def shutdown(self):
        pass
        
# Run-Time
if __name__ == '__main__':
    with open('hud_config.json', 'r') as jsonfile:
        hud_config = json.loads(jsonfile.read())
    with open('can_config.json', 'r') as jsonfile:
        can_config = json.loads(jsonfile.read())
    with open('obd_config.json', 'r') as jsonfile:
        obd_config = json.loads(jsonfile.read())
    root = MR16(hud_config, can_config, obd_config)
    root.run()
