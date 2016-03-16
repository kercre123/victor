
import os
import cv2
import sys
import numpy as np

#####################################################################
## Rotated Image generator
##
##Required:
##      OpenCV
#          - have coretech installed
#          - add to your ./bashrc or .profile file the line:                                             
#                 export PYTHONPATH="$PYTHONPATH:/Users/USRNAME/coretech-external/build/open\cv-2.4.8/lib/Debug"  ##
## input: 
##     -a source directory with base marker images
##     -a destination directory where those images will be stored  
##       [Optional: will default to <source>/rotated if empty or not specified]
##
## output:
##     - the destination directory will have all unique rotations of a 
##Purpose: 
##
##    The purpose of this program is to fill the destination directory with all possible rotations of the base markers provided
## in the source directory. The gallery generated should have no repeated interations of a marker and no more than rotations of
## a marker that are 90 degreee rotations of eachother
##
##
##
##
## Calling the Program: 
##       To call the program you must have two existing directories. The first directory must contain potential 
##    marker images. These .png images should be black markers with a filled white background. optional inputs 
#     include the image_size and the comparison threshold. The image_size and threshold are set at 256 and .40 
##    without input. Image_size is the size of the images output to the destination. The comparison threshold is 
##    a ratio that is used to determine how close two images must be to be considered similar. The range for what
##    may be considered a reasonable threshold varies based on image_size. A chart is provided below with ranges:
##    when choosing a threshold and image size it is best to choose one that matches it's range.
##   
##   From terminal the call will look like this:
##      Python genRotatedMarker.py srcpath/ dstpath/ image_size threshold
##
##      Image_size     |     Threshold range
##     -------------------------------------
##     32  (not ideal) |    .85 - 1.1         <-- at this size more images may drop than you'd like
##     64              |    .60 - .75
##     128             |    .35 - .55
##     256             |    .2  - .35
##     512             |    .15 - .35
##   (if you choose an image size not listed, try to choose a reasonable threshold based on these ranges) 
##
##
##
#####################################################################




def main(src = 'markers', dst = '',size = 256,threshold= .30):
    namearray = os.listdir(src)
    namearray = typefilter(namearray,'.png')
    imagearray = makesmallImglist(src,namearray,size)
    myimagearray = []
    for i in range(len(imagearray)):
        imagearray[i] = copyrotate(imagearray[i],size)
        namearray[i] = createimnames(namearray[i])
   
        myImlist = makeMyimagelist(imagearray[i],namearray[i])
        myImlist = compare(myImlist,threshold,size)
        myimagearray.append(myImlist)
    if len(dst)==0:
        dst = os.path.join(src, 'rotated')
    if not os.path.exists(dst):
        os.mkdir(dst)
    addtoGallery(dst,myimagearray)

def addtoGallery(dst,image_array):
    for i in range(len(image_array)):
        for j in range(len(image_array[i])):
            loc = os.path.join(dst,image_array[i][j].name)
            cv2.imwrite(loc,image_array[i][j].image)
   


def makeMyimagelist(image_list, name_list):
    myimlist = []
    for i in range(len(name_list)):
        myimlist.append(myImage(image_list[i],name_list[i]))
    return myimlist
def createimnames(name):
    suffix_list = ['_000.png','_090.png','_180.png','_270.png']
    name_tmp = name[:-4]
    name_list = []
    for nm in suffix_list:
        name_list.append(name_tmp + nm)
    return name_list

def makesmallImglist(path,namelist,image_size):
    img_list = []
    for im in namelist:
        img_list.append([ cv2.resize(cv2.imread(os.path.join(path,im),-1),(image_size,image_size))])

    return img_list
        
def copyrotate(image_list,size):
    if len(image_list) == 0:
        print 'list too short'
        return image_list
    elif len(image_list)== 4:
        return image_list
    else:
        newim = rotate90(image_list[-1],size)
        image_list += [newim] 
        image_list = copyrotate(image_list,size)
        return image_list

def rotate90(src,image_size):
    a0 = (0,0)
    b0 = (0,image_size)
    c0 = (image_size,image_size)
    srctri = np.array([a0,b0,c0])
    srctri = srctri.astype(np.float32)
    a1 = (0,image_size)
    b1 = (image_size,image_size)
    c1 = (image_size,0)
    dsttri = np.array([a1,b1,c1])
    dsttri = dsttri.astype(np.float32)
    M = cv2.getAffineTransform(srctri,dsttri)
    new_image = cv2.warpAffine(src,M,(image_size,image_size))
    return new_image

def typefilter(name_list,Ftype = '.png'):
    index = 0
    while index < len(name_list):
        if name_list[index].find(Ftype) == -1:
            name_list.pop(index)
        else:
            index += 1
    return name_list

def removeDotFiles(file_list):
    index = 0
    while index< len(file_list):
        if file_list[index][0]=='.':
                file_list.pop(index)
        else:
            index+= 1
    return file_list


def slideshow(image_array):
    for i in range(len(image_array)):
        for j in range(len(image_array[i])):
            cv2.imshow(image_array[i][j].name,image_array[i][j].image)
            print image_array[i][j].name



def compare(image_list,threshold,size):
    first_image = image_list[0] 
    small_list = image_list[1:]
    index_list = []
    if len(small_list) == 0:
        return [first_image]
    for i in range(len(small_list)):
        if detEQ(first_image.binImage,small_list[i].binImage,threshold,size):
            index_list.append(i)
        else:
            pass
    index_list.reverse()
    if len(index_list) == 0:
        pass
    else:
        for i in index_list:
            small_list.pop(i)
    if len(small_list) == 0:
        return [first_image]
    else:
        return [first_image] + compare(small_list,threshold,size)

   


def detEQ(image1,image2,threshold,image_size):
    diff = image1-image2
   
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



def findGradientImage(image):
    grad_x = cv2.Sobel(image,cv2.CV_64F,1,0,5)
    grad_y = cv2.Sobel(image,cv2.CV_64F,0,1,5)
    abs_grad_x = cv2.convertScaleAbs(grad_x)
    abs_grad_y = cv2.convertScaleAbs(grad_y)
    abs_grad_x = abs_grad_x.astype(np.uint8)
    abs_grad_y = abs_grad_y.astype(np.uint8)
    grad = cv2.addWeighted(abs_grad_x,.5,abs_grad_y,.5,0)
    return grad
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
               
                return cv2.bitwise_not(imarray[0])
            else:
                
                return imarray[-1]
        else:
            return cv2.bitwise_not(imarray[0])

if len(sys.argv) > 4:
    main(str(sys.argv[1]),str( sys.argv[2]),int( sys.argv[3]), float( sys.argv[4]))
elif len(sys.argv) > 3:
    main(str(sys.argv[1]),str( sys.argv[2]),int( sys.argv[3]))
elif len(sys.argv) > 2:
    main(str(sys.argv[1]),str( sys.argv[2]))
elif len(sys.argv) > 1:
    main(str(sys.argv[1]))
else:
    print ('please provide a source path and a destination path')
