"""
Camera frame server for Torpedo System on Module
"""
__author__  = "Daniel Casner"
__version__ = "0.0.1"


import sys, socket, time, math, subprocess, signal
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
        return [fps*1000, 1000000]

    SENSOR_RESOLUTION = messages.CAMERA_RES_SVGA
    SENSOR_RES_TPL = RESOLUTION_TUPLES[SENSOR_RESOLUTION]
    SENSOR_FPS     = 15

    ISP_MIN_RESOLUTION = messages.CAMERA_RES_QVGA

    ENCODER_SOCK_BINDADDR = "~/"
    ENCODER_SOCK_HOSTNAME = '127.0.0.1'
    ENCODER_SOCK_PORT     = 6000
    ENCODER_CODING        = messages.IE_JPEG
    ENCODER_QUALITY       = 97

    ENCODER_LATEANCY = 5 # ms, SWAG

    def __init__(self, server, timeout, verbose=False, test_framerate=False):
        "Initalize server for specified camera on given port"
        BaseSubServer.__init__(self, server, timeout, verbose)
        self.tfr = test_framerate
        if self.tfr:
            sys.stdout.write("Will print frame rate information\n")

        subprocess.call(['ifconfig', 'lo', 'up']) # Bring up the loopback interface if it isn't already

        # Setup local jpeg data receive socket
        self.encoderLock = threading.Lock()
        self.encoderProcess = None
        self.encoderSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.encoderSocket.bind((self.ENCODER_SOCK_HOSTNAME, self.ENCODER_SOCK_PORT))
        self.encoderSocket.settimeout(timeout)

        hostname = socket.gethostname()
        assert hostname.startswith('cozmo'), "Hostname must be of the format cozmo#"
        if hostname == 'cozmo':
            self.camDev = "mt9p031"
        else:
            self.camDev = "ov2686"

        assert subprocess.call(['media-ctl', '-v', '-r', '-l', '"%s":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]' % self.camDev]) == 0, "media-ctl ISP links setup failure"

        # ISP resizer resolution
        self.ISPResolution = messages.CAMERA_RES_NONE

        # Setup video data chunking state
        self.imageNumber    = 0
        self.sendMode       = messages.ISM_OFF

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
        resolutionEnum = min(resolutionEnum, self.ISP_MIN_RESOLUTION) # ISP has minimum resolution
        if resolutionEnum == self.ISPResolution:
            self.startEncoder()
            return True
        else:
            self.stopEncoder()
            if subprocess.call(['media-ctl', '-v', '-f', '"%s":0 [SBGGR12 %dx%d @ %d/%d], "OMAP3 ISP CCDC":2 [SBGGR10 %dx%d], "OMAP3 ISP preview":1 [UYVY %dx%d], "OMAP3 ISP resizer":1 [UYVY %dx%d]' % \
                                tuple([self.camDev] + self.SENSOR_RES_TPL + self.FPS2INTERVAL(self.SENSOR_FPS) + (self.SENSOR_RES_TPL * 2) + \
                                 self.RESOLUTION_TUPLES[resolutionEnum])]) == 0:
                self.ISPResolution = resolutionEnum
                self.startEncoder()
                return True
            else:
                return False


    def startEncoder(self):
        "Starts the encoder subprocess"
        self.encoderLock.acquire()
        if self.encoderProcess is None or self.encoderProcess.poll() is not None:
            if self.ENCODER_CODING == messages.IE_JPEG:
                sys.stdout.write("Starting the encoder\n")
                self.encoderProcess = subprocess.Popen(['nice', '-n', '-10', 'gst-launch', 'v4l2src', 'device=/dev/video6', '!', \
                                                        'TIImgenc1', 'engineName=codecServer', 'iColorSpace=UYVY', 'oColorSpace=YUV420P', 'qValue=%d' % self.ENCODER_QUALITY, 'numOutputBufs=2', '!', \
                                                        'udpsink', 'host=%s' % self.ENCODER_SOCK_HOSTNAME, 'port=%d' % self.ENCODER_SOCK_PORT])
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

    def giveMessage(self, message):
        "Process a message recieved by the server"
        if ord(message[0]) == messages.ImageRequest.ID:
            inMsg = messages.ImageRequest(message)
            sys.stdout.write("New image request: %s\n" % str(inMsg))
            self.sendMode = inMsg.imageSendMode
            if inMsg.imageSendMode == messages.ISM_OFF:
                self.stopEncoder()
            else: # Not off, set resolution and start encoder
                self.setResolution(inMsg.resolution)

    def standby(self):
        "Put the sub server into standby mode"
        self.stopEncoder()
        self.sendMode = messages.ISM_OFF

    def step(self):
        "A single execution step for this thread"
        if self.encoderProcess is not None and self.encoderProcess.poll() is not None:
            raise Exception("Encoder sub-process has terminated")
        try:
            frame = self.encoderSocket.recv(MTU)
        except socket.timeout:
            return # if no frame, skip the rest
        except socket.error, e:
            if e.errno == socket.EBADF and self._continue == False:
                return
            else:
                raise e
        else:
            if self.tfr:
                tick = time.time()
                sys.stdout.write('FP: %f ms\n' % ((tick - self.lastFrameOTime)*1000))
                self.lastFrameOTime = tick
            # If only sending single image
            if self.sendMode == messages.ISM_SINGLE_SHOT:
                self.stopEncoder()
                self.sendMode = messages.ISM_OFF
            self.imageNumber += 1
            msg = messages.ImageChunk()
            msg.imageId = self.imageNumber
            msg.imageTimestamp = self.server.timestamp.get()
            msg.imageEncoding = self.ENCODER_CODING
            msg.imageChunkCount = int(math.ceil(float(len(frame)) / messages.ImageChunk.IMAGE_CHUNK_SIZE))
            msg.resolution = self.ISPResolution
            chunkNumber = 0
            while frame:
                frame = msg.takeChunk(frame)
                msg.chunkId = chunkNumber
                chunkNumber += 1
                self.clientSend(msg.serialize())

