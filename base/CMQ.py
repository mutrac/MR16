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
def pretty_print(task, data):
    date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
    print("%s %s %s" % (date, task, data))

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
            raise ValueError('%s failed to initialize' % name) # return nothing if the device failed to start on USB
    
"""
CMQ is a CAN/ZMQ Wondersystem

This class is responsible for relaying all serial activity. The CMQ client is a
single node in a wider network, this configuration of push-pull from client to
server allows for multiple CMQ instances to communicate with a central host
(e.g. the OBD) in more complex configurations.

A systems network object (e.g. a SISO, MIMO, etc.) is passed to the CMQ, along with the
connection information for the ZMQ host.
"""    
class CMQ:

    # Initialize
    def __init__(self, config, addr="tcp://127.0.0.1:1980", timeout=0.1):
        self.addr = addr
        self.timeout = timeout
        self.zmq_context = zmq.Context()
        self.zmq_client = self.zmq_context.socket(zmq.REQ)
        self.zmq_client.connect(self.addr)
        self.zmq_poller = zmq.Poller()
        self.zmq_poller.register(self.zmq_client, zmq.POLLIN)
        self.config = config
        self.controllers = {}
        try:
            for dev in config: 
                c = Controller(dev['name'], dev['baud'], dev['timeout'], dev['rules'])
                if c: self.add_controller(c)
        except Exception as e:
            pretty_print('CMQ', str(e))
        pretty_print('CMQ', 'Controller List : %s' % self.list_controllers())
        
    # Add new controller to the network
    # Checks to make sure PID is correct before inserting into controllers dict
    # TODO: handle errors
    def add_controller(self, dev, attempts=1):
        try:
            pretty_print('CMQ', 'Trying to add %s @ %d' % (dev.name, dev.baud))
            for i in range(attempts):
                pretty_print('CMQ', '%s attempt #%d' % (dev.name, i+1))
                try:
                    string = dev.port.readline()
                    if string is not None:
                        try:    
                            data = ast.literal_eval(string)
                            uid = data['id'] #TODO Confirm if this is the right key
                        except Exception as e:
                            raise KeyError('Failed to find valid ID')
                        self.controllers[uid] = dev
                        self.generate_event('CMQ', "Adding %s is %s" % (dev.name, uid))
                except Exception as e:
                    pretty_print('CMQ', 'Error: %s' % str(e))
            raise ValueError('All attempts failed when adding: %s' % dev.name)
        except Exception as error:
            self.generate_event('CMQ', str(error))
        
    # Remove controller from the network by UID
    def remove_controller(self, uid):
        try:
            port = self.controllers[uid]
            port.close_all()
            del self.controllers[uid]
        except Exception as error:
            self.generate_event('CMQ', str(error))
            raise error
    
    # Listen
    def listen(self, dev):
        pretty_print('CMQ', 'Listening for %s' % dev.name)
        dump = dev.port.read(timeout=dev.timeout)
        event = ast.literal_eval(dump)
        for (key, val, uid, msg) in dev.rules:
            if (event['uid'] == uid) and (event[key] == val): # TODO: might have to handle Unicode
                try:
                    pretty_print('CMQ', 'Routing to controller %s:%s' % (str(key), str(val)))
                    target = self.controllers[uid]
                    target.port.write(msg)
                except Exception as e:
                    pretty_print('CMQ', str(e))
        return event
        
    # Listen All  
    def listen_all(self):
        if self.controllers:
            try:
                events = [self.listen(c) for c in self.controllers]
                return events
            except Exception as error:
                return [self.generate_event("CMQ", str(error))]
        else:
            return [self.generate_event("CMQ", 'Empty Network')]
    
    # List controllers
    def list_controllers(self):
        return self.controllers.keys()
        
    # Generate event/error
    def generate_event(self, task, data):
        pretty_print(task, data)
        event = {
            'type' : task,
            'data' : data,
            'time': datetime.strftime(datetime.now(), "%H:%M:%S.%f"),
        }
        return event
        
    # Run until a reset fails
    def run(self):
        while True:
            pretty_print('CMQ', 'Listening on all')
            events = self.listen_all()
            for e in events: # Read newest 
                try:
                    self.generate_event('CMQ', 'send: %s' % str(e))
                    dump = json.dumps(e)
                    self.zmq_client.send(dump)
                    time.sleep(self.timeout)
                    socks = dict(self.zmq_poller.poll(self.timeout))
                    if socks:
                        if socks.get(self.zmq_client) == zmq.POLLIN:
                            dump = self.zmq_client.recv(zmq.NOBLOCK) # zmq.NOBLOCK
                            response = json.loads(dump)
                            event = self.generate_event('CMQ', 'recv: %s' % str(response))
                        else:
                            event = self.generate_event('CMQ', 'Poller Timeout')
                    else:
                        event = self.generate_event('CMQ', 'Socket Timeout')
                except Exception as error:
                    event = self.generate_event('CMQ', str(error))
    
    # Reset server socket connection
    def reset(self):
        pretty_print('CMQ RST','Resetting CMQ connection to OBD')
        try:
            self.zmq_client = self.zmq_context.socket(zmq.REQ)
            self.zmq_client.connect(self.addr)
        except Exception:
            pretty_print('CMQ ERR', 'Failed to reset properly')

if __name__ == '__main__':
    with open('config/CMQ_v1.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load settings file
    cmq = CMQ(config) # Start the MQ client
    cmq.run()
