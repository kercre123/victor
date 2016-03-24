import cv2
import numpy as np
import sys
import os


'''
file: cbcalib.py
author: Peter Unrein , intern 2015
purpose: find the focal lenghth, principal point, and distortion coefficients of a cozmo camera given a series of images
or more information on the functions used from opencv navigate to the page below:                        
http://docs.opencv.org/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html

instructions


- add to your ./bashrc or .profile file the line 
export PYTHONPATH="$PYTHONPATH:<path to CORETECH-EXTERNAL repository>/build/opencv-2.4.8/lib/Debug"

-for the best accuracy take at least 10 photographs of a checkerboard pattern at multiple angles (it will work with less but opencv reccomends at least 10)
      >see example images to see what calibration images should look like
      >the calibration target should be close to filling the field of veiw so distortion can be detected
      >the calibration target should be held at all the angles shown in the example images at least once

-to call the fn on a group of jpg images of an 9x7 square checkerboard, call the fn and give the filepath as an argument 
example:   python cbcalib.py /User/peter.unrein/Desktop/calib_images/ 

-if you are not using an 9X7 checker board just add arguments for lenth and width. 
example, for 10x9 checkerboard write:
 python /Users/peter.unrein/Desktop/calib_images/  10 9
- similarly you can change the square size (default is a square with a side 30 mm long) by setting the 5th argument to a new size
for example, for a 9X7 checkerboard with a 27 mm squares you'd write
python /Users/peter.unrein/Desktop/calib_images/  9 7 27
- the path to the folder containing your .jpg images should end with a /   
- ensure that there are no other .jpg images in the folder not meant for calibration
- non .jpg files will be ignored.
- the resulting reprojection error should be between .1 and 1.0. If it isn't you may need more/better calibration images.
'''

def main(filepath,length = 9 , width = 7,square_size = 30):
    length = int(length)-1 #opencv actually uses the inner points of a checkerboard
    width = int(width)-1
    image_list = []
    obj_pts = []
    img_pts = []
    image_list = os.listdir(filepath)
    count = 0
    while count < len(image_list): #makes the list of image names
        if image_list[count].find('.jpg')== -1:
            image_list.pop(count)
        else:
            count += 1
    if len(image_list)==0:
        print ("no images found, make sure you typed the correct path")
        return
    print ('images found: ' +str(image_list))

    pattern_size = (width,length)
    in_pts = []
    for i in range(length): #generates a list of object points base on 30 mm squares
        for j in range(width):
            x = length -1
            y = width -1
            x = x-i
            y = y-j
            in_pts.extend([[x*float(square_size),y*float(square_size),0]])
 
    for im in image_list: #will look for a checkerboard in each image listed
       
        imagename = filepath + im
        image = cv2.imread(imagename,0)
        h,w = image.shape #the image size is needed later
        found, corners = cv2.findChessboardCorners(image, pattern_size) 
        if found: #if the checkerboard image is found it will be displayed 
            pts = np.array(corners,np.int32)
            cv2.polylines(image,[pts],True,(125,0,255))
            cv2.imshow(im,image)
            cv2.waitKey(2500)
            obj_pts.append(in_pts) #object pts always grows with img_pts
            img_pts.append(corners.reshape(-1,2)) #the checkerboard pts are added to img_pts
        else:
            print ("no pattern found for image -->  " + im)
    cv2.destroyAllWindows() #gets rid of those pesky opencv windows
    obj_pts = np.array(obj_pts)
    img_pts = np.array(img_pts)
    obj_pts = obj_pts.astype(np.float32)
    img_pts = img_pts.astype(np.float32)
    retval,cameraMatrix,distCoeffs,rvecs,tvecs = cv2.calibrateCamera(obj_pts,img_pts,(h,w)) #does the hard part
    print ('reprojection error: ' + str(retval)+ ' this is the number of pixels between the found points and the same points once they were reprojected using this calibration. ideally this error should be between .1 and 1.0') 
    print ('camera Matrix')
    print cameraMatrix
    print ('fx: ' + str(cameraMatrix[0][0]))
    print ('fy: '  + str(cameraMatrix[1][1]))
    print ('cx: ' +str(cameraMatrix[0][2]))
    print ('cy: ' +str(cameraMatrix[1][2]))
    print ('distortion coefficients: ' + str(distCoeffs))
    return
if len(sys.argv) <= 1:
    print 'please call the function with a path to the images you wish to calibrate'
    print 'for example: python cbcalib.py /Users/peter.unrein/Desktop/calib_images/'
elif len(sys.argv) > 4:
    main(str(sys.argv[1]),sys.argv[2],sys.argv[3],sys.argv[4])
elif len(sys.argv) > 3:
    main(str(sys.argv[1]),sys.argv[2],sys.argv[3])
elif len(sys.argv) > 2:
    main(str(sys.argv[1]),sys.argv[2])
else:
    main(str(sys.argv[1]))
