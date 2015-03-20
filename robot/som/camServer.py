"""
Camera frame server for Torpedo System on Module
"""
__author__  = "Daniel Casner"
__version__ = "0.0.1"


import sys, socket, select, time, math, subprocess, signal
from subserver import *
import messages

MTU = 65535

class CameraSubServer(BaseSubServer):
    "imageChunk server base class"

    RESOLUTION_TUPLES = { # Actually lists so we can contactinate
        messages.CAMERA_RES_QUXGA: [3200, 2400],
        messages.CAMERA_RES_QXGA: [2048, 1536],
        messages.CAMERA_RES_UXGA: [1600, 1200],
        messages.CAMERA_RES_SXGA: [1280, 960],
        messages.CAMERA_RES_XGA: [1024, 768],
        messages.CAMERA_RES_SVGA: [800, 600],
        messages.CAMERA_RES_VGA: [640, 480],
        messages.CAMERA_RES_QVGA: [320, 240],
        messages.CAMERA_RES_QQVGA: [160, 120],
        messages.CAMERA_RES_QQQVGA: [80, 60],
        messages.CAMERA_RES_QQQQVGA: [40, 30],
        messages.CAMERA_RES_NONE: [0, 0]
    }

    @classmethod
    def FPS2INTERVAL(cls, fps):
        return (fps*1000, 1000000)

    SENSOR_RESOLUTION = messages.CAMERA_RES_SVGA
    SENSOR_RES_TPL = RESOLUTION_TUPLES[SENSOR_RESOLUTION]
    SENSOR_FPS     = 15

    ISP_MIN_RESOLUTION = messages.CAMERA_RES_QVGA
    CROP_THRESHOLD = messages.CAMERA_RES_VGA

    ENCODER_SOCK_HOSTNAME = '127.0.0.1'
    ENCODER_SOCK_PORT     = 6000
    ENCODER_CODING        = messages.IE_JPEG_COLOR
    ENCODER_QUALITY       = 70

    ENCODER_LATEANCY = 5 # ms, SWAG

    def __init__(self, server, verbose=False, test_framerate=False):
        "Initalize server for specified camera on given port"
        BaseSubServer.__init__(self, server, verbose)
        self.tfr = test_framerate
        if self.tfr:
            sys.stdout.write("Will print frame rate information\n")

        subprocess.call(['ifconfig', 'lo', 'up']) # Bring up the loopback interface if it isn't already

        # Setup local jpeg data receive socket
        self.encoderLock = threading.Lock()
        self.encoderProcess = None
        self.encoderSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.encoderSocket.bind((self.ENCODER_SOCK_HOSTNAME, self.ENCODER_SOCK_PORT))
        self.encoderSocket.setblocking(0)

        hostname = socket.gethostname()
        assert hostname.startswith('cozmo'), "Hostname must be of the format cozmo#"
        if hostname == 'cozmo':
            self.camDev = "mt9p031"
            self.camInt = ""
        else:
            self.camDev = "ov2686"
            self.camInt = " @ %d/%d" % self.FPS2INTERVAL(self.SENSOR_FPS)

        assert subprocess.call(['media-ctl', '-v', '-r', '-l', '"%s":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]' % self.camDev]) == 0, "media-ctl ISP links setup failure"

        # ISP resizer resolution
        self.resolution = messages.CAMERA_RES_NONE

        # Setup video data chunking state
        self.imageNumber    = 0
        self.sendMode       = messages.ISM_OFF
        self.sendModeLock   = threading.Lock()

        self.lastFrameOTime = 0
        self.lastFrameITime = 0

    def stop(self):
        sys.stdout.write("Closing camServer\n")
        BaseSubServer.stop(self)
        self.stopEncoder(False)
        self.encoderSocket.close()
        sys.stdout.flush()

    def setResolution(self, resolutionEnum):
        "Adjust the camera resolution if nessisary and (re)start the encoder"
        # Camera resolution enum is backwards, smaller number -> larger image
        resolutionEnum = max(resolutionEnum, self.SENSOR_RESOLUTION) # Can't provide image larger than sensor
        if resolutionEnum == self.resolution: # Already configured
            self.startEncoder()
            return True
        else:
            self.stopEncoder()
            if subprocess.call(['media-ctl', '-v', '-f', '"%s":0 [SBGGR12 %dx%d%s], "OMAP3 ISP CCDC":2 [SBGGR10 %dx%d], "OMAP3 ISP preview":1 [UYVY %dx%d], "OMAP3 ISP resizer":1 [UYVY %dx%d]' % \
                                tuple([self.camDev] + self.SENSOR_RES_TPL + [self.camInt] + (self.SENSOR_RES_TPL * 2) + self.RESOLUTION_TUPLES[min(resolutionEnum, self.CROP_THRESHOLD)])]) == 0:
                self.resolution = resolutionEnum
                self.startEncoder()
                return True
            else:
                return False


    def startEncoder(self):
        "Starts the encoder subprocess"
        self.encoderLock.acquire()
        if self.encoderProcess is None or self.encoderProcess.poll() is not None:
            if self.ENCODER_CODING == messages.IE_JPEG_COLOR:
                sys.stdout.write("Starting the encoder\n")
                encoderCall = ['nice', '-n', '-10', 'gst-launch', 'v4l2src', 'device=/dev/video6', '!']
                if self.resolution > self.CROP_THRESHOLD: # Cropping to QVGA
                    h = (self.RESOLUTION_TUPLES[self.CROP_THRESHOLD][0] - self.RESOLUTION_TUPLES[self.resolution][0])/2
                    v = (self.RESOLUTION_TUPLES[self.CROP_THRESHOLD][1] - self.RESOLUTION_TUPLES[self.resolution][1])/2
                    encoderCall.extend(['videocrop', 'left=%d' % h, 'top=%d' % v, 'right=%d' % h, 'bottom=%d' % v, '!'])
                encoderCall.extend(['TIImgenc1', 'engineName=codecServer', 'iColorSpace=UYVY', 'oColorSpace=YUV420P', \
                                    'qValue=%d' % self.ENCODER_QUALITY, 'numOutputBufs=2', '!', \
                                    'udpsink', 'host=%s' % self.ENCODER_SOCK_HOSTNAME, 'port=%d' % self.ENCODER_SOCK_PORT])
                self.encoderProcess = subprocess.Popen(encoderCall)
            else:
                raise ValueError("Unsupported encoder coding specified")
        self.encoderLock.release()

    def stopEncoder(self, blocking=True):
        "Stop the encoder subprocess if it is running"
        haveLock = self.encoderLock.acquire(blocking)
        if self.encoderProcess is not None:
            sys.stdout.write("Stopping the encoder\n")
            if self.encoderProcess.poll() is None:
                self.encoderProcess.send_signal(signal.SIGINT)
                self.encoderProcess.wait()
            self.encoderProcess = None
        if haveLock:
            self.encoderLock.release()
        return haveLock

    def giveMessage(self, message):
        "Process a message recieved by the server"
        if ord(message[0]) == messages.ImageRequest.ID:
            inMsg = messages.ImageRequest(message)
            sys.stdout.write("New image request: %s\n" % str(inMsg))
            self.sendModeLock.acquire()
            self.sendMode = inMsg.imageSendMode
            if inMsg.imageSendMode == messages.ISM_OFF:
                self.stopEncoder()
            else: # Not off, set resolution and start encoder
                self.setResolution(inMsg.resolution)
            self.sendModeLock.release()

    def standby(self):
        "Put the sub server into standby mode"
        self.sendModeLock.acquire()
        self.stopEncoder()
        self.sendMode = messages.ISM_OFF
        self.sendModeLock.release()

    def step(self):
        "A single execution step for this thread"
        if self.encoderProcess is not None and self.encoderProcess.poll() is not None:
            sys.stderr.write("Encoder sub-process has terminated %d\n" % self.encoderProcess.poll())
            self.encoderProcess = None
            return
        rfds, wfds, efds = select.select([self.encoderSocket], [], [self.encoderSocket], 1.0)
        if self.encoderSocket in efds:
            if self._continue == False:
                return
            else:
                sys.stderr.write("camServer.encoderSocket in exceptional condition\n")
        if self.encoderSocket not in rfds: # Timeout
            return
        frame = None
        try:
            while True: # Receive all the data grams and throw out all but the last one
                frame = self.encoderSocket.recv(MTU)
        except socket.error, e:
            if e.errno == 11: # No more data, this is how we expect to exit the loop
                if self.tfr:
                    tick = time.time()
                    sys.stdout.write('FP: %f ms\n' % ((tick - self.lastFrameOTime)*1000))
                    self.lastFrameOTime = tick
                # If only sending single image
                self.sendModeLock.acquire()
                if self.sendMode != messages.ISM_OFF:
                    self.imageNumber += 1
                    msg = messages.ImageChunk()
                    msg.imageId = self.imageNumber
                    msg.imageTimestamp = self.server.timestamp.get()
                    msg.imageEncoding = self.ENCODER_CODING
                    msg.imageChunkCount = int(math.ceil(float(len(frame)) / messages.ImageChunk.IMAGE_CHUNK_SIZE))
                    msg.resolution = self.resolution
                    chunkNumber = 0
                    while frame:
                        frame = msg.takeChunk(frame)
                        msg.chunkId = chunkNumber
                        chunkNumber += 1
                        self.clientSend(msg.serialize())
                        time.sleep(0.001) # Allow a little time for the UDP socket to process
                    if self.sendMode == messages.ISM_SINGLE_SHOT:
                        self.sendMode = messages.ISM_OFF
                self.sendModeLock.release()
            elif e.errno == socket.EBADF and self._continue == False:
                return
            else:
                raise e
