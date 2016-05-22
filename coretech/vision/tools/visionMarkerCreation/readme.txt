
Marker Generation programs
Author Peter Unrein

The three python files in this file are meant to be used on MARKER IMAGES that Cozmo's camera can detect. 

all three functions require opencv 2.7. 

 before running you should....
 
   >  have the coretech-external repostitory installed
   
   > add to youre .bashrc or .profile the line 
     	    
		export PYTHONPATH="$PYTHONPATH:/Users/USRNAME/coretech-external/build/open\cv-2.4.8/lib/Debug"

The final format for these markers is a black and white image with a fiducial surrounding the image. between the edge of the image and the black fiducial there is a white/translucent "padding". These images can be 3/4 channel. If the marker images are 4 channel they can be a black image on a translucenct background or a black image on a white back ground. 
  


                                  ================== genRotatedMarker ===============

genRotatedMarker.py will copy-rotate a given directory of borderless marker images and determine if any rotations of it are identical to any other. The function will then place all unique representations of the images into the destination directory, for example, if one marker is and I and another is an A it is likely that only 2 rotations of the I will be needed while 4 versions of the A would be needed. 

due to the nature of the functions comparison algorithm it should only be assumed to be reliable on black marker images with a white or transparent back ground. These images can be with or without an alpha channel and should be .png images.

genRotatedMarker.py has two required inputs and two optional inputs. The required inputs include the source path and the destination path. the function can be called from terminal so if you call 

		    Python genRotatedMarker.py markers/ gallery/ 

then the function will  fill the directory ~/gallery/ with all unique rotations of ~/markers/. the two optional inputes are

 image size and threshold. the threshold is an arbitrary ratio used to determine when two images are similar and image size is the size at which the images are compared and output. the default image size and threshold are 256 x 256 and .30 respectively. The acceptable range for the threshold is based on the image size and a chart is provided below. 


  Image_size     |     Threshold range                                                                                                                        
##     -------------------------------------                                                                                                                        
##     32  (not ideal) |    .85 - 1.1         <-- at this size more images may drop than you'd like                                                                 
##     64              |    .60 - .75                                                                                                                               
##     128             |    .35 - .55                                                                                                                               
##     256             |    .2  - .35                                                                                                                               
##     512             |    .15 - .35                                                                                  


note that at a 32 x 32 image you will have the most errors (images that are different may be called similar and vice versa) 
but for example if I wanted to fill a directory with 512 x 512 images i would call 

    		   Python genRotatedMarker.py Users/peter.unrein/markers/ Users/peter.unrein/gallery/  512 .25



 ============= compareImages.py =================

compareImages will compare every image in a given directory with every other image in a directory. The function will print out any similarities that are found. 

the function uses the same comparison algorithm as genRotatedMarker so it will work best on libraries of .png marker images just like in genRotatedMarker.py. 

This function has 1 mandatory input and 2 optional ones. the first is the path to the directory where they images exist and are being compared. the optional inputs are for the comparison and thus are the same as the image size and threshold inputs that were described for genRotatedMarker.py
 
example call:    Python compareImages.py gallery/

example output: 

dice3_090 copy.png is similar to dice3_090.png
 
28 out of 28 possible unique comparisons were made.
 

============== addBorder.py ====================

addBorder will add the Fiducial borders to the images. The required inputs for this function are the source and destination paths. For example

  Pygthon addBorder.py gallery/ gallery/withFiducials 

will fill the directory gallery/withFiducials with versions of all the images in gallery only with square blacke borders. 

the optional inputs for this function include size, padding, radius, border, and innerpadding. 

size will determine the output size of an image (default: 512)

padding will determine the thickness of the outer layer of padding (default 0.0 of the image length)

radius is used to specify a radius for a curved corner and represents what the radius of those corners should be measured in decimal ratio to the lenght of the border. The maximum reasonable radius if .5 as it results in a complete circle (default: 0.0 or, no curve at all , max is .5)

border is the thickness of the border measured in ratio to image length (default .1)

innerpadding is the thickness of the innerpadding (the blank space between the marker image and the border) in ratio to the image length (default .1)



example : 
	Python addBorder.py gallery/ gallery/withFiducials 256 .1 .25 .1 .1
will fill gallery/withFiducials with images that have an outer padding and curved corners as well as a size of 256
