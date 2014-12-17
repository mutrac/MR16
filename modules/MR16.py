#!/usr/bin/env python
"""
MR16
MuTrac Release Candidate 2016
"""

# Dependencies
import groundspeed
import hud
import can
import sys, os

# Functions
def format_time():
    return datetime.strftime(datetime.now(), '[%Y-%m-%d %H:%M:%S.%f]')
    
def parse_args():
    for i in sys.argv:
        print i

# Class
class MR16:
    def __init__(self):
        self.hud = hud.SafeMode() 
        self.can = can.MIMO()
        self.groundspeed = groundspeed.BruteMatch()
    
    def run(self):
        kph = self.groundspeed.get_kph()
        snapshot = self.can.get_snapshot()
        self.hud.update_display(snapshot)
        
    def shutdown(self):
        self.hud.close()
        self.can.close()
        self.groundspeed.close()
        
# Run-Time
if __name__ == '__main__':
    root = MR16()
    try:
        root = MR16()
        A = True
        while A:
            root.run()
    finally:
        root.shutdown()
