"""
Camera frame server for Torpedo System on Module
"""
__author__  = "Daniel Casner"
__version__ = "0.0.1"


import sys, socket, time, math, subprocess, select
import messages

MTU = 65535

class CameraSubServer(object):
    "imageChunk server base class"

    RESOLUTION_TUPLES = {
        messages.CAMERA_RES_QUXGA: (3200, 2400),
        messages.CAMERA_RES_QXGA: (2048, 1536),
        messages.CAMERA_RES_UXGA: (1600, 1200),
        messages.CAMERA_RES_SXGA: (1280, 960),
        messages.CAMERA_RES_XGA: (1024, 768),
        messages.CAMERA_RES_SVGA: (800, 600),
        messages.CAMERA_RES_VGA: (640, 480),
        messages.CAMERA_RES_QVGA: (320, 240),
        messages.CAMERA_RES_QQVGA: (160, 120),
        messages.CAMERA_RES_QQQVGA: (80, 60),
        messages.CAMERA_RES_QQQQVGA: (40, 30),
        messages.CAMERA_RES_NONE: (0, 0)
    }

    SENSOR_RESOLUTION = messages.CAMERA_RES_SVGA
    SENSOR_RES_TPL = RESOLUTION_TUPLES[SENSOR_RESOLUTION]

    ENCODER_SOCK_HOSTNAME = '127.0.0.1'
    ENCODER_SOCK_PORT     = 6000
    ENCODER_QUALITY       = 95
    ENCODER_FPS           = 30

    ENCODER_LATEANCY = 5 # ms, SWAG

    def __init__(self, poller, verbose=False, test_framerate=False):
        "Initalize server for specified camera on given port"
        self.v = verbose
        if self.v:
            sys.stdout.write("CameraSubServer will be verbose\n")
        self.tfr = test_framerate
        if self.tfr:
            sys.stdout.write("Will print frame rate information\n")

        subprocess.call(['ifconfig', 'lo', 'up']) # Bring up the loopback interface if it isn't already

        # Setup local jpeg data receive socket
        self.encoderProcess = None
        self.encoderSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.encoderSocket.bind((self.ENCODER_SOCK_HOSTNAME, self.ENCODER_SOCK_PORT))
        self.encoderSocket.settimeout(0) # Set non-blocking
        poller.register(self.encoderSocket, select.POLLIN)

        hostname = socket.gethostname()
        assert hostname.startswith('cozmo'), "Hostname must be of the format cozmo#"
        if hostname == 'cozmo':
            self.camDev = "mt9p031"
        else:
            self.camDev = "ov2686"

        assert subprocess.call(['media-ctl', '-v', '-r', '-l', '"%s":0->"OMAP3 ISP CCDC":0[1], "OMAP3 ISP CCDC":2->"OMAP3 ISP preview":0[1], "OMAP3 ISP preview":1->"OMAP3 ISP resizer":0[1], "OMAP3 ISP resizer":1->"OMAP3 ISP resizer output":0[1]' % self.camDev]) == 0, "media-ctl ISP links setup failure"

        # ISP resizer resolution
        self.ISPResolution = messages.CAMERA_RES_NONE
        # Image encoder output resolution
        self.ENCResolution = messages.CAMERA_RES_NONE

        # Setup video data chunking state
        self.imageNumber    = 0
        self.nextFrame      = ""
        self.sendMode       = messages.ISM_OFF

        self.latestTimestamp = (0, time.time())
        self.lastFrameOTime = 0
        self.lastFrameITime = 0

    def __del__(self):
        "Shut down processes in the right order"
        self.stopEncoder()
        self.encoderSocket.close()

    def updateTimestamp(self, timestamp):
        "Update the latest sync time / M4 timestamp"
        self.latestTimestamp = (timestamp, time.time())

    def getTimestamp(self):
        "Return extrapolated synced timestamp"
        return self.latestTimestamp[0] # + ((time.time() - self.latestTimestamp[1])*1e3) # Timestamp is in units of ms

    def setResolution(self, resolutionEnum):
        "Adjust the camera resolution if nessisary and (re)start the encoder"
        if resolutionEnum < self.SENSOR_RESOLUTION:
            resolutionEnum = self.SENSOR_RESOLUTION
        elif resolutionEnum > messages.CAMERA_RES_QQVGA:
            resolutionEnum = messages.CAMERA_RES_QQVGA
        if resolutionEnum == self.ENCResolution:
            self.startEncoder(resolutionEnum)
            return True
        else:
            self.stopEncoder()
            if resolutionEnum > messages.CAMERA_RES_QVGA: # Minimum isp resolution
                ispRes = messages.CAMERA_RES_QVGA
            if subprocess.call(['media-ctl', '-v', '-f', '"%s":0 [SBGGR12 %dx%d], "OMAP3 ISP CCDC":2 [SBGGR10 %dx%d], "OMAP3 ISP preview":1 [UYVY %dx%d], "OMAP3 ISP resizer":1 [UYVY %dx%d]' % \
                                (self.camDev, self.SENSOR_RES_TPL[0], self.SENSOR_RES_TPL[1], self.SENSOR_RES_TPL[0], self.SENSOR_RES_TPL[1], self.SENSOR_RES_TPL[0], self.SENSOR_RES_TPL[1], \
                                 self.RESOLUTION_TUPLES[ispRes][0], self.RESOLUTION_TUPLES[ispRes][1])]) == 0:
                self.ISPResolution = ispRes
                self.startEncoder(resolutionEnum)
                return True
            else:
                return False


    def startEncoder(self, encRes):
        "Starts the encoder subprocess"
        if self.encoderProcess is None or self.encoderProcess.poll() is not None:
            self.encoderProcess = subprocess.Popen(['gst-launch', 'v4l2src', 'device=/dev/video6', '!', 'ffmpegcolorspace', \
                                                    '!', 'TIImgenc1', 'engineName=codecServer', 'codecName=jpegenc', \
                                                    'resolution=%dx%d' % self.RESOLUTION_TUPLES[encRes], 'iColorSpace=UYVY', \
                                                    'oColorSpace=YUV420P', 'qValue=%d' % self.ENCODER_QUALITY, '!', 'udpsink', \
                                                    'host=%s' % self.ENCODER_SOCK_HOSTNAME, 'port=%d' % self.ENCODER_SOCK_PORT])
            self.ENCResolution = encRes

    def stopEncoder(self):
        "Stop the encoder subprocess if it is running"
        if self.encoderProcess is not None:
            if self.encoderProcess.poll() is None:
                self.encoderProcess.kill()
                self.encoderProcess.wait()
            self.encoderProcess = None

    def poll(self, message=None):
        "Poll for this subserver, passing incoming message if any and returning outgoing message if any."
        outMsgs = []
        # Handle incoming messages if any
        if message and ord(message[0]) == messages.ImageRequest.ID:
            inMsg = messages.ImageRequest(message)
            if self.v: sys.stdout.write("New image request: %s\n" % str(inMsg))
            self.sendMode = inMsg.imageSendMode
            if inMsg.imageSendMode == messages.ISM_OFF:
                self.stopEncoder()
            else: # Not off, set resolution and start encoder
                self.setResolution(inMsg.resolution)
        # Handle encoder data transmission
        if self.nextFrame:
            if self.tfr:
                tick = time.time()
                sys.stdout.write('FOP: %f ms\n' % ((tick - self.lastFrameOTime)*1000))
                self.lastFrameOTime = tick
            # If only sending single image
            if self.sendMode == messages.ISM_SINGLE_SHOT:
                self.stopEncoder()
                self.sendMode = messages.ISM_OFF
            frame = self.nextFrame
            self.imageNumber += 1
            msg = messages.ImageChunk()
            msg.imageId = self.imageNumber
            msg.imageTimestamp = self.getTimestamp()
            msg.imageEncoding = messages.IE_JPEG
            msg.imageChunkCount = int(math.ceil(float(len(frame)) / messages.ImageChunk.IMAGE_CHUNK_SIZE))
            msg.resolution = self.ENCResolution
            chunkNumber = 0
            while frame:
                frame = msg.takeChunk(frame)
                msg.chunkId = chunkNumber
                chunkNumber += 1
                outMsgs.append(msg.serialize())
        # Get a new frame if any from encoder
        try:
            self.nextFrame = self.encoderSocket.recv(MTU)
            if self.tfr:
                tick = time.time()
                sys.stdout.write("FIP: %f ms\n" % ((tick - self.lastFrameITime)*1000))
                self.lastFrameITime = tick
        except:
            pass
        if self.encoderProcess is not None and self.encoderProcess.poll() is not None:
            raise Exception("Encoder sub-process has terminated")
        return outMsgs

    def standby(self):
        "Put the sub server into standby mode"
        self.stopEncoder()
        self.sendMode = messages.ISM_OFF


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
