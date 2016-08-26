#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
useFlaskForImages = True
if useFlaskForImages:
    from imageViewerFlask import ImageViewerFlask
else:
    from imageViewerTk import ImageViewerTk
from threading import Thread
from time import sleep

'''This is a simple script where cozmo will track the first face it sees and make sure to keep
   it within his field of vision. Works when you're not moving too fast. 
   He will not do anything in response to other faces.'''
        
def FaceTrackingDemo(imageViewer):
    cozmo = CozmoInterface()

    cozmo.WaitUntilSeeFaces(1, 0)

    while True:
        state = cozmo.GetState()
        firstFace = state.GetFace(0)
        cozmo.TurnTowardsFace(firstFace.faceID)

        imageViewer.DisplayImage( state.GetImage(drawViz=True) )
        sleep(0.1)

    cozmo.Shutdown()

if __name__ == '__main__':
    if useFlaskForImages:
        imageViewer = ImageViewerFlask()
    else:
        imageViewer = ImageViewerTk()
    thread = Thread(target=FaceTrackingDemo, kwargs=dict(imageViewer=imageViewer))
    thread.daemon = True  # Force to quit on main quitting
    thread.start()
    if useFlaskForImages:
        imageViewer.RunFlask()
    else:
        imageViewer.mainloop()
    thread.join()


