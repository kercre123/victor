
import sys
import string
from PIL import Image, ImageFont, ImageDraw, ImageColor

DEBUG=0

name = "Huge"

if name == "Medium":
    height = 12
    width = 9
    pt = 17
    startrow = 5
    first = ord(' ')
    last = ord('~')
    font = ImageFont.truetype("Inconsolata-Bold.ttf", pt)
    ctype = "uint16_t"
    formatstr = "0x{:04x},"


elif name == "Huge":
    height = 32
    width = 24
    pt = 38
    startrow = 7
    first = ord(' ')
    last = ord('~')
    ctype = "uint32_t"
    formatstr = "0x{:08x},"
    font = ImageFont.truetype("Menlo.ttc", pt, 1) #bold

else:
    print("Invalid font name;")
    sys.exit(-1)




NCHARS_PER_LINE = 4
nchars = 0


print( "#define {}_FONT_WIDTH {}".format(name.upper(), width))
print( "#define {}_FONT_HEIGHT {}".format(name.upper(), height))
print( "#define {}_FONT_CHAR_START {}".format(name.upper(), first))
print( "#define {}_FONT_CHAR_END {}".format(name.upper(), last))
print("\n")
print("static const {} g{}FontGlyphs[] = {{".format(ctype, name))
def RenderChar(ch):
    global nchars
    # use a truetype font
    image = Image.new("1", (width,height*2), "black")
    draw = ImageDraw.Draw(image)

    draw.text((0, 0), ch, font=font, fill="white")

    data = [v for v in list(image.getdata())]

    sr = startrow +1 if ch in ["{","}","Q"] else startrow

    bbox = image.getbbox() or (1,2,3,4)
#    print(ch, bbox,width, bbox[3]-bbox[1])
    if (bbox[3]-sr>height):
        print("/*Font too tall*/")
#        sys.exit(-1)
    if (bbox[2]>=width):
        print("/*Image must be wider than {}*/".format(width))
#        sys.exit(-1)

    if DEBUG:
        print(ch)
        for i in range(height):
            l = (i+sr)*width
            print("".join(["X" if v else " " for v in  data[l:l+width]]))
        print(" ")
    else:
        print("\t",end="")
        for col in range(width):
            bitmap = 0
            for row in range(height-1):
                xy = (col,height-2-row+startrow)
                #            print(xy)
                bit = 1 if image.getpixel(xy) else 0
                bitmap = (bitmap*2) + bit
            print(formatstr.format(bitmap),end='');
            if col==9:
                print("\n\t",end="");
        print(" /* {} */ ".format(ch))
#    print(" ", end="")
    # nchars+=1
    # if (nchars%NCHARS_PER_LINE==0):
    #     print("\n")




#image.save("sample.bmp")

#for c in string.printable:
for i in range(first,last+1):
    c = chr(i)
    RenderChar(c)
RenderChar(' ')

print("};")
