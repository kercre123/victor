import os
import cv2
import sys
import numpy as np
import math

WHITE3 = [255,255,255]
BLACK3 = [0,0,0]
BLACK = [0,0,0,255]
TRANSPARENT= [0,0,0,0]
WHITE = [255,255,255,255]
BORDERWHITE = [255,255,255,0]
#############################
#tool: Border adding tool
# Author : Peter Unrein, Intern 2015
#Required:
#      OpenCV                                                                                                
#          - have coretech installed                                                                
#          - add to your ./bashrc or .profile file the line:   
                                                                            
#                 export PYTHONPATH="$PYTHONPATH:/Users/USRNAME/coretech-external/build/open\cv-2.4.8/lib/Debug"                
#

#Purpose: 
# This code will add fiducial borders to all the markers in a directory and save these new versions of the markers in a directory. The final marker images should be comprised of the original marker images surrounded by some white/transparent padding, a black fiducial and optionally some outer white or transparent padding. 
# 
#
#
#Calling the program:
#    >From a terminal windo call:
#           Python addBorder.py sourcePath destinationPath [size [padding [radius [border [innerpad]]]]]
#     > The fucntion should be called on a directory of png images with a black marker and a transparent or white filling. they can be RGBA or RGB images or a mixture of both.  
#     > the one mandatory input is the source path. The rest of the inputs (destinatino path, size, padding, radius, border, innerpadding) are set to default values unless they are overwritten
# the default settings are :
#          dst = ''         (where to put the output images. Defaults to <src>/withFiducials if empty or unspecified)
#          size = 512
#          padding = 0.0    (the space between the fiducial and the end of the image
#          radius = 0.0     (the radius of the corner (.5 is the highest this should go and will create a circle)) 
#          border = 0.1     (the thickness of the fiducial that will be added)
#          innerpad = 0.1   (the thickness of the space betweent he edge of the marker and the fiducial)
#    all of these are inputs that represent what their size should be based on a fraction of the fiducial length. for example if the fiducial is 256 pixels long a .1 border will be 25-26 pixels thick.  
#############################
def main(src = 'gallery',dst = '',size=512, padding = 0.0,radius= 0.0, border = .1, innerpad = .1):
    name_list = os.listdir(src)
    print 'size ' + str(size)
    print 'padding ' + str(padding)
    print 'radius ' + str(radius)
    print 'border ' + str(border)
    print 'innerpad ' + str(innerpad)
    name_list = typefilter(name_list,'.png')
    image_list = makeImglist(src,name_list,size)
    if len(dst)==0:
        dst = os.path.join(src,'withFiducials')
    if not os.path.exists(dst):
        print 'Making directory: ' + dst
        os.mkdir(dst)
    for i in image_list:
        if i.chan >3:  #image has RGBA channels
            if i.image.mean() < 60:  #image has a translucent fill
                out = makeBorder(i,size,padding,border,innerpad,radius)
            else: #image has a white fill
                out = makeBorderW(i,size,padding,border,innerpad,radius)
        else: #RGB image
            out = makeBorder3(i,size,padding,border,innerpad,radius)
        path = os.path.join(dst,i.name)
        cv2.imwrite(path,out)
def makeBorderW(marker,size,padding,border,innerpad,radius): #makes a border for RGBA images with a white fill
    image = marker.image
    I = marker.image.shape[0]
    F = findF(I,padding)
    border = F * border
    innerpad = F * innerpad
    padding = F * padding
    smallimage = crop(image)

    rSize = I - 2*(border + innerpad + padding)
    rSize = np.float32(rSize)
    smallimage = cv2.resize(smallimage,(rSize,rSize))

    border = np.float32(border)
    innerpad = np.float32(innerpad)
    padding = np.float32(padding)
    borderType = cv2.BORDER_CONSTANT
    framed = cv2.copyMakeBorder(smallimage,innerpad,innerpad,innerpad,innerpad, borderType,value = WHITE)
    if radius > 0:

        framed = cv2.copyMakeBorder(framed,border,border,border,border,borderType,value = WHITE)
        fiducial = makeCornerW(F,radius,border)
        x,y,z =  fiducial.shape
        framed = cv2.resize(framed,(x,y))
        framed -= fiducial
    else:
        framed = cv2.copyMakeBorder(framed,border,border,border,border,borderType,value = BLACK)
    framed = cv2.copyMakeBorder(framed,padding,padding,padding,padding, borderType,value =WHITE)

    return framed
