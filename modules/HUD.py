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

# Useful Functions 
def pretty_print(task, msg):
    date = datetime.strftime(datetime.now(), "%Y-%m-%d %H:%M:%S.%f")
    print("%s %s %s" % (date, task, msg))
    
def save_config(config, filename):
    with open(filename, 'w') as jsonfile:
        jsonfile.write(json.dumps(config, indent=True))
    
# Classes (Note: class names should be capitalized)
class SafeMode: 

    def __init__(self, config):
        pretty_print('INIT', 'Setting Layout')
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
   
    def update_labels(self, new_labels):
        pretty_print('DISPLAY', 'Updating Labels')
        for name in new_labels:
            try:
                label_txt = self.label_formats[name] % new_labels[name]
                self.labels[name].set(label_txt)
                self.master.update_idletasks()
            except KeyError as error:
                pretty_print('ERROR', "label '%s' does not exist" % name)
                
if __name__ == '__main__':
    config = {
        'geometry' : '640x480+0+0',
        'fullscreen' : True,
        'state' : 'normal',
        'pad' : 3,
        'bg' : '#FFFFFF',
        'labels' : {
            'test' : {
                'format' : '%s sec',
                'initial_value' : '',
                'x' : 0,
                'y' : 20,
                'font_type' : 'Helvetica',
                'font_size' : 24,
                'bg_color' : '#FFFFFF'
            }
        }
    }
    save_config(config, 'test')
    display = SafeMode(config)
