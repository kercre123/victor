import os 
import cv2
import sys
import numpy as np


#####################################################################
##     Comparing marker images in a gallery
##
##
##Required:                                                                                                                       
##      OpenCV                                                                                                                    
#          - have coretech installed                                                                                              
#          - add to your ./bashrc or .profile file the line:                                                                      
#                 export PYTHONPATH="$PYTHONPATH:/Users/USRNAME/coretech-external/build/open\cv-2.4.8/lib/Debug"  ##            
##
## Calling the Program:
##     To call the program you must provide a source directory. Optional inputs include an image size and a threshold. the call should look like this
##         Python compareImages.py path image_size* threshold*
##                  *these inputs are optional 
## These thresholds are reasonable for these image sizes. You may adjust the threshold how you wish based on how similar or different you want images
## to be                                                                                                                              
##      Image_size     |     Threshold range                                                                                      
##     -------------------------------------                                                                                      
##     32  (not ideal) |    .85 - 1.1         <-- at this size more images may fail than you'd like                               
##     64              |    .60 - .75                                                                                             
##     128             |    .35 - .55                                                                                             
##     256             |    .2  - .35                                                                                             
##     512             |    .15 - .35                                                                                             
##   (if you choose an image size not listed, try to choose a reasonable threshold based on these ranges)                         
##                                                                                                         
##
##      input:
##         -a directory containing marker images without the border
##         -OPTIONAL: A size at which images should be compared                            (default 256)
##         -OPTIONAL: A threshold at which it will be decided if images are similar or not (default .35) 
##      
##      output:
##         - The program will print into the terminal window which images are similar 
##         - The program will also print how many comparisons where made
##Purpose:
##   Given a directory this program will compare each image in that directory and determine if they are similar. The threshold can be adjusted to determine
## how similar is "too similar". 
##
##
##
##
##
##
##
##
##
####################################################################


def main(src = 'gallery/', size = 256, threshold = .30 ):
    print ' '
    name_list = os.listdir(src)
    name_list = typefilter(name_list,'.png')
    imagearray = makeImglist(src,name_list,size)   
   # slideshow(imagearray)
    imagearray = comparison(imagearray,size, threshold)
    print ' '

def comparison(imagelist, size, threshold):
    count = 0
    for i in range(len(imagelist) - 1):
        for jim in imagelist[i+1:]:
            count += 1
           # print ('comparing ' + str(imagelist[i].name) + ' to ' + str(jim.name))
            if detEQ(imagelist[i].binImage,jim.binImage,size,threshold):
                print ( str(imagelist[i].name) + ' is similar to ' + str(jim.name))
                compshow(imagelist[i], jim)
            else:
                pass
    length = len(imagelist)
    total = length * (length -1)
    total = total/2
    print ' '
    print (str(count)+' out of ' + str(total) + ' possible unique comparisons were made.')
def compshow(image1,image2):
    string = image1.name + ' minus ' + image2.name
    image = cv2.absdiff(image1.binImage,image2.binImage)
    cv2.imshow(string,image)
def typefilter(name_list,Ftype = '.png'):
    index = 0
    while index < len(name_list):
        if name_list[index].find(Ftype) == -1:
            name_list.pop(index)
        else:
            index += 1
    return name_list


def makeImglist(path,namelist,image_size):
    img_list = []
    for im in namelist:
        img_list.append(myImage(cv2.resize(cv2.imread(path+im,-1),(image_size,image_size)), im))
    return img_list

def detEQ(image1,image2,image_size,threshold):
    diff = cv2.bitwise_xor(image1,image2)
   
    diff = diff.cumsum(0)
    diff = diff.cumsum(1)
    diff = diff[-1][-1]
   
    total = image1.cumsum(0)
    total = total.cumsum(1)
    total = total[-1][-1]
    diff  = diff.astype('float32')
    total = total.astype('float32')

    ratio =  diff/total


    if ratio <= threshold:
        return True

    else:
        return False



def slideshow(image_array):
    for i in range(len(imrage_array)):
        for j in range(len(image_array[i])):
            cv2.imshow(image_array[i][j].name,image_array[i][j].image)
            print image_array[i][j].name

class myImage():
    def __init__(self,image,name):
        self.image = image
        self.name = name
        self.binImage = self.binim()
    def binim(self):
        baseimage = cv2.GaussianBlur(self.image,(3,3),0)
        imarray = cv2.split(baseimage)
        if len(imarray) == 4:
           
            if imarray[-1].mean() >225:
                cv2.imshow(self.name,cv2.bitwise_not(imarray[0]))
                return cv2.bitwise_not(imarray[0])
            else:
                cv2.imshow(self.name,imarray[-1])
                return imarray[-1]
        else:
            cv2.imshow(self.name,cv2.bitwise_not(imarray[0]))
            return cv2.bitwise_not(imarray[0])





if len(sys.argv) > 3:
    main(str(sys.argv[1]),str( sys.argv[2]),int( sys.argv[3]))
elif len(sys.argv) > 2:
    main(str(sys.argv[1]),str( sys.argv[2]))
elif len(sys.argv) > 1:
    main(str(sys.argv[1]))
else:
    print ('please provide a source path and a destination path')
