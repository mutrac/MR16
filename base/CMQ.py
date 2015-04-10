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
from itertools import cycle

# Useful Functions 
def pretty_print(task, data):
    date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
    print("%s %s %s" % (date, task, data))

def save_config(config, filename):
    with open(filename, 'w') as jsonfile:
        jsonfile.write(json.dumps(config, indent=True))

"""
Rule Class
CURRENTLY NOT IMPLEMENTED
"""
class Rule:
    def __init__(self, conditions, target, command, description=""):
        self.conditions = conditions
        self.target = target
        self.command = command
        self.description = description

"""
Controller Class
This is a USB device which is part of a MIMO system
"""
class Controller:
    def __init__(self, uid, name, baud=9600, timeout=0.001, rules=[], port_attempts=5, read_attempts=10):
        self.name = name
        self.uid = uid # e.g. VDC
        self.baud = baud
        self.timeout = timeout
        self.rules = rules
        self.uid = uid
        
        ## Make several attempts to locate serial connection to self.port
        for i in range(port_attempts):
            try:
                self.name = name + str(i) # e.g. /dev/ttyACM1
                pretty_print('CMQ', 'attempting to attach %s on %s' % (self.uid, self.name))
                self.port = serial.Serial(self.name, self.baud, timeout=self.timeout)
                time.sleep(1)
                for j in range(read_attempts):
                    string = self.port.readline()
                    if string is not None:
                        try:    
                            data = ast.literal_eval(string)
                            if data['uid'] == self.uid:
                                pretty_print('CMQ', 'found matching UID')
                                return
                        except Exception as error:
                            pretty_print('CMQ', str(error))
            except Exception as error:
                pretty_print('CMQ', str(error))
        else:
            raise ValueError('all attempts failed when searching for %s' % self.uid)
    
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
                self.add_controller(dev)
        except Exception as e:
            pretty_print('CMQ', str(e))
        pretty_print('CMQ', 'Controller List : %s' % self.list_controllers())
        
       
    # Add new controller to the network
    # Checks to make sure UID is correct before inserting into controllers dict
    def add_controller(self, config):
        try:
        
            # The device config should have each of these key-vals
            uid = config['uid']
            name = config['name']
            baud = config['baud']
            timeout = config['timeout']
            rules = config['rules']
            
            # Attempt to locate controller
            c = Controller(uid, name, baud=baud, timeout=timeout, rules=rules)
            pretty_print('CMQ', "adding %s on %s" % (c.uid, c.name))
            self.controllers[uid] = c #TODO Save the controller obj if successful
            
        except Exception as error:
            pretty_print('CMQ', str(error))
            
    # Remove controller from the network by UID
    def remove_controller(self, uid):
        try:
            port = self.controllers[uid]
            port.close_all()
            del self.controllers[uid]
        except Exception as error:
            pretty_print('CMQ', str(error))
            raise error

    # Listen for new event and check rules
    def listen(self, dev):
    
        ## Read and parse
        pretty_print('CMQ', 'Listening for %s' % dev.name)
        try:
            dump = dev.port.readline()
            event = ast.literal_eval(dump)
            if not self.checksum(event):  #! TODO: check sum here?
                return self.generate_event('CMQ', 'error', '%s failed checksum' % dev.name)
            else:
                event['task'] = 'control'
            pretty_print('CMQ', 'Received: %s' % str(event))
        except Exception as e:
            return self.generate_event('CMQ', 'error', 'No data from %s' % dev.name)
        
        ## Follow rule-base
        data = event['data']
        for r in dev.rules:
            try:
                target = r['target']
            except Exception as e:
                pretty_print('CMQ', 'Rule does not have a target')
            try:
                cmd = r['command']
            except Exception as e:
                pretty_print('CMQ', 'Rule does not have a command')
            try:
                desc = r['description']
            except Exception as e:
                pretty_print('CMQ', 'Rule does not have description')
            try:
                target_dev = self.controllers[target]
            except Exception as e:
                pretty_print('CMQ', 'Target does not exist: %s' % target)
            for [key,val] in r['conditions']:
                if (data[key] == val): # TODO: might have to handle Unicode
                    try:
                        pretty_print('CMQ', 'Routing %s command to %s' % (str(cmd), str(target)))
                        target_dev.port.write(msg)
                    except Exception as e:
                        pretty_print('CMQ', 'Failed to follow rule: %s' % desc)
        return event
        
    # Listen All 
    def listen_all(self):
        if self.controllers:
            events = []
            for c in self.controllers.values(): # get each controller in network
                try:
                    e = self.listen(c)
                    events.append(e)
                except Exception as error:
                    events.append(self.generate_event("CMQ", 'error', str(error)))
            return events
        else:
            return [self.generate_event("CMQ", 'error', 'Empty Network!')]
    
    # List controllers
    def list_controllers(self):
        return self.controllers.keys()
        
    # Generate event/error
    def generate_event(self, uid, task, data):
        pretty_print(uid, data)
        event = {
            'uid' : uid,
            'task' : task,
            'data' : data,
            'time': datetime.strftime(datetime.now(), "%H:%M:%S.%f"),
        }
        return event
    
    # Check sum
    # TODO something is wrong and the checksums are different. check arduino to
    # ensure that it is doing the proper sum
    def checksum(self, event):
        try:
            chksum = 0
            data = str(event['data'])
            data = "".join(data.split()) # remove whitespace
            for c in data:
                chksum += ord(c)
            if (chksum % 256) == event['chksum']:
                return True
            else:
                return False
        except Exception as e:
            pretty_print('CMQ', str(e))
        
    # Run Indefinitely
    def run(self):
        while True:
            events = self.listen_all()
            for e in events: # Read newest 
                try:
                    pretty_print('CMQ', 'Sent: %s' % str(e))
                    dump = json.dumps(e)
                    self.zmq_client.send(dump)
                    time.sleep(self.timeout)
                    socks = dict(self.zmq_poller.poll(self.timeout))
                    if socks:
                        if socks.get(self.zmq_client) == zmq.POLLIN:
                            dump = self.zmq_client.recv(zmq.NOBLOCK) # zmq.NOBLOCK
                            response = json.loads(dump)
                            pretty_print('CMQ', 'Received: %s' % str(response))
                            #! TODO handle response from the host
                            if response['task'] == 'handler':
                                pass
                                """
                                This is where you could put cool stuff like
                                updating controllers, etc.
                                """
                            if response['task'] == 'status':
                                pass
                                """
                                This is where you could handle different status
                                related tasks.
                                """
                        else:
                            pretty_print('CMQ', 'Poller Timeout')
                    else:
                        pretty_print('CMQ', 'Socket Timeout')
                except Exception as error:
                    pretty_print('CMQ', str(error))
    
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
