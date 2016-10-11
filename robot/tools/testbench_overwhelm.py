#!/usr/bin/env python3
"""
Testbench to deliberately overwhelm the robot with too many messages
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time
sys.path.insert(0, os.path.join("tools"))
try:
    import robotInterface
except ImportError:
    sys.exit("Couldn't import robotInterface library. Are you running from :/robot ?")
else:
    Anki = robotInterface.Anki
    RI = robotInterface.RI

USAGE = """
    {} <INTERVAL> <MESSAGE TYPE> [Additional message types]

A pretty good test is:
tools/testbench_overwhelm.py 0.003 setCubeLights setBackpackLightsMiddle setCubeLights stop
""".format(sys.argv[0])

def run(interval, msgGenerator):
    "Spam the robot with messages from msgGenerator every interval seconds"
    try:
        for msg in msgGenerator:
            ts = time.time()
            robotInterface.Step()
            robotInterface.Send(msg)
            sys.stdout.write(str(time.time()) + '\r')
            while time.time() < ts + interval:
                robotInterface.Step()
    except KeyboardInterrupt:
        return

def tagNameToMessage(tagName):
    "Returns an EngineToRobot message of the given type"
    if not tagName in RI.EngineToRobot._tags_by_name:
        raise ValueError("{} isn't a valid message type".format(tagName))
    else:
        cmdType = RI.EngineToRobot.typeByTag(RI.EngineToRobot._tags_by_name[tagName])
        return RI.EngineToRobot(**{tagName: cmdType()})

def MessageCycler(msgList):
    while True:
        yield from msgList

# Script entry point
if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.exit(USAGE)
    
    try:
        interval = float(sys.argv[1])
    except:
        sys.exit("Couldn't evaluate \"{0}\" as a number of seconds{ls}{ls}{1}{ls}".format(sys.argv[1], USAGE, ls=os.linesep))
    
    try:
        msgs = [tagNameToMessage(t) for t in sys.argv[2:]]
    except ValueError as e:
        sys.exit(str(e))
    
    robotInterface.Init(True, forkTransportThread = False)
    robotInterface.Connect(imageRequest=True)
    while not robotInterface.GetConnected():
        robotInterface.Step()
    
    run(interval, MessageCycler(msgs))
    
    robotInterface.Die()
