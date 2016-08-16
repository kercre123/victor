"""
Image Manger to store current/past images from the robot
"""
__author__ = "Alec Solder"

import io, sys, os

hasImageDecoding = True

try:
  import numpy as np
except ImportError:
  print("imageManager.py: Cannot import numpy: cannot decode or use Images. Do `pip3 install numpy` to install")
  hasImageDecoding = False

try:
  from PIL import Image, ImageDraw
except ImportError:
  print("imageManager.py: Cannot import from PIL: cannot decode or use Images. Do `pip3 install pillow` to install")
  hasImageDecoding = False

# ================================================================================    
# Image Manager
# ================================================================================
class ImageManager:
    "ImageManger"

    def __init__(self, EToG, GToE):
        self.EToG = EToG
        self.GToE = GToE
        self.images = []
        self.incompleteImageMsgs = []
        self.incompleteImages = [[None]*10] * 10
        self.currentImageId = 0

    def ImageChunk(self, msg):
        "Gets called when receiving an imageChunk Message, dechunks the images and adds to self.images"
        # The destination for the chunk is in self.incompleteImages, once it is done building
        # it is transfered into self.images for possible reading. We never want an incomplete image
        # to be accidentally read
        for i in range(len(self.incompleteImageMsgs)):

            # Try to add to the image being built (if it exists)
            if self.incompleteImageMsgs[i].imageId != msg.imageId:
                continue
           
            self.incompleteImages[i][msg.chunkId] = msg.data

            # The image is now complete (since the messages all come in order)
            if (msg.chunkId + 1) == msg.imageChunkCount:
                self.incompleteImages[i] = self.incompleteImages[i][:msg.imageChunkCount]
                # Flatten the list and append it to the start of self.images
                for elem in self.incompleteImages[i]:
                    if elem == None:
                        # In this case, the picture isn't full for some reason, so just quit out before reporting
                        return
                self.images = [[pix for block in self.incompleteImages[i] for pix in block]] + self.images
                self.images = self.images[:10]
            return

        # For the real robot, msg.imageChunkCount is only nonzero on the final image
        # So we have to hard code it for now if we want to assume out of order receiving
        self.incompleteImages = [([msg.data] + ([None] * 9))] + self.incompleteImages
        self.incompleteImages = self.incompleteImages[:10]

        self.incompleteImageMsgs = [msg] + self.incompleteImageMsgs
        self.incompleteImageMsgs = self.incompleteImageMsgs[:10]

    def GetImage(self, index = 1):
        "Returns the final image in decoded jpeg form (in a numpy array)"
        # Images coming from the robot are of type JPEGMinimizedGray
        # Images coming from webots simulated robot are of type JPEGColor
        # Standard for a QVGA image

        if not hasImageDecoding:
          sys.stderr.write("imageManager.py: image decoding disabled - GetImage() unavailable" + os.linesep)
          return None

        width = 320
        height = 240
        if len(self.images) > 1:
            arr = np.asarray(self.images[index], dtype=np.uint8)
            if self.incompleteImageMsgs[0].imageEncoding == self.EToG.ImageEncoding.JPEGMinimizedGray:
                arr = self._MiniGrayToJpeg(arr, len(arr), height, width)
            image = Image.open(io.BytesIO(arr))
            image = image.convert("RGB")
            return image

    def DrawRectangle(self, image, x1, y1, x2, y2, fill = None, outline = None):
        if image is None:
            return None
        # Colors are BGR
        # scaled variables
        draw = ImageDraw.Draw(image)
        draw.rectangle([(x1, y1), (x2, y2)], fill = fill, outline = outline)
        del draw

    def DrawText(self, image, x, y, text, fill = None):
        if image is None:
            return None
        draw = ImageDraw.Draw(image)
        draw.text((x,y), text, fill = fill)
        del draw

    def _MiniGrayToJpeg(self, arrIn, bufferLength,  height,  width):
        "Converts miniGrayToJpeg format to normal jpeg format"
        # Does not work correctly yet
        bufferIn = arrIn.tolist()
        currLen = len(bufferIn)
        #This should be 'exactly' what is done in the miniGrayToJpeg function in ImageDeChunker.cpp
        header50 = np.array([
              0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
              0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, #// 0x19 = QTable
              0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23,
              0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40,
              0x44, 0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D,
              
              #//0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0xF0, #// 0x5E = Height x Width
              0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x01, 0x28, #// 0x5E = Height x Width
              
              #//0x01, 0x40, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
              0x01, 0x90, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
              
              0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
              0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
              0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
              0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
              0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
              0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
              0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
              0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
              0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
              0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
              0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
              0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
              0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01,
              0x00, 0x00, 0x3F, 0x00
            ], dtype=np.uint8)
        headerLength = len(header50)
        # For worst case expansion 
        bufferOut = np.array([0] * (currLen*2 + headerLength), dtype=np.uint8)

        for i in range(headerLength):
            bufferOut[i] = header50[i]

        bufferOut[0x5e] = height >> 8
        bufferOut[0x5f] = height & 0xff
        bufferOut[0x60] = width  >> 8
        bufferOut[0x61] = width  & 0xff
        # Remove padding at the end
        while (bufferIn[currLen-1] == 0xff):
            currLen -= 1
            
        off = headerLength
        for i in range(currLen-1):
            bufferOut[off] = bufferIn[i+1]
            off += 1
            if (bufferIn[i+1] == 0xff):
                bufferOut[off] = 0
                off += 1

        bufferOut[off] = 0xff
        off += 1
        bufferOut[off] = 0xD9

        bufferOut[:off]
        return np.asarray(bufferOut)
