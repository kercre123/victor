#!/usr/bin/env python3
import time
from cozmoInterface import CozmoInterface
useOpenCv = False
if useOpenCv:
    from imageViewerCv import ImageViewerCv
else:
    from imageViewerTk import ImageViewerTk
from threading import Thread
from time import sleep

def PilTest(imageViewer):

    cozmo = CozmoInterface()

    while True:
        state = cozmo.GetState()

        imageViewer.DisplayImage( state.GetImage(drawViz=True) )
        sleep(0.1)

    cozmo.Shutdown()


if __name__ == '__main__':
    if useOpenCv:
        imageViewer = ImageViewerCv()
        PilTest(imageViewer)
    else:
        imageViewer = ImageViewerTk()
        thread = Thread(target=PilTest, kwargs=dict(imageViewer=imageViewer))
        thread.daemon = True  # Force to quit on main quitting
        thread.start()
        imageViewer.mainloop()
        thread.join()


