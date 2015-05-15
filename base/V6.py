"""
V6 - Vision Speed Inference Extension

Runs as a ZMQ client.

The main function, run(), takes into account that the algorithm is slower than the 
camera framerate.
"""

__author__ = 'Trevor Stanhope'
__version__ = '0.1'

import cv2, cv
import numpy as np
import time
import sys
from matplotlib import pyplot as plt
from itertools import cycle
import zmq
import json
from datetime import datetime
import gps as gpsd

# Useful Functions 
def pretty_print(task, msg):
    date = datetime.strftime(datetime.now(), '[%d/%b/%Y:%H:%M:%S]')
    print("%s %s %s" % (date, task, msg))
    
class V6:

    """
    Initialize
    Optional Arguments:
        capture :
        fov : in degrees
        d : depth from surface (meters)
        roll : 
        pitch :
        yaw : 
        hessian : iterations of hessian filter
        frame
    """
    def __init__(self, capture=0, fov=0.75, f=6, aspect=1.33, d=1000, roll=0, pitch=0, yaw=0, hessian=1000, w=640, h=480, neighbors=2, factor=0.7):
        
        # Things which should be set once
        try:
            if capture.isdigit():
                capture = int(capture)
        except Exception as e:
            pass
        self.camera = cv2.VideoCapture(capture)
       
        # Things which can be changed at any time
        self.set_matchfactor(factor)
        self.set_resolution(w, h)
        self.set_fov(fov) # set the field of view (horizontal)
        self.set_aspect(aspect)
        self.set_focal_length(f)
        self.set_pitch(pitch) # 0 rad
        self.set_roll(roll) # 0 rad
        self.set_yaw(yaw) # 0 rad
        self.set_depth(d) # camera distance at center
        self.set_neighbors(neighbors)
        self.set_matcher(hessian)
        
    """
    Set the keypoint matcher configuration, supports BF or FLANN
    """
    def set_matcher(self, hessian, use_flann=False):
        try:
            self.surf = cv2.SURF(hessian)
            # Use the FLANN matcher
            if use_flann:
                self.FLANN_INDEX_KDTREE = 1
                self.FLANN_TREES = 5
                self.FLANN_CHECKS = 50
                self.INDEX_PARAMS = dict(algorithm=self.FLANN_INDEX_KDTREE, trees=self.FLANN_TREES)
                self.SEARCH_PARAMS = dict(checks=self.FLANN_CHECKS) # or pass empty dictionary
                self.matcher = cv2.FlannBasedMatcher(self.INDEX_PARAMS, self.SEARCH_PARAMS)
            # Use the Brute Force matcher
            else:
                self.matcher = cv2.BFMatcher()
        except Exception as e:
            print str(e)
            raise Exception("Failed to generate a matcher")
    """
    Close
    """  
    def close(self):
        self.camera.release()
        
    def set_matchfactor(self, factor):
        if factor < 0:
            raise Exception("Cannot have match less than 1")
        else:
            self.factor = factor
            
    """
    Set Neighbors
    """
    def set_neighbors(self, neighbors):
        if neighbors < 2:
            raise Exception("Cannot have neighbors less than 2")
        else:
            self.neighbors = neighbors
            
    """
    Set Resolution
    w : image width [px]
    h : image height [px]
    """
    def set_resolution(self, w, h):
        if (w <= 0) or (h <= 0):
            raise Exception("Improper frame size")
        else:
            self.w = w
            self.h = h
            self.camera.set(cv.CV_CAP_PROP_FRAME_WIDTH, w)
            self.camera.set(cv.CV_CAP_PROP_FRAME_HEIGHT, h)
    
    """
    Set distance at center of frame
    d : depth of view [m]
    """
    def set_depth(self, d):
        if d <= 0:
            raise Exception("Improper distance")
        else:
            self.d = d

    """
    Set Pitch
    pitch [rad]
    0 is orthogonal to surface 
    """
    def set_pitch(self, pitch):
        if pitch < 0:
            raise Exception("Cannot have negative inclination")
        elif pitch > np.pi/2.0:
            raise Exception("Cannot have inclination parallel to surface")
        else:
            self.pitch = pitch

    """
    Set Roll
    roll [rad]
    0 is parallel to surface 
    """
    def set_roll(self, roll):
        self.roll = roll

    """
    Set Yaw (relative to direction of travel)
    yaw [rad]
    0 is parallel to direction of travel 
    """
    def set_yaw(self, yaw):
        self.yaw = yaw
    
    """
    Set Field-of-View
    fov [rad]
    """
    def set_fov(self, fov):
        if fov <= 0:
            raise Exception("Cannot have negative FOV")
        if fov >= np.pi:
            raise Exception("Cannot have FOV greater than Pi radians")
        else:
            self.fov = fov

    """
    Set Aspect Ratio
    aspect [constant]
    """
    def set_aspect(self, aspect):
        if aspect <= 0:
            raise Exception("Cannot have negative aspect ratio")
        else:
            self.aspect = aspect

    """
    Set Focal Length
    f [mm]
    """
    def set_focal_length(self, f):
        if f <= 0:
            raise Exception("Cannot have negative focal length")
        else:
            self.f = f
    
    """ 
    Flush Buffer
    To get the most recent images, flush the buffer of older frames
    """
    def flush(self, frames=2):
        for i in range(frames):
            (s, bgr) = self.camera.read()
            
    """
    Match Images
    Find (good) pairs of matching points between two images
    Returns: [ (pt1, pt2), ... ]
    """
    def match_images(self, bgr1, bgr2):
        if (self.matcher is not None):
            if (bgr1 is not None) and (bgr2 is not None):
                gray1 = cv2.cvtColor(bgr1, cv2.COLOR_BGR2GRAY)
                gray2 = cv2.cvtColor(bgr2, cv2.COLOR_BGR2GRAY)
                (pts1, desc1) = self.surf.detectAndCompute(gray1, None)
                (pts2, desc2) = self.surf.detectAndCompute(gray2, None)
                matching_pairs = []
                if pts1 and pts2:
                    all_matches = self.matcher.knnMatch(desc1, desc2, k=self.neighbors)
                    try:
                        for (m,n) in all_matches:
                            if m.distance < self.factor * n.distance:
                                pt1 = pts1[m.queryIdx]
                                pt2 = pts2[m.trainIdx]
                                pt1 = (pt1.pt[0], pt1.pt[1])
                                pt2 = (pt2.pt[0], pt2.pt[1])
                                matching_pairs.append((pt1, pt2))
                    except Exception as e:
                        print str(e)
                return matching_pairs
            else:
                raise Exception('No images to match!')
        else:
            raise Exception("No matcher exists!")
    
    """
    Distance between two keypoints, where keypoints are in units of pixels
    Arguments:
        pt1 : (int x1, int y1) 
        pt2 : (int x2, int y2)
    Returns:
        distance : float
    """
    def distance(self, pt1, pt2, project=False):
        (x1, y1) = pt1
        (x2, y2) = pt2
        if project:
            (x1, y1) = self.project(x1, y1)
            (x2, y2) = self.project(x2, y2)
        dist = np.sqrt( (x2 - x1)**2 + (y2 - y1)**2 )
        return dist
    
    """
    Project points from pixels to real units
    Required arguments:
        x : 
        y : 
    Optional arguments:
        h : height of camera from surface (any units)
        fov : horizontal field of view
        w : width of image (in pixels)
        pitch : angle of inclination (in radians)
        f : focal length
    Returns:
        (X, Y): point location
    """
    def project(self, x, y, rotated=True):
        f = 2.0 * np.tan(self.fov / 2.0)
        if rotated:
            l = self.w / f
            theta = np.arctan(y / l)
            Y = self.d / np.tan( (np.pi / 2.0 - self.pitch) - theta)
            X = x * np.sqrt( (self.d**2 + Y**2) / (l**2 + y**2) )
        else:
            l = self.w / f
            theta = np.arctan(y / l)
            Y = self.d / np.tan( (np.pi / 2.0 - self.pitch) - theta)
            X = x * np.sqrt( (self.d**2 + Y**2) / (l**2 + y**2) )
        return (X, Y)
    
    """
    Optional:
        dt : time differential between bgr1 and bgr2
    Returns:
        v : the estimated speed of travel
        t : the estimated angle moved between two keypoints
        pairs : matching pairs between bgr1 and bgr2
        bgr1 : the first image
        bgr2 : the second image
        
    """
    def estimate_vector(self, dt=None, p_min=5, p_max=95):
        # Flush buffer
        for i in range(3):
            self.camera.read()
        # Read first
        (s1, bgr1) = self.camera.read()
        t1 = time.time()
        # Read second
        (s2, bgr2) = self.camera.read()
        t2 = time.time()
        # If no dt specificed:
        if not dt:
            dt = t2 - t1
        # Match keypoint pairss
        pairs = self.match_images(bgr1, bgr2)
        # Convert units
        dists = [self.distance(pt1, pt2, project=True) for (pt1, pt2) in pairs]
        dists = np.array(dists)
        v_all = (3.6 / 1000.0) * (dists / dt) # convert from m/s to km/hr
        v_min = np.percentile(v_all, p_min)
        v_max = np.percentile(v_all, p_max)
        v_top = v_all[v_all > v_min]
        v_best = v_top[v_top < v_max]
        return (v_best, pairs, bgr1, bgr2) # (gamma, theta)
    
    """
    Run the matching algorithm directly on a video source or file
    Optional Arguments:
        dt : the time between each frame
    """
    def run(self, dt=None, display=False, logging=False, name="%m-%d %H:%M.csv", gps=False, ultrasonic=False):
        if gps:
            try:
                self.gps = gpsd.gps()
                self.gps.stream()
            except Exception as e:
                raise e
                self.gps = None
        else:
            self.gps = gps
        if ultrasonic:
            pass #TODO add ultrasonic
        if logging:
            logname = datetime.strftime(datetime.now(), name)
            logfile = open(logname, 'w')
        while True:
            try:
                (v_best, pairs, bgr1, bgr2) = self.estimate_vector(dt=dt)
                if display:
                    output = np.array(np.hstack((bgr1, bgr2)))
                    for ((x1,y1), (x2,y2)) in pairs:
                        pt1 = (int(x1), int(y1))
                        pt2 = (int(x1 + self.w), int(y2))
                        cv2.circle(output, pt1, 5, (0,0,255), 2)
                        cv2.circle(output, pt2, 5, (0,255,0), 2)
                        cv2.line(output, pt1, pt2, (255,0,0), 1)
                    cv2.imshow("", output)
                    if cv2.waitKey(5) == 5:
                        break
                if logging:
                    datetime.strftime(datetime.now(), name)
                    newline = []
                    if self.gps is not None:
                        self.gps.next()
                        lon = self.gps.fix.longitude
                        lat = self.gps.fix.latitude
                        alt = self.gps.fix.altitude
                        print("%f N\t%f E\t%f m\t%f m/s" % (lat, lon, alt, v_best.mean()))
                        gps_data = [str(g) for g in [lon, lat, alt]] #TODO add more gps data
                        newline = newline + gps_data
                        
                    v_best = [str(v) for v in v_best.tolist()]
                    newline = newline + v_best
                    newline.append('\n')
                    logfile.write(','.join(newline))
            except Exception as e:
                raise e
            except KeyboardInterrupt as e:
                self.close()
                break
                
    """
    Run algorithm with buffer flushing
    This compensates for the relatively slow pace of the algorithm
    WARNING: this function is meant to be used with a LIVE VIDEO STREAM ONLY
    """
    def run_async(self, N=3, dt=None, precision=2, uid='CV6', task='push', zmq_addr="tcp://127.0.0.1:1980", zmq_timeout=0.1):
        self.zmq_addr = zmq_addr
        self.zmq_timeout = zmq_timeout
        self.zmq_context = zmq.Context()
        self.zmq_client = self.zmq_context.socket(zmq.REQ)
        self.zmq_client.connect(self.zmq_addr)
        self.zmq_poller = zmq.Poller()
        self.zmq_poller.register(self.zmq_client, zmq.POLLIN)
        v_hist = [0] * N
        for i in cycle(range(N)):
            try:
                self.flush()
                (v_best, pairs, bgr1, bgr2) = self.estimate_vector(dt=dt)
                v_hist[i] = np.median(v_best)
                v_avg = round(np.mean(v_hist), precision)
                event = {
                    'uid' : uid,
                    'task' : task, # generally, all events from CV6 are pushes
                    'data' : {
                        'v_avg' : v_avg
                    }
                }
                pretty_print('CV6', '%s' % str(event))
                try:
                    dump = json.dumps(event)
                    self.zmq_client.send(dump)
                    time.sleep(self.zmq_timeout)
                    socks = dict(self.zmq_poller.poll(self.zmq_timeout))
                    if socks:
                        if socks.get(self.zmq_client) == zmq.POLLIN:
                            dump = self.zmq_client.recv(zmq.NOBLOCK) # zmq.NOBLOCK
                            response = json.loads(dump)
                            pretty_print('CV6', 'Received: %s' % str(response))
                        else:
                            pass
                    else:
                        pass
                except Exception as err:
                    pretty_print('CV6', str(err))
                except KeyboardInterrupt as err:
                    raise KeyboardInterrupt
            except KeyboardInterrupt:
                raise KeyboardInterrupt

if __name__ == '__main__':
    try:
        ext = V6(capture=0)
        ext.run_async(dt=1/25.0)
    except Exception:
        ext.close()