class Client(object):
    "Client for UDP camera server for testing"

    def __init__(self, host, port=5551, resolution=messages.CAMERA_RES_SVGA, socketType="UDP"):
        "Connect to server"
        self.resolution = resolution
        if socketType == "UDP":
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.server = (host, port)
        elif socketType == "TCP":
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((host, port))
            self.server = "TCP"
        else:
            raise ValueError("Unsupported socket type")

    def __del__(self):
        "Stop the server on destruction"
        try:
            self.stop()
        except:
            pass

    def _request(self, imageSendMode):
        msg = messages.ImageRequest()
        msg.imageSendMode = imageSendMode
        msg.resolution = self.resolution
        if self.server == "TCP":
            self.sock.send(msg.serialize())
        else:
            self.sock.sendto(msg.serialize(), self.server)

    def _getChunk(self):
        while True:
            data = self.sock.recv(1500)
            if ord(data[0]) == messages.ImageChunk.ID:
                return messages.ImageChunk(data)

    def stop(self):
        "Stop the sever"
        self._request(messages.ISM_OFF)

    def saveSingleImage(self, fileName):
        "Grab a single image from the server and save it"
        self._request(messages.ISM_SINGLE_SHOT)
        fh = open(fileName, 'wb')
        while True:
            msg = self._getChunk()
            sys.stdout.write("Received: %s\n" % str(msg))
            fh.write(msg.data)
            if len(msg.data) < messages.ImageChunk.IMAGE_CHUNK_SIZE: # Last chunk
                break
        fh.close()

    def frameRateTest(self):
        "Grabs frames repeatedly and prints the inter frame interval"
        self._request(messages.ISM_STREAM)
        while True:
            frameNo = 0
            t1 = time.time()
            msg = self._getChunk()
            if msg.imageId != frameNo:
                t2 = time.time()
                frameNo = msg.imageId
                sys.stdout.write("%0.3fms\n" % ((t2-t1)*1000))



if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1].isdigit():
        camera = int(sys.argv[1])
        srv = ServerUDP(camera)
        srv.run()
    elif len(sys.argv) == 3:
        client = Client(sys.argv[1])
        client.saveSingleImage(sys.argv[2])
