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


# Classes (Note: class names should be capitalized)

"""
This is the MIMO class (Multiple-Input, Multiple-Output) which supports
several controllers and custom network topologies. 
"""
class CAN:
    
    def __init__(self, config):
    
        ## Create the controller registry look-up dictionary
        self.controllers = {}
        for (dev_path, baud) in config:
            self.add_controller(dev_path, baud)
                
    """
    Parse message from controller
    """           
    def parse_msg(raw):
        msg = ast.literal_eval(raw)
        return msg
    
    """
    Get Controller info
    """           
    def controller_info(string):
        msg = ast.literal_eval(string)
        msg['id']
        return msg
        
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
    config = [('/dev/ttyACM0', 9600)]
    can = CAN(config)
