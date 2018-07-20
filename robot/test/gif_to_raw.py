
import os,sys
#import struct
import array
from PIL import Image
#import Image

SCREEN_WIDTH,SCREEN_HEIGHT = 184,96 #240,240 #180,240
SIZE = (SCREEN_WIDTH,SCREEN_HEIGHT)

def pack16bitRGB(pixel):
#    print(pixel)
    try:
        r,g,b,a = pixel
    except (ValueError,TypeError):
        try:
            a = 0
            r,g,b = pixel
        except (ValueError,TypeError):
            r = g = b = pixel
    #    print(r,g,b,a,"\n")
    word = ( (int(r>>3)<<11) |
             (int(g>>2)<< 5) |
             (int(b>>3)<< 0) )
    return word
    # return ((word>>8)&0xFF) | ((word&0xFF)<<8)


def convert_to_raw(img):
    bitmap = [0x0000]*(SCREEN_WIDTH*SCREEN_HEIGHT)
    for y in range(img.size[1]):
        for x in range(img.size[0]):
            pixel = pack16bitRGB(img.getpixel((x,y)))
            bitmap[(y)*SCREEN_WIDTH + (x)] = pixel
    return bitmap


RAW = 1

def extractFrames(inGif):


    frame = Image.open(inGif)
    nframes = 0
    with open('%s.raw' % (os.path.basename(inGif),), "wb+") as f:
        while frame:
#            newframe = frame.rotate(90).resize( SIZE, Image.ANTIALIAS).convert('RGBA')
            newframe = frame.convert('RGBA')
            newframe = convert_to_raw(newframe)
            data = array.array("H",newframe)
            f.write(data.tostring())
            nframes += 1
            try:
                frame.seek( nframes )
            except EOFError:
                break;
    return True


extractFrames(sys.argv[1])
