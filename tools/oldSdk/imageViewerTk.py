#!/usr/bin/env python3

"""
Tkinter implementation of IImageViewer
Consumes and displays images from imageManager.py
"""
__author__ = "Mark Wesley"


from iImageViewer import IImageViewer
import tkinter
from tkinter import *
from PIL import ImageTk
from PIL import Image, ImageDraw
import threading


class ImageViewerTk(IImageViewer, Frame):

    def __init__(self, tkRoot=tkinter.Tk(), imageScale = 2, windowName = "CozmoView", forceOnTop=True):

        IImageViewer.__init__(self)
        Frame.__init__(self, tkRoot)

        self.scale = imageScale
        tkRoot.wm_title(windowName)

        self.display = Canvas(self)

        self.tkRoot = tkRoot
        self.label  = tkinter.Label(self.tkRoot,image=None)
        self.queuedImage = None
        self.lock   = threading.Lock()

        self.tkRoot.protocol("WM_DELETE_WINDOW", self._DeleteWindow)
        self._isRunning = True

        if forceOnTop:
            # force window on top of all others, regardless of focus
            tkRoot.wm_attributes("-topmost", 1)

        self._RepeatDrawFrame()

    def _DeleteWindow(self):
        print("_DeleteWindow")
        self.tkRoot.destroy()
        self.quit()
        self._isRunning = False
        sys.exit()

    def _DrawFrame(self):
        nextImage = self.PopImage()

        if nextImage == None:
            return None

        if self.scale != 1:
            width  = nextImage.width
            height = nextImage.height

            nextImage = nextImage.resize((self.scale * width, self.scale * height), Image.ANTIALIAS)

        photoImage = ImageTk.PhotoImage(nextImage)

        self.label.configure(image=photoImage)
        self.label.image = photoImage
        self.label.pack()

    def _RepeatDrawFrame(self, event=None):
        self._DrawFrame()
        timeBetweenFrames_ms = 10
        self.display.after(timeBetweenFrames_ms, self._RepeatDrawFrame)

    def DisplayImage(self, nextImage):
        return self.QueueImage(nextImage)

    def QueueImage(self, nextImage):
        "Thread safe queing of next image to display (clobbers any pending queuedImage - we only need the latest one)"
        self.lock.acquire()
        self.queuedImage = nextImage
        self.lock.release()

    def PopImage(self):
        "Thread safe pop of most recently queued image"
        self.lock.acquire()
        poppedImage = self.queuedImage
        self.queuedImage = None
        self.lock.release()
        return poppedImage

