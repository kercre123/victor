#!/usr/bin/env python3
"""
ThreadedInput - wraps input in a thread so it can run be used without blocking main thread
"""
__author__ = "Mark Wesley"

import sys, os, time, threading
from collections import deque
    

class ThreadedInput(threading.Thread):

    def __init__(self, promptText):
        "ThreadedInput"
        if os.name == 'posix' and sys.version_info.major > 2:
            threading.Thread.__init__(self, daemon=True)
        else:
            threading.Thread.__init__(self)
        self.queue = deque([]) # deque of commands in the order they were input
        self.lock = threading.Lock()
        self.input = input
        self.prompt = promptText
        self._continue = True
        self.framesToWaitForOutput = 0

    def __del__(self):
        self.KillThread()

    def run(self):
        while self._continue:     
            try:
                time.sleep(0.1)
                if self.framesToWaitForOutput <= 0:
                    args = self.input(self.prompt).split()
                    if self._continue and len(args):
                        self.QueueCommand(args)
            except EOFError:
                sys.stdout.write("[ThreadedInput] Exception EOF\t{}{}".format(str(ret), os.linesep))
                #return
            except KeyboardInterrupt: # e.g. ctrl-c
                sys.stdout.write("[ThreadedInput] Exception Keyboard Interrupt\t{}{}".format(str(ret), os.linesep))
                #return
            except Exception as e:
                sys.stderr.write("[ThreadedInput] Exception: {0}\t{0}{1}".format(e, str(ret), os.linesep))
            else:
                pass

    def KillThread(self):
        "Clean up the thread"
        self._continue = False
        self.lock.acquire()
        self.queue = []
        self.lock.release()
        
    def QueueCommand(self, *args):
        "Thread safe queing of a command"
        self.lock.acquire()
        self.framesToWaitForOutput += 5
        self.queue.append(*args)
        self.lock.release()
        
    def PopCommand(self):
        "Thread safe pop of oldest command queued"
        self.lock.acquire()
        if self.framesToWaitForOutput > 0:
            self.framesToWaitForOutput -= 1
        try:
            poppedCommand = self.queue.popleft()
        except IndexError:
            poppedCommand = None
        self.lock.release()
        return poppedCommand;

