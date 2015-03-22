"""
Vehicle HUD
This class is responsible for the user interface
"""

__author__ = 'Trevor Stanhope'
__version__ = 0.1

# Dependencies
import Tkinter as tk
from datetime import datetime 
import time
import json
import random

# Useful Functions 
def pretty_print(task, msg):
    date = datetime.strftime(datetime.now(), '%d/%b/%Y:%H:%M:%S')
    print("%s %s %s" % (date, task, msg))
    
# Classes (Note: class names should be capitalized)
class SafeMode: 

    def __init__(self, config):
        pretty_print('DISP_INIT', 'Setting Layout')
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
        
    def create_label(self, name, settings):
        pretty_print('NEW LABEL', '%s' % name)
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
        
    def list_labels(self):
        return [n for n in self.labels]
   
    def update_labels(self, event):
        pretty_print('DISP', 'Updating Labels')
        event['time'] = time.time()#! the event should determine which labels are changed
        for name in event.keys():
            try:
                label_txt = self.label_formats[name] % event[name]
                self.labels[name].set(label_txt)
                self.master.update_idletasks()
            except KeyError as error:
                pretty_print('DISP_ERR', "label '%s' does not exist" % name)
