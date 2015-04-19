"""
Vehicle HUD
This class is responsible for the user interface
"""

__author__ = 'Trevor Stanhope'
__version__ = 0.1

# Dependencies
import Tkinter as tk
from datetime import datetime 
import zmq
import time
import json
import random
import numpy as np

# Useful Functions 
def pretty_print(task, msg):
    date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
    print("%s %s %s" % (date, task, msg))
    
# Classes (Note: class names should be capitalized)
class SafeMode: 
        
    def __init__(self, config, addr="tcp://127.0.0.1:1980", timeout=0.1):
        pretty_print('HUD', 'Setting Layout')
        self.addr = addr
        self.timeout = timeout
        self.zmq_context = zmq.Context()
        self.zmq_client = self.zmq_context.socket(zmq.REQ)
        self.zmq_client.connect(self.addr)
        self.zmq_poller = zmq.Poller()
        self.zmq_poller.register(self.zmq_client, zmq.POLLIN)
        self.master = tk.Tk()
        self.master.config(background = config['bg'])
        self._geom = config['geometry']
        self.master.geometry("{0}x{1}+0+0".format(
            self.master.winfo_screenwidth()-config['pad'],
            self.master.winfo_screenheight()-config['pad'])
        )
        self.master.overrideredirect(config['fullscreen']) # make fullscreen
        self.master.focus_set()
        self.master.state(config['state'])
        self.labels = {} # dictionary of labels for display
        self.label_formats = {}
        for label in config['labels']:
            self.create_label(label, config['labels'][label])
        self.master.update_idletasks()
    
    # Create a new label
    def create_label(self, name, settings):
        pretty_print('HUD', '%s' % name)
        self.labels[name] = tk.StringVar()
        self.labels[name].set(settings['format'] % settings['initial_value'])
        self.label_formats[name] = settings['format']
        label = tk.Label(
            self.master,
            textvariable=self.labels[name],
            font=(settings['font_type'], settings['font_size']),
            bg=settings['bg_color']
        )
        label.pack()
        label.place(x=settings['x'], y=settings['y'])
    
    # List all labels  
    def list_labels(self):
        return [n for n in self.labels]
   
    # Generate event/error
    def generate_event(self, uid, task, data):
        event = {
            'uid' : 'HUD',
            'task' : task,
            'data' : data,
            'time': datetime.strftime(datetime.now(), "%H:%M:%S.%f"),
        }
        return event
    
    # Update the label values
    def run_async(self):
    
        # Ping host to request data update
        try:
            request = self.generate_event('HUD', 'pull', {}) #! TODO add error creator component, currently only makes update requests
            dump = json.dumps(request)
            self.zmq_client.send(dump)
        except Exception as error:
            pretty_print('HUD', str(error))
        
        # Wait for response and map values to labels
        try:
            time.sleep(self.timeout)
            socks = dict(self.zmq_poller.poll(self.timeout))
            if socks:
                if socks.get(self.zmq_client) == zmq.POLLIN:
                    dump = self.zmq_client.recv(zmq.NOBLOCK) # zmq.NOBLOCK
                    event = json.loads(dump)
                    pretty_print('HUD', 'Received response from OBD')
                    pretty_print('HUD', 'Updating Labels: %s' % str(event['data']))
                    event['time'] = time.time()
                    data = event['data'] #! the event determines which labels are changed
                    #!TODO Add handler for changing the display mode (i.e. from the ESC 'display_mode' key-val)
                    for name in data.keys():
                        try:
                            label_val = data[name]
                            if not np.isnan(label_val):
                                label_txt = self.label_formats[name] % str(label_val)
                                self.labels[name].set(label_txt)
                                self.master.update_idletasks()
                        except KeyError as error:
                            pretty_print('HUD', "label '%s' does not exist" % name)
                else:
                    pretty_print('HUD', 'ERROR: Poller Timeout')
            else:
                pretty_print('HUD', 'ERROR: Socket Timeout')

        except Exception as error:
            pretty_print('HUD', str(error))

if __name__ == '__main__':
    with open('config/HUD_debug.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load settings file
    display = SafeMode(config)
    while True:
        try:
            display.run_async()
        except KeyboardInterrupt:
            break
