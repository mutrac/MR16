"""
Controller-Area Network
This class is responsible for managing the controller network
"""

# Dependencies
import pymongo
import serial
import ast
from datetime import datetime
import json
import time

# Useful Functions 
def pretty_print(task, msg):
    date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
    print("%s %s %s" % (date, task, msg))

def save_config(config, filename):
    with open(filename, 'w') as jsonfile:
        jsonfile.write(json.dumps(config, indent=True))
        
# Classes (Note: class names should be capitalized)

"""
This is the MIMO class (Multiple-Input, Multiple-Output) which supports
several controllers and custom network topologies. 
"""
class MIMO:
    
    """
    Initialize
    """
    def __init__(self, config):
        self.config = config
        self.controllers = {}
        for name in config: 
            dev = self.config[name]
            try:
                self.add_controller(dev['path'], dev['baud'])
            except Exception as error:
                pretty_print('ERROR', str(error))
        pretty_print('CAN', self.controllers.keys())
        
    """
    Add new controller to the network
    TODO: handle errors
    """
    def add_controller(self, dev_path, baud):
        try:
            port = serial.Serial(dev_path, baud)
            string = port.read()
            data = ast.literal_eval(string)
            dev_id = data['id']
            self.controllers[dev_id] = port  #! make into own class?
            pretty_print('CAN', dev_id)
        except Exception as error:
            pretty_print('ERROR', str(error))
            raise error
        
    """
    Remove controller from the network
    """
    def remove_controller(self, dev_id):
        try:
            port = self.controllers[dev_id]
            port.close_all()
            del self.controllers[dev_id]
        except Exception as error:
            pretty_print('ERROR', str(error))
            raise error
    
    """
    Generate random event
    """
    def generate_event(self, task, msg):
        event = {
            'type' : task,
            'msg' : msg,
            'time': datetime.strftime(datetime.now(), "%Y-%m-%d %H-%M-%S.%f"),
            'w_eng' : time.time()
        }
        return event
        
    """
    Listen All
    TODO: refactor to make this an async monitor of each controller
    """   
    def listen_all(self):
        if self.controllers:
            for c in self.controllers:
                try:
                    pretty_print('CAN', 'listening on %s' % c)
                    e = c.read(timeout=1)
                    return e
                except Exception as error:
                    pretty_print("ERROR", str(error))
        else:
            pretty_print("ERROR", 'No controllers in network!')
            return self.generate_event("ERROR", 'No controllers in network!')  

if __name__ == '__main__':
    with open('can_config.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read())
    can = MIMO(config)
    while True:
        try:
            can.listen_all()
        except KeyboardInterrupt:
            break
        except Exception as error:
            pretty_print('ERROR', str(error))
