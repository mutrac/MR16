#!/usr/bin/env python
"""
MR16
MuTrac Release Candidate 2016
"""

# Dependencies
import HUD
import CAN
import sys, os, json, time

# Functions
def format_time():
    return datetime.strftime(datetime.now(), '[%Y-%m-%d %H:%M:%S.%f]')
    
def parse_args():
    for i in sys.argv:
        print i

# Class
class MR16:

    def __init__(self, hud_config, can_config):
        self.display = HUD.SafeMode(hud_config) 
        self.network = CAN.MIMO(can_config)
    
    def run(self):
        while True:
            new_labels = {
                'test' : time.time(),
            }
            self.display.update_labels(new_labels)
        
    def shutdown(self):
        pass
        
# Run-Time
if __name__ == '__main__':
    with open('hud_config.json', 'r') as jsonfile:
        hud_config = json.loads(jsonfile.read())
    with open('can_config.json', 'r') as jsonfile:
        can_config = json.loads(jsonfile.read())
    root = MR16(hud_config, can_config)
    root.run()
