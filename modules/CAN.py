"""
Controller-Area Network
This class is responsible for managing the controller network
"""

# Dependencies
import pymongo
import serial
import ast
import zmq
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
several USB serial connections and some custom network topologies via ZMQ. 
"""
class MIMO:
    
    # Initialize
    def __init__(self, config):
        self.config = config
        self.controllers = {}
        try:
            for name in config['CONTROLLERS']: 
                dev = self.config[name]
                self.add_controller(dev['path'], dev['baud'])
        except Exception as error:
            pretty_print('INIT ERROR', str(error))
        pretty_print('INIT CAN', 'Controller List : %s' % self.list_controllers())
        
    # Add new controller to the network
    # TODO: handle errors
    def add_controller(self, dev_path, baud):
        try:
            port = serial.Serial(dev_path, baud)
            string = port.read()
            data = ast.literal_eval(string)
            dev_id = data['id']
            self.controllers[dev_id] = port  #! make into own class?
            self.generate_event('CAN_ADD', dev_id)
        except Exception as error:
            self.generate_event('ADD_ERR', str(error))
            raise error
        
    # Remove controller from the network
    def remove_controller(self, dev_id):
        try:
            port = self.controllers[dev_id]
            port.close_all()
            del self.controllers[dev_id]
        except Exception as error:
            self.generate_event('RMV_ERR', str(error))
            raise error
    
    # Generate event/error
    def generate_event(self, task, msg):
        pretty_print(task, msg)
        event = {
            'type' : task,
            'msg' : msg,
            'time': datetime.strftime(datetime.now(), "%H:%M:%S.%f"),
        }
        return event
    
    # Listen All  
    def listen_all(self):
        if self.controllers:
            try:
                events = []
                for c in self.controllers:
                    print c
                    tout = self.config['controllers']['timeout']
                    dump = c.read(timeout=tout)
                    e = ast.literal_eval(dump) #! TODO Check for malformed data
                    print e
                    events.append(e)
                return events
            except Exception as error:
                return [self.generate_event("CAN ERR_1", str(error))]
        else:
            return [self.generate_event("CAN_ERR_0", 'no controllers in network!')]
    
    # List controllers
    def list_controllers(self):
        return self.controllers.keys()
    
"""
CAN/ZMQ Wondersystem
"""    
class CAN(object):

    # Initialize
    def __init__(self, object):
        self.system = object
        self.zmq_context = zmq.Context()
        self.zmq_client = self.zmq_context.socket(zmq.REQ)
        self.zmq_client.connect(self.system.config['ZMQ_SERVER']) #! TODO: this class could have it's own inputs
        self.zmq_poller = zmq.Poller()
        self.zmq_poller.register(self.zmq_client, zmq.POLLIN)
        
    # Run until a reset fails
    def run(self):
        reset_trig = False
        while not reset_trig:
            for e in self.system.listen_all(): # Read newest 
                try:
                    dump = json.dumps(e)
                    self.zmq_client.send(dump)
                    socks = dict(self.zmq_poller.poll(self.system.config['ZMQ_TIMEOUT']))
                    if socks:
                        if socks.get(self.zmq_client) == zmq.POLLIN:
                            dump = self.zmq_client.recv(zmq.NOBLOCK)
                            self.system.generate_event('ZMQ_MSG_OUT', str(e))
                            response = json.loads(dump)
                            self.system.generate_event('ZMQ_MSG_IN', str(response))
                        else:
                            self.system.generate_event('ZMQ_ERR_3', 'Poller Timeout')
                    else:
                        self.system.generate_event('ZMQ_ERR_1', 'Socket Timeout')
                except Exception as error:
                    self.system.generate_event('ZMQ_ERR_0', str(error))
                    reset_trig = True
            if reset_trig:
                reset_trig = self.reset_zmq()
    
    # Reset ZMQ connection
    def reset_zmq(self):
        pretty_print('RESET','')
        try:
            self.zmq_client = self.zmq_context.socket(zmq.REQ)
            self.zmq_client.connect(self.system.config['ZMQ_SERVER'])
            return False #! TODO indicates no errors
        except Exception:
            pretty_print('RESET ERROR', 'failed on reset')
            return True #! TODO indicates errors on reset
    
if __name__ == '__main__':
    
    ## Load settings file --> needed for proper initialization
    with open('can_config.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read())
        
    ## Start the serial network
    mimo = MIMO(config)
    can = CAN(mimo)
    
    ## Run the CAN as a listen_all() loop
    try:
        can.run()
    except KeyboardInterrupt:
        pass
    except Exception as error:
        pretty_print('ERROR', str(error))
