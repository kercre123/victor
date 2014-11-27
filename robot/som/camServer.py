"""
Camera frame server for Torpedo System on Module
"""
__author__  = "Daniel Casner"
__version__ = "0.0.1"


import sys, socket, numpy
import cv2 as cv
import messages

class Server(object):
    "UDP socket server sending imageChunk messages"

    def __init__(self, camera, port):
        "Initalize server for specified camera on given port"
        self.vc = cv.VideoCapture(camera)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('', port))
        self.sock.settimeout(0.010) # 10ms
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
                self.client = (req, addr)
                # TODO Do something with request parameters...
                self.frameNumber = 0
                self.chunkNumber = 0
                self.dataQueue = ""
        if self.client:
            if not len(self.dataQueue): # Need a new frame
                rslt, frame = self.vc.read()
                if rslt: # Got an image
                    rslt, data = cv.imencode(".jpg", frame, [1, 50])
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


if __name__ == "__main__":
    camera = int(sys.argv[1])
    port   = int(sys.argv[2])
    srv = Server(camera, port)
    srv.run()