def makeBorder3(marker,size,padding,border,innerpad,radius): #makes a border for an RGB image
    image = marker.image
    I = marker.image.shape[0]
    F = findF(I,padding)
    border = F * border
    innerpad = F * innerpad
    padding = F * padding
    smallimage = crop(image)

    rSize = I - 2*(border + innerpad + padding)
    rSize = np.float32(rSize)
    smallimage = cv2.resize(smallimage,(rSize,rSize))

    border = np.float32(border)
    innerpad = np.float32(innerpad)
    padding = np.float32(padding)
    borderType = cv2.BORDER_CONSTANT
    framed = cv2.copyMakeBorder(smallimage,innerpad,innerpad,innerpad,innerpad, borderType,value = WHITE3)
    if radius > 0:

        framed = cv2.copyMakeBorder(framed,border,border,border,border,borderType,value = WHITE3)
        fiducial = makeCorner3(F,radius,border)
        x,y,z =  fiducial.shape
        framed = cv2.resize(framed,(x,y))
        framed -= fiducial
    else:
        framed = cv2.copyMakeBorder(framed,border,border,border,border,borderType,value = BLACK3)
    framed = cv2.copyMakeBorder(framed,padding,padding,padding,padding, borderType,value =WHITE3)
    framed = cv2.resize(framed,(size,size))
    return framed

def makeBorder(marker,outSize, padding ,border, innerpad, radius):
  
    image = marker.image
    I = marker.image.shape[0]
    F = findF(I,padding)
  
  
    border = F * border
    innerpad = F * innerpad
    padding = F * padding
    smallimage = crop(image)
    rSize = I - 2*(border + innerpad + padding) 
    rSize = np.float32(rSize)
  
    smallimage = cv2.resize(smallimage,(rSize,rSize))
    border = np.float32(border)
    innerpad = np.float32(innerpad)
    padding = np.float32(padding)
    borderType = cv2.BORDER_CONSTANT
    framed = cv2.copyMakeBorder(smallimage,innerpad,innerpad,innerpad,innerpad, borderType,value = TRANSPARENT)
    if radius > 0.0:

        framed = cv2.copyMakeBorder(framed,border,border,border,border,borderType,value = TRANSPARENT)
        fiducial = makeCorner(F,radius,border)
        x,y,z =  fiducial.shape
        framed = cv2.resize(framed,(x,y))
        framed += fiducial
    else:
        framed = cv2.copyMakeBorder(framed,border,border,border,border,borderType,value = BLACK)
    framed = cv2.copyMakeBorder(framed,padding,padding,padding,padding, borderType,value = TRANSPARENT)
    framed = cv2.resize(framed,(outSize,outSize))
   
    return framed
def makeCornerW(F,radiusRat,border):
    radius = radiusRat * F
    img = np.zeros((F,F,4))

    radius = np.uint32(radius)

    circ = drawCircle(radius,border,TRANSPARENT,BLACK)

    circ = crop(circ)
    circ = invert(circ)
    if radiusRat >.49:
        return circ
    mid = circ.shape[0]/2
    half = circ.shape[1]/2

    ctl = circ[:mid,:half]
    cbl = circ[-mid:,:half]
    ctr = circ[:mid,-half:]
    cbr = circ[-mid:,-half:]
    img[:len(ctl),:len(ctl[0])] = ctl
    img[-len(cbl):,:len(cbl[0])] = cbl
    img[:len(ctr),-len(ctr[0]):] = ctr
    img[-len(cbr):,-len(cbr[0]):] = cbr

    border = np.uint32(border)
    cv2.rectangle(img,(len(ctl),0),(F-len(ctr),border),BORDERWHITE,-1)
    cv2.rectangle(img,(0,len(ctl)),(border,F-len(ctl)),BORDERWHITE,-1)
    cv2.rectangle(img,(len(ctl),F),(F-len(ctl),F-border),BORDERWHITE,-1)
    cv2.rectangle(img,(F,len(ctl)),(F-border,F-len(ctl)),BORDERWHITE,-1)
    cv2.imwrite('testfile/circlecorners.png',img)
    return img
def invert(img):
    a = cv2.split(img)
    alphaImage = cv2.merge([a[-1],a[-1],a[-1],a[0]])
    return alphaImage
def makeCorner3(F,radiusRat,border):
    radius = radiusRat * F
    img = np.zeros((F,F,3))
   
    radius = np.uint32(radius)
    circ = drawCircle(radius,border,WHITE3,BLACK3)

    circ = np.uint8(circ)
   
    cv2.imwrite('testfile/3uncroppedcircle.png',circ)
    circ = crop(circ)
    cv2.imwrite('testfile/3circle.png',circ)
    circ = cv2.bitwise_not(circ)
    cv2.imwrite('testfile/circ3.png',circ)

    if radiusRat >.49:
        return circ
   
    mid = circ.shape[0]/2
    half = circ.shape[1]/2
    
    ctl = circ[:mid,:half]
    cbl = circ[-mid:,:half]
    ctr = circ[:mid,-half:]
    cbr = circ[-mid:,-half:]
    border = np.uint32(border)
    img[:len(ctl),:len(ctl[0])] = ctl
    img[-len(cbl):,:len(cbl[0])] = cbl
    img[:len(ctr),-len(ctr[0]):] = ctr
    img[-len(cbr):,-len(cbr[0]):] = cbr

    cv2.rectangle(img,(len(ctl[0]),0),(F-len(ctl[0]),border),WHITE3,-1)
    cv2.rectangle(img,(0,len(ctl[0])),(border,F-len(ctl[0])),WHITE3,-1)
    cv2.rectangle(img,(len(ctl[0]),F),(F-len(ctl[0]),F-border),WHITE3,-1)
    cv2.rectangle(img,(F,len(ctl[0])),(F-border,F-len(ctl[0])),WHITE3,-1)


    return img

