#!/usr/bin/env python3

"""
Flask implementation of IImageViewer (serving images to a webpage)
"""
__author__ = "Mark Wesley"


from iImageViewer import IImageViewer
try:
    from flask import Flask, send_file, make_response
except ImportError:
    print("flaskTest.py: Cannot import from Flask: Do 'pip3 install flask' to install")
    raise

from io import BytesIO              # for serving images
from PIL import Image, ImageDraw
from datetime import datetime
import logging                      # so we can mute Flask
import threading
import webbrowser
from time import sleep

gFlaskApp = Flask(__name__)
gFlaskImageViewer = None

def SleepAndOpenWebBrowser(url, new=0, autoraise=True, delay=0.1):
    sleep(delay)
    webbrowser.open(url, new=new, autoraise=autoraise)

class ImageViewerFlask(IImageViewer):

    def __init__(self, imageScale = 2):
        IImageViewer.__init__(self)
        self.scale = imageScale
        # self.windowName = windowName
        self.queuedImage = None
        self.hasNewImage = False
        self.lock   = threading.Lock()
        # self._isRunning = True
        global gFlaskImageViewer
        gFlaskImageViewer = self

    def _DelayedOpenBrowser(self, url, new=0, autoraise=True, delay=0.1):
        "Kick off a thread to sleep and then open the browser - allows flask chance to be running before page requests data"
        thread = threading.Thread(target=SleepAndOpenWebBrowser, kwargs=dict(url=url, new=new, autoraise=autoraise, delay=delay))
        thread.daemon = True  # Force to quit on main quitting
        thread.start()

    def RunFlask(self, hostIp="127.0.0.1", hostPort=5000, enableFlaskLogging=False, openPage=True):
        # global gFlaskApp
        # if gFlaskApp is None:
        #     gFlaskApp = Flask(__name__)
        if enableFlaskLogging:
            # it's enabled by default, and we don't support re-running
            pass
        else:
            # gFlaskApplogger.disabled = True
            # log = cf.logging.getLogger('werkzeug')
            log = logging.getLogger('werkzeug')
            log.setLevel(logging.ERROR)
            # log.disabled = True

        # ideally webbrowser doesn't try to open for a few milliseconds to ensure that flask is running...
        self._DelayedOpenBrowser("http://" + hostIp + ":" + str(hostPort), delay=5.0)
        gFlaskApp.run(host=hostIp, port=hostPort, use_evalex=False)



    def __del__(self):
        global gFlaskImageViewer
        if gFlaskImageViewer == self:
            gFlaskImageViewer = None

    def DisplayImage(self, nextImage):
        return self.QueueImage(nextImage)

    def QueueImage(self, nextImage):
        "Thread safe queing of next image to display (clobbers any pending queuedImage - we only need the latest one)"
        self.lock.acquire()
        self.queuedImage = nextImage
        # if self.hasNewImage:
        #     print("Skipping")
        self.hasNewImage = (nextImage is not None)
        self.lock.release()

    def GetLatestImage(self):
        "Thread safe pop of most recently queued image"
        self.lock.acquire()
        latestImage = self.queuedImage
        # if not self.hasNewImage:
        #     print("Reusing")
        self.hasNewImage = False
        self.lock.release()
        return latestImage


def make_uncached_response(inFile):
    response = make_response(inFile)
    response.headers['Pragma-Directive'] = 'no-cache'
    response.headers['Cache-Directive'] = 'no-cache'
    response.headers['Cache-Control'] = 'no-cache, no-store, must-revalidate'
    response.headers['Pragma'] = 'no-cache'
    response.headers['Expires'] = '0'
    return response


def serve_pil_image(pil_img):
    sendAsJPEG = False
    img_io = BytesIO()

    # draw = ImageDraw.Draw(pil_img)
    # draw.text((0, 0), datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f"))
    # pil_img.show()

    if sendAsJPEG:
        pil_img.save(img_io, 'JPEG', quality=70)
        img_io.seek(0)
        return make_uncached_response( send_file(img_io, mimetype='image/jpeg') )
    else:
        pil_img.save(img_io, 'PNG')
        img_io.seek(0)
        return make_uncached_response( send_file(img_io, mimetype='image/png') )


@gFlaskApp.route("/")
def handleIndexPage():
    return '''
    <html>
      <head>
        <title>imageViewerFlask.py Display</title>
      </head>
      <body>
        <p>Cozmo View</p>
        <script type="text/javascript">
            setInterval(
                function()
                {
                    // Note: Firefox ignores the no_store and still caches, only fix is to add a "?UID" suffix to fool it
                    document.getElementById("cozmoImageId").src="cozmoImage?" + (new Date()).getTime();
                    //document.getElementById("cozmoImageId").src="cozmoImage";
                },
            90); // repeat every X ms
        </script>
        <img src="cozmoImage" id="cozmoImageId">
      </body>
    </html>
    '''


def CreateDefaultImage():
    kImageWidth = 320
    kImageHeight = 240
    imageBytes = bytearray([0x00, 0x00, 0x00]) * kImageWidth * kImageHeight

    pI = 0
    for y in range(kImageHeight):
        for x in range(kImageWidth):

            imageBytes[pI]   = int(255.0 * (x / kImageWidth))
            imageBytes[pI+1] = int(255.0 * (y / kImageHeight))
            imageBytes[pI+2] = 0
            pI += 3

    image = Image.frombytes('RGB', (kImageWidth, kImageHeight), bytes(imageBytes))
    return image


gDefaultImage = None


def GetDefaultImage():
    global gDefaultImage
    if gDefaultImage is None:
        gDefaultImage = CreateDefaultImage()
    return gDefaultImage


@gFlaskApp.route("/cozmoImage")
def handleCozmoImage():
    global gFlaskImageViewer
    if gFlaskImageViewer is not None:
        latestImage = gFlaskImageViewer.GetLatestImage()
        if latestImage is not None:
            return serve_pil_image(latestImage)
    # no image!
    return serve_pil_image( GetDefaultImage() )





