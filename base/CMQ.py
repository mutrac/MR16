"""
Controller Message Query (CMQ)

This class is responsible for managing the controller network.

NOTE: The CMQ system is built such that the OBD is not explicitly necessary for the
CMQ to handle inter-controller message passing, which is why the CMQ takes a 
rule-base as one of it's required configuration settings.
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
    print("[%s] %s %s" % (date, task, data))

def save_config(config, filename):
    with open(filename, 'w') as jsonfile:
        jsonfile.write(json.dumps(config, indent=True))

"""
Rule Class
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
    def __init__(self, uid, name, dev_num=None, baud=9600, timeout=1.0, rules=[], port_attempts=3, read_attempts=20, write_timeout=0.5, flushable_chars=256):
        self.name = name
        self.uid = uid # e.g. VDC
        self.baud = baud
        self.timeout = timeout
        self.rules = rules
        self.uid = uid
        
        ## Make several attempts to locate serial connection to self.port
	if not dev_num:
	        for i in range(port_attempts):
	            try:
	                self.name = name + str(i) # e.g. /dev/ttyACM1
	                pretty_print('CMQ', 'Attempting to attach %s on %s' % (self.uid, self.name))
	                self.port = serial.Serial(self.name, self.baud, timeout=self.timeout, writeTimeout=write_timeout)
	                time.sleep(timeout)
	                while True:
	      		    while (self.port.inWaiting() > flushable_chars):
	    			dump = self.port.readline()
	                    string = self.port.readline()
	                    if string is not (None or ''):
	                        try:
	                            data = ast.literal_eval(string)
	                            pretty_print('CMQ', 'Read OK, device is %s' % data['uid'])
	                            if data['uid'] == self.uid:
	                                pretty_print('CMQ', 'Found matching UID')
					self.dev_num = i
	                                return # return the Controller object
	                            else:
	                                break
	                        except Exception as error:
	                            pretty_print('CMQ', str(error))
	            except Exception as error:
	                pretty_print('CMQ', str(error))
	        else:
	            raise ValueError('All attempts failed when searching for %s' % self.uid)
	else:
            try:
                self.name = name + str(i) # e.g. /dev/ttyACM1
                pretty_print('CMQ', 'Attempting to attach %s on %s' % (self.uid, self.name))
                self.port = serial.Serial(self.name, self.baud, timeout=self.timeout, writeTimeout=write_timeout)
                time.sleep(2)
                for j in range(read_attempts):
                    time.sleep(timeout)
                    string = self.port.readline()
                    if string is not (None or ''):
                        try:
                            data = ast.literal_eval(string)
                            pretty_print('CMQ', 'Read OK, device is %s' % data['uid'])
                            if data['uid'] == self.uid:
                                pretty_print('CMQ', 'Found matching UID')
                                return # return the Controller object
                            else:
                                break
                        except Exception as error:
                            pretty_print('CMQ', str(error))
            except Exception as error:
                pretty_print('CMQ', str(error))
		    
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
    def add_controller(self, config, dev_num=None):
        try:
        
            # The device config should have each of these key-vals
            uid = config['uid']
            name = config['name']
            baud = config['baud']
            timeout = config['timeout']
            rules = config['rules']
            
            # Attempt to locate controller
            c = Controller(uid, name, baud=baud, timeout=timeout, rules=rules, dev_num=dev_num)
            pretty_print('CMQ', "Adding %s on %s" % (c.uid, c.name))
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
        pretty_print('CMQ', '%s (%s) -- Listening' % (dev.uid, dev.name))
        try:
            while (dev.port.inWaiting() > 128):
	    	dump = dev.port.readline()
	    event = ast.literal_eval(dump)
            if False: # not self.checksum(event):  #! TODO: check sum here?
                return self.generate_event('CMQ', 'error', '%s (%s) -- Checksum Failed' % (dev.uid, dev.name))
            pretty_print('CMQ', '%s (%s) -- OKAY' % (dev.uid, dev.name))
        except SyntaxError as e:
            return self.generate_event('CMQ', 'error', '%s (%s) -- Parse Failed' % (dev.uid, dev.name))
            pretty_print('CMQ', '%s (%s) -- Flushing input buffer' % (dev.uid, dev.name))
        except Exception as e:
            return self.generate_event('CMQ', 'error', '%s (%s) -- NO DATA' % (dev.uid, dev.name))
            
        ## Follow rule-base
        data = event['data']
        for r in dev.rules:
            try:
                target = r['target']
            except Exception as e:
		pretty_print('CMQ', 'ERROR: %s' % 'Rule does not have a target')
            	pass
            try:
                cmd = r['command']
            except Exception as e:
		pretty_print('CMQ', 'ERROR: %s' % 'Rule does not have a command')
		pass
            try:
                desc = r['description']
            except Exception as e:
                pretty_print('CMQ', 'ERROR: %s' % 'Rule does not have description')
		pass
            try:
                target_dev = self.controllers[target]
            except Exception as e:
		pretty_print('CMQ', 'ERROR: %s does not exist!' % target)
		pass
            for [key,val] in r['conditions']:
                if (data[key] == val): # TODO: might have to handle Unicode
                    try:
                        # target_dev.port.flushOutput()
                        pretty_print('CMQ', 'Writing %s command to %s ...' % (str(cmd), str(target)))
                        target_dev.port.write(str(cmd) + '\n')
                    except Exception as e:
                        pretty_print('CMQ', 'ERROR: Failed to follow rule -- %s' % desc)
        return event
        
    # Listen for data from all arduino controllers
    # Arguments: None
    # Returns: List of each successfully parsed event
    def listen_all(self):
	if self.controllers:
            events = []
            for c in self.controllers.values(): # get name of each controller in network
                try:
                    e = self.listen(c) # listen for event
                    events.append(e)
                except Exception as error:
                    events.append(self.generate_event("CMQ", 'error', str(error))) # create 'error' event
            return events
        else:
            return [self.generate_event("CMQ", 'error', 'Empty Network!')]
    
    # List all arduino controllers in the network
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
    
    # Compares the Check sum of an event from a controller to the proper value
    # Arguments: <Event>
    # Returns: True if it passes the checksum, False if otherwise
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
            pretty_print('CMQ', 'ERROR: %s' % str(e))
        
    # Run Indefinitely
    def run_async(self, frequency=10):
        while True:
	    a = time.time()
            events = self.listen_all()
            for e in events: # Read newest 
                try:
                    dump = json.dumps(e)
                    self.zmq_client.send(dump)
                    time.sleep(self.timeout)
                    socks = dict(self.zmq_poller.poll(self.timeout))
                    if socks:
                        if socks.get(self.zmq_client) == zmq.POLLIN:
                            dump = self.zmq_client.recv(zmq.NOBLOCK) # zmq.NOBLOCK
                            response = json.loads(dump)
                            pretty_print('CMQ', 'Received response from OBD')
                            #! TODO handle any fancy push/pull responses from the host
                            """
                            CURRENTLY THIS SECTION DOES NOTHING
                            IN THE FUTURE, THE OBD WILL BE ABLE TO ROUTE TESTING
                            AND OTHER DIAGNOSTIC COMMANDS TO THE CMQ
                            """
                        else:
                            pretty_print('CMQ', 'WARNING: Poller Timeout')
                    else:
                        pretty_print('CMQ', 'WARNING: Socket Timeout')
                except Exception as error:
                    pretty_print('CMQ', 'ERROR: %s' % str(error))
            b = time.time()
            while (1 / (float(b - a)) > frequency):
		time.sleep(0.01)
		b = time.time()
    # Reset server socket connection
    def reset(self):
        pretty_print('CMQ','Resetting CMQ connection to OBD')
        try:
            self.zmq_client = self.zmq_context.socket(zmq.REQ)
            self.zmq_client.connect(self.addr)
        except Exception:
            pretty_print('CMQ', 'ERROR: Failed to reset properly')

if __name__ == '__main__':
    with open('config/CMQ_v1.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load settings file
    cmq = CMQ(config) # Start the MQ client
    cmq.run_async()
