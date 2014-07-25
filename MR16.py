#!/usr/bin/env python
"""
MR16
MuTrac Release 2016
"""

__author__ = 'Trevor Stanhope'
__version__ = '0.1'

# Libraries
import cv2
import pymongo
import Tkinter as tk
import thread
import serial
import numpy as np
import time
from datetime import datetime 

def format_time():
    return datetime.strftime(datetime.now(), '[%Y-%m-%d %H:%M:%S.%f]')

class MR16:
    def __init__(self, ecus=4, cam=0):
        print('%s Initializing MR16' % format_time())
        self.GUI = GUI()
        self.HitchCam = HitchCam()
        
    def run(self):
        pass
        
    def __close__(self):
        pass
        
class GUI:
    def __init__(self):
        print('%s Initializing GUI' % format_time())
    
class ECU:
    def __init__(self, device_id, device_baud=57600):
        print('%s Initializing ECU: %s' % (format_time(), device_id))
        try:
            self.arduino = serial.Serial(device_id, device_baud)
        except Exception as error:
            print('\tERROR: %s' % str(error))

class HitchCam:
    
    def __init__(self, device_id=0):
        print('%s Initializing HitchCam: %s' % (format_time(), device_id))
        try:
            self.camera = cv2.VideoCapture(device_id)
        except Exception as error:
            print('\tERROR: %s' % str(error))
        
    def get_speed(self):
        print('%s Calculating Groundspeed' % format_time())
        return 0
        
    def get_image(self):
        (s, bgr) = self.camera.read()
        if s:
            return bgr

if __name__ == '__main__':
    session = MR16()
    session.run()
