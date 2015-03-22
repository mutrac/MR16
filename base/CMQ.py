"""
Controller Message Query (CMQ)
This class is responsible for managing the controller network.

"""

# Dependencies
import pymongo
import serial
import ast
import zmq
from datetime import datetime
import json
import time
import thread

# Useful Functions 
def pretty_print(task, msg):
    date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
    print("%s %s %s" % (date, task, msg))

def save_config(config, filename):
    with open(filename, 'w') as jsonfile:
        jsonfile.write(json.dumps(config, indent=True))

"""
Device Class
This is a USB device which is part of a MIMO system
"""
class Controller:
    def __init__(self, name, baud, timeout, rules):
        self.name = name
        self.baud = baud
        self.timeout = timeout
        self.rules = rules
        """ 
        Rules are formatted as a 4 part list:
            1. Key
            2. Value
            3. UID
            4. Message
        """
        try:
            self.port = serial.Serial(name, baud, timeout)
        except:
            return None # return nothing if the device failed to start on USB
        
    def set_pid(self, pid):
        self.pid = pid
        
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
            for dev in config: 
                c = Controller(dev['name'], dev['baud'], dev['timeout'], dev['rules'])
                if c: self.add_controller(c)
        except Exception as e:
            pretty_print('MIMO ERR', str(e))
        pretty_print('MIMO INIT', 'Controller List : %s' % self.list_controllers())
        
    # Add new controller to the network
    # Checks to make sure PID is correct before inserting into controllers dict
    # TODO: handle errors
    def add_controller(self, dev, attempts=1):
        try:
            pretty_print('MIMO ADD', 'Trying to add %s @ %d' % (dev.name, dev.baud))
            for i in range(attempts):
                try:
                    string = dev.port.readline()
                    if string is not None:
                        try:    
                            data = ast.literal_eval(string)
                            uid = data['id'] #TODO Confirm if this is the right key
                        except Exception as e:
                            raise KeyError('Failed to find valid ID')
                        self.controllers[uid] = dev
                        self.generate_event('MIMO ADD', "%s is %s" % (dev.name, uid))
                except Exception as e:
                    pretty_print('MIMO ERR', str(e))
            raise ValueError('All attempts failed when adding: %s' % dev.name)
        except Exception as error:
            self.generate_event('CAN ERR', str(error))
            raise ValueError('MIMO ERR')
        
    # Remove controller from the network by UID
    def remove_controller(self, uid):
        try:
            port = self.controllers[uid]
            port.close_all()
            del self.controllers[uid]
        except Exception as error:
            self.generate_event('MIMO ERR', str(error))
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
    
    # Listen
    def listen(self, dev):
        pretty_print('listen()', dev.name)
        dump = dev.port.read(timeout=dev.timeout)
        event = ast.literal_eval(dump)
        for (key, val, uid, msg) in dev.rules:
            if (event['uid'] == uid) and (event[key] == val): # TODO: might have to handle Unicode
                try:
                    pretty_print('listen()', dev.name)
                    target = self.controllers[uid]
                    target.port.write(msg)
                except Exception as e:
                    pretty_print('listen() error', str(e))
        return event
        
    # Listen All  
    def listen_all(self):
        if self.controllers:
            try:
                events = [self.listen(c) for c in self.controllers]
                return events
            except Exception as error:
                return [self.generate_event("MIMO ERR", str(error))]
        else:
            return [self.generate_event("MIMO ERR", 'No controllers found in network!')]
    
    # List controllers
    def list_controllers(self):
        return self.controllers.keys()
    
"""
CMQ is a CAN/ZMQ Wondersystem

This class is responsible for relaying all serial activity. The CMQ client is a
single node in a wider network, this configuration of push-pull from client to
server allows for multiple CMQ instances to communicate with a central host
(e.g. the OBD) in more complex configurations.
"""    
class CMQ(object):

    # Initialize
    def __init__(self, object, addr="tcp://127.0.0.1:1980", timeout=0.1):
        self.system = object
        self.addr = addr
        self.timeout = timeout
        self.zmq_context = zmq.Context()
        self.zmq_client = self.zmq_context.socket(zmq.REQ)
        self.zmq_client.connect(self.addr)
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
                    socks = dict(self.zmq_poller.poll(self.timeout))
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
            self.zmq_client.connect(self.addr)
            return False #! TODO indicates no errors
        except Exception:
            pretty_print('RESET ERROR', 'failed on reset')
            return True #! TODO indicates errors on reset
