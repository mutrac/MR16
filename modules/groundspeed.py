"""
Ground Speed Detector
This class is responsible for the groundspeed camera
"""

# Dependencies
import numpy as np
import cv2
import cv
import thread
import time

# Constants
VIDEO_WIDTH_CM = 19 # centimeters
VIDEO_HEIGHT_CM = 10
VIDEO_WIDTH_PX = 320 # centimeters
VIDEO_HEIGHT_PX = 240
VIDEO_PX2CM = float(VIDEO_WIDTH_CM) / float(VIDEO_WIDTH_PX)
FLANN_INDEX_KDTREE = 0
FLANN_TREES = 5
FLANN_CHECKS = 50
INDEX_PARAMS = dict(algorithm=FLANN_INDEX_KDTREE, trees=FLANN_TREES)
SEARCH_PARAMS = dict(checks=FLANN_CHECKS) # or pass empty dictionary
SURF_HESSIAN_FILTER = 1000
MOVING_AVERAGES = 10
MATCH_FACTOR = 0.65

# Functions
def convert_speed(x1, y1, x2, y2, t1, t2):
    distance_px = math.sqrt((x2 - x1)**2 + (y2 - y1)**2)
    distance_cm = VIDEO_PX2CM *  distance_px
    kph = 0.036 * distance_cm / (t2 - t1)
    return kph

# Classes
class BruteMatch:    

    def __init__(self, device_num = 0):
        self.video = cv2.VideoCapture(device_num)
        self.video.set(cv.CV_CAP_PROP_FRAME_WIDTH, VIDEO_WIDTH_PX)
        self.video.set(cv.CV_CAP_PROP_FRAME_HEIGHT, VIDEO_HEIGHT_PX)
        self.surf = cv2.SURF(SURF_HESSIAN_FILTER)
        self.matcher = cv2.BFMatcher()
        
    def get_kph(self):
        (s1, bgr1) = self.video.read()
        t1 = time.time()
        (s2, bgr2) = self.video.read()
        t2 = time.time()
        gray1 = cv2.cvtColor(bgr1, cv2.COLOR_BGR2GRAY)
        gray2 = cv2.cvtColor(bgr2, cv2.COLOR_BGR2GRAY)
        (points1, descriptors1) = surf.detectAndCompute(gray1, None)
        (points2, descriptors2) = surf.detectAndCompute(gray2, None)
        all_matches = self.matcher.knnMatch(descriptors1, descriptors2, k=2)
        good_matches = []
        for (m,n) in all_matches:
            if m.distance < MATCH_FACTOR * n.distance:
                good_matches.append(m)
        speeds = []
        for match in good_matches:
            point1 = points1[match.queryIdx]
            point2 = points2[match.trainIdx]
            x1 = int(point1.pt[0])
            y1 = int(point1.pt[1])
            x2 = int(point2.pt[0])
            y2 = int(point2.pt[1])
            kph = convert_speed(x1, y1, x2, y2, t1, t2)
            speeds.append(kph)
        avg_speed = np.mean(speeds)
        return avg_speed
    
    def close(self):
        print "---"
            
