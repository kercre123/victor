"""
A base class for sub servers
"""
__author__ = "Daniel Casner"

import sys, threading, time, socket

class BaseSubServer(threading.Thread):

    def __init__(self, server, verbose=False):
        threading.Thread.__init__(self)
        self._continue = True
        self.server = server
        self.v = verbose
        if verbose:
            sys.stdout.write("%s will be verbose\n" % repr(self))
        self.sendSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def __del__(self):
        "Call stop before deleating"
        self.stop()

    def stop(self):
        "Stops the subserver thread the next time through it's main loop"
        self._continue = False
        self.sendSocket.close()

    def threadYield(self):
        "Have the thried yeild to other threads"
        time.sleep(0.0003) # Allow a little time for traffic to clear through various ISRs

    def giveMessage(self, message):
        "Passes in a message from the main server"
        raise TypeError("BaseSubServer sub-classes must override giveMessage")

    def clientSend(self, message):
        "Sends a message to the servers client"
        if self.server.client:
            self.sendSocket.sendto(message, self.server.client)

    def standby(self):
        "Puts the subserver it a lower power state"
        # Sub classes may or may not implement this
        pass

    def step(self):
        "One main loop iteration for the thread"
        raise TypeError("BaseSubServer sub-classes must override step")

    def run(self):
        "Run method for the thread"
        while self._continue is True:
            if self.v > 10:
                sys.stdout.write("%s\n" % repr(self))
            self.step()
            self.threadYield()

    def log(self, data):
        "Write something to the log and echo to console. Use instead of writing to stdout."
        return self.server.logger.write(data)
