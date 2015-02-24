"""
A base class for sub servers
"""
__author__ = "Daniel Casner"

import sys, threading

class BaseSubServer(threading.Thread):

    def __init__(self, server, timeout=1.0, verbose=False):
        threading.Thread.__init__(self)
        self._continue = True
        self.server = server
        self.v = verbose
        if verbose:
            sys.stdout.write("%s will be verbose\n" % repr(self))

    def __del__(self):
        "Call stop before deleating"
        self.stop()

    def stop(self):
        "Stops the subserver thread the next time through it's main loop"
        self._continue = False

    def giveMessage(self, message):
        "Passes in a message from the main server"
        raise TypeError("BaseSubServer sub-classes must override giveMessage")

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
