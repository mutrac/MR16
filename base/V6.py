import cv2, cv
import numpy as np
import time

class V6:
    
    """
    
    """
    def __init__(self, capture=0, fov=0.7, dist=100, heading=0, incl=0, hessian=500, frame_w=640, frame_h=480, neighbors=2, factor=0.65):
        self.camera = cv2.VideoCapture(capture)
        self.set_matchfactor(factor)
        self.set_resolution(frame_w, frame_h)
        self.set_fov(fov)
        self.set_heading(heading)
        self.set_inclination(incl) # 0 rad
        self.set_distance(dist) # camera distance at center
        self.set_neighbors(neighbors)
        self.surf = cv2.SURF(hessian)
        self.matcher = cv2.BFMatcher()
    
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
    frame_w [px]
    frame_h [px]
    """
    def set_resolution(self, frame_w, frame_h):
        if (frame_w <= 0) or (frame_h <= 0):
            raise Exception("Improper frame size")
        else:
            self.frame_w = frame_w
            self.frame_h = frame_h
            self.camera.set(cv.CV_CAP_PROP_FRAME_WIDTH, frame_w)
            self.camera.set(cv.CV_CAP_PROP_FRAME_HEIGHT, frame_h)
    
    """
    Set distance at center of frame
    dist [cm]
    """
    def set_distance(self, dist):
        if dist <= 0:
            raise Exception("Improper distance")
        else:
            self.dist = dist
    
    """
    Set Inclination
    incl [rad]
    0 is orthogonal to surface 
    """
    def set_inclination(self, incl):
        if incl < 0:
            raise Exception("Cannot have negative inclination")
        elif incl > np.pi/2.0:
            raise Exception("Cannot have inclination parallel to surface")
        else:
            self.incl = incl
    
    """
    Set Field-of-View
    fov [rad]
    """
    def set_fov(self, fov):
        if fov <= 0:
            raise Exception("Cannot have negative FOV")
        else:
            self.fov = fov
    
    """
    Set Heading
    heading [rad]
    The heading of the camera relative to the direction of travel
    0 = in direction of travel
    pi/2 = right of direction of travel
    -pi/2 = left of direction of travel
    pi = opposite direction of travel
    """
    def set_heading(self, heading):
        self.heading = heading
    
    """ 
    Flush video buffer
    To get the most recent images, flush the buffer of older frames
    """
    def flush_buffer(self, frames):
        for i in range(frames):
            (s, bgr) = self.camera.read()
    
    """
    Calculate Speed
    """
    def speed(self, samples=2, display=False):
        results = []
        for i in range(samples):
            attempts = 0
            s1 = False
            s2 = False
            while not (s1 and s2):
                t1 = time.time()
                (s1, bgr1) = self.camera.read()
                t2 = time.time()
                (s2, bgr2) = self.camera.read()
                attempts += 1
                if attempts > 10:
                    raise Exception("Camera failure")
            else:
                gray1 = cv2.cvtColor(bgr1, cv2.COLOR_BGR2GRAY)
                gray2 = cv2.cvtColor(bgr2, cv2.COLOR_BGR2GRAY)
                (pts1, desc1) = self.surf.detectAndCompute(gray1, None)
                (pts2, desc2) = self.surf.detectAndCompute(gray2, None)
                if display: output = np.array(np.hstack((bgr1,bgr2)))
                if pts1 and pts2:
                    all_matches = self.matcher.knnMatch(desc1, desc2, k=self.neighbors)
                    good_matches = []
                    for m,n in all_matches:
                        if m.distance < self.factor * n.distance:
                            good_matches.append(m)
                    for match in good_matches:
                        pt1 = pts1[match.queryIdx]
                        pt2 = pts2[match.trainIdx]
                        x1 = int(pt1.pt[0])
                        y1 = int(pt1.pt[1])
                        x2 = int(pt2.pt[0])
                        y2 = int(pt2.pt[1])
                        if display: cv2.line(output, (x1, y1), (x2 + self.frame_w, y2), (100,0,255), 1)
                        distance_px = np.sqrt( (x2 - x1)**2 + (y2 - y1)**2 ) #! this is going to get complex with heading/incl compensation
                        distance_units = distance_px * 2 * self.dist * np.tan(self.fov / 2.0) / self.frame_w
                        speed = distance_units / (t2 - t1)
                        results.append(speed)
                else:
                    raise Exception("No matches found: check hessian value")
                if display:
                    cv2.imshow('', output)
                    if cv2.waitKey(5) == 3:
                        pass
        return np.mean(results)

if __name__ == '__main__':

    # Test
    test = V6(capture='tests/grass_2kmh_25fps.avi')
    try:
        while True:
            print test.speed(display=True)
    except KeyboardInterrupt:
        test.close()
    
    # Run as daemon   
    with open('config/v6_v1.json', 'r') as jsonfile:
        config = json.loads(jsonfile.read()) # Load config file
        cam = V6(config) # start watchdog
        cam.speed()
