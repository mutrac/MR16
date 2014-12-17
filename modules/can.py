"""
Controller-Area Network
This class is responsible for managing the controller network
"""

# Dependencies
import pymongo
import serial
import thread
import ast

# Functions
def parse_msg(raw):
    msg = ast.literal_eval(raw)
    return msg

# Classes (Note: class names should be capitalized)
"""
This is the MIMO class (Multiple-Input, Multiple-Output) which supports
several controllers and custom network topologies. 
"""
class MIMO:
    
    def __init__(self, devices=None, patterns=None):
        ## Create the controller registry look-up dictionary
        self.controllers = {}
        if devices:
            for (dev_id, baud) in devices:
                self.add_controller(dev_id, baud)
        ## Create the network direction look-up dictionary
        self.network_map = {}
        if patterns:
            for (dev_id, targets) in patterns:
                pass
    
    def add_controller(self, dev_id, baud, target):
        try:
            port = self.controllers[dev_id]
            if port:
              raise
            else:
                port = serial.Serial(dev_id, baud)
                self.controllers[dev_id] = port
                thread.start_new_thread(self.listen, dev_id)
        except Exception as error:
            print str(error)
        
    def remove_controller(self, dev_id):
        try:
            port = self.controllers[dev_id]
            if not port:
                raise
            else:
                port.close_all()
                del self.controllers[dev_id]
        except Exception as error:
            print str(error)
        
    def listen(self, dev_id):
        while self.controllers[dev_id]:
            raw = self.controllers[dev_id].read()
            msg = parse_msg(raw)
            print msg
    
    def broadcast(self, dev_id, msg):
        while self.controllers[dev_id]:
            print msg
    
    def get_snapshot(self):
        return self.controllers
    
    def close(self):
        print "---"
