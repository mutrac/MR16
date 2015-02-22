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
            for name in self.config['CONTROLLERS']: 
                dev = self.config['CONTROLLERS'][name]
                self.add_controller(dev['path'], dev['baud'])
        except Exception as error:
            pretty_print('CAN ERR1', str(error))
        pretty_print('CAN INIT', 'Controller List : %s' % self.list_controllers())
        
    # Add new controller to the network
    # TODO: handle errors
    def add_controller(self, dev_path, baud, attempts=5):
        try:
            pretty_print('CAN ADD', 'Trying to add %s @ %d' % (dev_path, baud))
            for i in range(attempts):
                try:
                    port = serial.Serial(dev_path, baud)
                    string = port.readline()
                    if string is not None:
                        try:    
                            data = ast.literal_eval(string)
                            dev_id = data['id']
                        except Exception as e:
                            raise KeyError('Failed to find valid ID')
                        self.controllers[dev_id] = port  #! make into own class?
                        self.generate_event('CAN ADD', dev_id)
                except Exception as e:
                    pretty_print('CAN ERR', str(e))
            raise ValueError('All attempts failed')
        except Exception as error:
            self.generate_event('CAN ERR', str(error))
            raise ValueError('CAN ERR')
        
    # Remove controller from the network
    def remove_controller(self, dev_id):
        try:
            port = self.controllers[dev_id]
            port.close_all()
            del self.controllers[dev_id]
        except Exception as error:
            self.generate_event('CAN ERR', str(error))
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
                    tout = self.config['CONTROLLERS'][c]['timeout']
                    dump = c.read(timeout=tout)
                    e = ast.literal_eval(dump) #! TODO Check for malformed data
                    events.append(e)
                return events
            except Exception as error:
                return [self.generate_event("CAN ERR", str(error))]
        else:
            return [self.generate_event("CAN ERR", 'No controllers found in network!')]
    
    # List controllers
    def list_controllers(self):
        return self.controllers.keys()
    
"""
CMQ is a CAN/ZMQ Wondersystem
This class is responsible for relaying all serial activity
"""    
class CMQ(object):

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
                            self.system.generate_event('CMQ RECV', str(e))
                            response = json.loads(dump)
                            self.system.generate_event('CMQ SEND', str(response))
                        else:
                            self.system.generate_event('CMQ ERR', 'Poller Timeout')
                    else:
                        self.system.generate_event('CMQ ERR', 'Socket Timeout')
                except Exception as error:
                    self.system.generate_event('CMQ ERR', str(error))
                    reset_trig = True
            if reset_trig:
                reset_trig = self.reset()
    
    # Reset server socket connection
    def reset(self):
        pretty_print('RESET','')
        try:
            self.zmq_client = self.zmq_context.socket(zmq.REQ)
            self.zmq_client.connect(self.system.config['ZMQ_SERVER'])
            return False #! TODO indicates no errors
        except Exception:
            pretty_print('RESET ERROR', 'failed on reset')
            return True #! TODO indicates errors on reset
