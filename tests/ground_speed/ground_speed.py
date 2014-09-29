import cv2, cv
import numpy
import math
import time
import sys

## Constants
VIDEO_CAPTURE = sys.argv[1]
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
FPS = 10.0
MOVING_AVERAGES = 10

## Objects
try:
    print('Starting video stream...')
    video = cv2.VideoCapture(VIDEO_CAPTURE)
    video.set(cv.CV_CAP_PROP_FRAME_WIDTH, VIDEO_WIDTH_PX)
    video.set(cv.CV_CAP_PROP_FRAME_HEIGHT, VIDEO_HEIGHT_PX)
    print('\tOKAY')
except Exception as err:
    print('\tERROR: %s' % str(err))
    
try:
    print('Initializing SURF detector...')
    surf = cv2.SURF(SURF_HESSIAN_FILTER)
    print('\tOKAY')
except Exception as err:
    print('\tERROR: %s' % str(err))

try:
    print('Initializing Brute-Force Matcher...')
    brute = cv2.BFMatcher()
    print('\tOKAY')
except Exception as err:
    print('\tERROR: %s' % str(err))

try:
    print('Initializing FLANN Matcher...')
    flann = cv2.FlannBasedMatcher(INDEX_PARAMS,SEARCH_PARAMS)
    print('\tOKAY')
except Exception:
    print('\tERROR')
    
## Convert to Speed
def speed(x1, y1, x2, y2):
    distance_px = math.sqrt((x2 - x1)**2 + (y2 - y1)**2)
    distance_cm = VIDEO_PX2CM *  distance_px # assume constant distance
    speed_kmh = 0.036 * distance_cm * FPS # assue constant fps
    return speed_kmh
    
# Loop
print('Starting video...')
frame = 0
moving_average = [0] * MOVING_AVERAGES
(s1, bgr1) = video.read()
(s2, bgr2) = video.read()
gray1 = cv2.cvtColor(bgr1, cv2.COLOR_BGR2GRAY)
gray2 = cv2.cvtColor(bgr2, cv2.COLOR_BGR2GRAY)
(points1, descriptors1) = surf.detectAndCompute(gray1, None)
(points2, descriptors2) = surf.detectAndCompute(gray2, None)
while True:
    a = time.time()
    bgr1 = bgr2
    (s, bgr2) = video.read()
    if s:
        try:
            print('\nFrame #%d' % frame)
            bgr1 = bgr2
            gray1 = gray2
            points1 = points2
            descriptors1 = descriptors2
            (s, bgr2) = video.read()
            print('Converting to Grayscale...')
            gray2 = cv2.cvtColor(bgr2, cv2.COLOR_BGR2GRAY)
            print('Finding to Features...')
            (points2, descriptors2) = surf.detectAndCompute(gray2, None)
            print('Matching Keypoints...')
            all_matches = brute.knnMatch(descriptors1, descriptors2, k=2)
            good_matches = []
            for m,n in all_matches:
                if m.distance < 0.65 * n.distance:
                    good_matches.append(m)
            print('All Matches: %d' % len(all_matches))
            print('Good Matches: %d' % len(good_matches))
            distances = []
            output = numpy.array(numpy.hstack((bgr1,bgr2)))
            for match in good_matches:
                point1 = points1[match.queryIdx]
                point2 = points2[match.trainIdx]
                x1 = int(point1.pt[0])
                y1 = int(point1.pt[1])
                x2 = int(point2.pt[0])
                y2 = int(point2.pt[1])
                cv2.line(output, (x1, y1), (x2 + VIDEO_WIDTH_PX, y2), (100,0,255), 1)
                distances.append(speed(x1, y1, x2, y2))
            print('Output image...')
            average = numpy.mean(distances)
            print('Estimated Ground Speed: %f' % average)
            if not numpy.isnan(average):
                moving_average.pop(0)
                moving_average.append(average)
            print('Moving Average Ground Speed: %f' % numpy.mean(moving_average))
            cv2.imshow('', output)
            if cv2.waitKey(1) == 27:
                pass
            b = time.time()
            print('Freqency: %f' % (1 / float(b -a)))
        except Exception as error:
            print(str(error))
            break
    else:
        break
