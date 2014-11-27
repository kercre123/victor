"""
Camera frame server for Torpedo System on Module
"""
__author__  = "Daniel Casner"
__version__ = "0.0.1"


import sys, socket, time
try:
    import numpy
    import cv2 as cv
except:
    sys.stderr.write("Cannot import numpy or cv modules, some features will be unavailable\n")
import messages

DEFAULT_PORT = 9000

class Server(object):
    "UDP socket server sending imageChunk messages"

    def __init__(self, camera=0, port=DEFAULT_PORT, encoding='.jpg', encodingParams=[1, 90]):
        "Initalize server for specified camera on given port"
        self.vc = cv.VideoCapture(camera)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('', port))
        self.sock.settimeout(0.010) # 10ms
        self.encoding = encoding
        self.encodingParams = encodingParams
        self.client = None

    def step(self):
        "A single server main loop iteration"
        try:
            recvData, addr = self.sock.recvfrom(2000)
        except socket.timeout:
            pass
        else:
            try:
                req = messages.ImageRequest(recvData)
            except Exception, e:
                sys.stderr.write("Bad request:\n\t%s\n\n" % str(e))
            else:
                sys.stdout.write("New connection from %s: %s\n" % (str(addr), str(req)))
                if req.imageSendMode == messages.ISM_OFF:
                    self.client = None
                else:
                    self.client = (req, addr)
                    # TODO Do something with request resolution
                    self.frameNumber = 0
                    self.chunkNumber = 0
                    self.dataQueue = ""
        if self.client:
            if not len(self.dataQueue): # Need a new frame
                # If single shot and have already sent
                if self.frameNumber > 0 and self.client[0].imageSendMode == messages.ISM_SINGLE_SHOT:
                    self.client = None
                    return
                rslt, frame = self.vc.read()
                if rslt: # Got an image
                    rslt, data = cv.imencode(self.encoding, frame, self.encodingParams)
                    if rslt:
                        self.dataQueue = data.tostring()
                        self.chunkNumber =  0
                        self.frameNumber += 1
                    else:
                        sys.stderr.write("Failed to jpeg encode frame\n")
                else:
                    sys.stderr.write("Failed to read camera frame\n")
            if len(self.dataQueue): # have a frame
                msg = messages.ImageChunk()
                self.dataQueue = msg.takeChunk(self.dataQueue)
                msg.imageId = self.frameNumber
                msg.imageEncoding = messages.IE_JPEG
                msg.chunkId = self.chunkNumber
                sys.stdout.write("Send frame %d chunk %d\n" % (self.frameNumber, self.chunkNumber))
                # TODO: Set message resolution
                # msg.resolution = ?????
                self.chunkNumber += 1
                self.sock.sendto(msg.serialize(), self.client[1])

    def run(self):
        "Run the server continuously"
        while True:
            self.step()


class Client(object):
    "Client for UDP camera server for"

    def __init__(self, host, port=DEFAULT_PORT, resolution=messages.CAMERA_RES_VGA):
        "Connect to server"
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server = (host, port)
        self.resolution = resolution

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
        self.sock.sendto(msg.serialize(), self.server)

    def _getChunk(self):
        return messages.ImageChunk(self.sock.recv(1500))

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
    camera = int(sys.argv[1])
    srv = Server(camera)
    srv.run()