def makeCorner(F,radiusRat,border):
    radius = radiusRat * F
    img = np.zeros((F,F,4))

    circ = drawCircle(radius,border,TRANSPARENT,BLACK)

    circ = crop(circ)

    if radiusRat >.49:
        return circ
    mid = circ.shape[0]/2
    half = circ.shape[1]/2
   
    ctl = circ[:mid,:half]
    cbl = circ[-mid:,:half]
    ctr = circ[:mid,-half:]
    cbr = circ[-mid:,-half:]
   

    img[:len(ctl),:len(ctl[0])] = ctl
    img[-len(cbl):,:len(cbl[0])] = cbl
    img[:len(ctr),-len(ctr[0]):] = ctr
    img[-len(cbr):,-len(cbr[0]):] = cbr

    border = np.uint32(border)
    cv2.rectangle(img,(len(ctl),0),(F-len(ctr),border-1),BLACK,-1)
    cv2.rectangle(img,(0,len(ctl)),(border-1,F-len(ctl)),BLACK,-1)
    cv2.rectangle(img,(len(ctl),F),(F-len(ctl),F-border),BLACK,-1)
    cv2.rectangle(img,(F,len(ctl)),(F-border,F-len(ctl)),BLACK,-1)

    return img

def findF(I,padding):
    if padding == 0:
        return I
    else:
        d  =1+( padding*2)
        I  =np.float32(I)
        F =  np.float32(I/d)

        F = np.uint32(F)

        return F

def crop(image):
    image = myImage(image,'shrink')
  
    size = image.binImage.shape[0]
    vert = image.binImage.cumsum(0)
    hor  = image.binImage.cumsum(1)

    min = size
    for i in range(size):
        top = hor[i][-1]
        bott = hor[-i][-1]
        left = vert[-1][i]
        right = vert[-1][-i]
        sum = top + bott + left + right
        if sum > 0:

            min = i
            break
    if min ==0:
        min = 1
    smallImage = image.image[min:-min,min:-min]


    return smallImage

def drawCircle(radius,thickness,fillColor,circleColor):

    size = 2*radius + 2*thickness

    circle = np.zeros((size,size,len(fillColor)))
    Xo,Yo = len(circle)/2, len(circle[0])/2
    center = Xo,Yo
    for x in range(len(circle)):
        for y in range(len(circle[0])):
            a =abs(Xo - x)
            b =abs(Yo - y)
            d = math.hypot(a,b)
            if d <= (radius) and d>=(radius-thickness):
                circle[x][y] = circleColor
            else:
                circle[x][y] = fillColor


    return circle
def makeImglist(path,namelist,image_size):
    img_list = []
    for im in namelist:
        img_list.append(myImage(cv2.resize(cv2.imread(os.path.join(path,im),-1),(image_size,image_size)), im))
    return img_list

def typefilter(name_list,Ftype = '.png'):
    index = 0
    while index < len(name_list):
        if name_list[index].find(Ftype) == -1:
            name_list.pop(index)
        else:
            index += 1
    return name_list


class myImage():
    def __init__(self,image,name):
        self.chan = len(image[0][0])
        self.image = image
        self.name = name
        self.binImage = self.binim()
    def binim(self):
        imarray = cv2.split( self.image)
        if len(imarray) == 4:
            if imarray[-1].mean() >225:

                return cv2.bitwise_not(imarray[0])
            else:
 
                return imarray[-1]
        else:
            return cv2.bitwise_not(imarray[0])

    


if len(sys.argv) > 7:
    main(str(sys.argv[1]), str(sys.argv[2]), np.uint32(sys.argv[3]), np.float32(sys.argv[4]), np.float32(sys.argv[5]),np.float32(sys.argv[6]),np.float32(sys.argv[7]))
elif len(sys.argv) > 6:
    main(str(sys.argv[1]), str(sys.argv[2]), np.uint32(sys.argv[3]), np.float32(sys.argv[4]), np.float32(sys.argv[5]),np.float32(sys.argv[6]))
elif len(sys.argv) > 5:
    main(str(sys.argv[1]), str(sys.argv[2]), np.uint32(sys.argv[3]), np.float32(sys.argv[4]), np.float32(sys.argv[5]))
elif len(sys.argv) > 4:
    main(str(sys.argv[1]), str(sys.argv[2]), np.uint32(sys.argv[3]), np.float32(sys.argv[4]))
elif len(sys.argv) > 3:
    main(str(sys.argv[1]), str(sys.argv[2]),np.uint32(sys.argv[3]))
elif len(sys.argv) > 2:
    main(str(sys.argv[1]), str(sys.argv[2]))
elif len(sys.argv) > 1:
    main(str(sys.argv[1]))
else:
    print 'please enter a source path and a destination path'

# main(src = 'gallery/',dst = 'gallery/withFiducials/',size=512, padding = 0.1,radius= 0.2, border = .1, innerpad = .1):
