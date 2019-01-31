#!/usr/bin/env python3

## simple tool to throttle the output from ninja. This is needed for compilation within a compilation buffer
## in emacs because emacs chokes with lots of long-line output. Usage of this script is:
## $ build | ninja-throttle.py
## where "build" is a build command that outputs status messages of the form:
## [93/174] Building CXX object engine/CMakeFiles/cozmo_engine.dir/vision/visionSystem.cpp.o

import sys
import re
import datetime

sys.stdin.reconfigure(errors='replace')

#TODO: arg

matcher = re.compile(r'\[([0-9]*)/([0-9*]*)\] [a-z,A-Z]*')

class ThrottlePrinter:
    def __init__(self, freq_s):
        self.lastPrintTime = None
        self.throttle_freq = datetime.timedelta(seconds=freq_s)

    def throttle(self, s):
        now = datetime.datetime.now()
        if not self.lastPrintTime or (now - self.lastPrintTime) >= self.throttle_freq:
            self.lastPrintTime = now
            sys.stdout.write(s)


# TODO: argument for freq
printer = ThrottlePrinter(1.0)

for line in sys.stdin:
    match = matcher.match(line)
    if match:
        printer.throttle(line)
    else:        
        sys.stdout.write(line)

   
