"""
Controller-Area Network
This class is responsible for managing the controller network
"""

# Dependencies
import pymongo
import serial
import thread
import ast
from datetime import datetime

# Useful Functions 
def pretty_print(task, msg):
    date = datetime.strftime(datetime.now(), "%Y-%m-%d %H:%M:%S.%f")
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
    
    def __init__(self, config):
        pretty_print('CAN', 'initializing')
        self.controllers = {}
        for name in config: 
            self.add_controller(config[name]['path'], config[name]['baud'])
        
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
        
    """
    Return a list of controllers and their status
    """
    def list_controllers(self):
        return self.controllers.values()
    
    def close(self):
        pretty_print('SYSTEM', 'Closing')

if __name__ == '__main__':
    config = {
        '1' : {
            'path' : '/dev/ttyACM0',
            'baud' : 9600
        }
    }
    can = MIMO(config)
