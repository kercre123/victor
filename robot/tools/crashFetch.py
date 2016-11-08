#!/usr/bin/env python3

import sys, os, time

sys.path.insert(0, os.path.join("tools"))
import robotInterface
RI = robotInterface.RI
Anki = robotInterface.Anki

MAX_CRASH_LOGS = 4 # depends on :robot/espressif/include/driver/crash.h

class CrashDownloader:
    "Downloads crashes from the robot"

    def RequestLog(self, index):
        robotInterface.Send(RI.EngineToRobot(requestCrashReports=RI.RequestCrashReports(index)))

    def OnConnect(self, *args):
        self.RequestLog(self.logIndex)
        
    def OnData(self, msg):
        sys.stdout.write("ROBOT CRASH REPORT: err = {0.errorCode}\tsource = {0.which}\t{1} words{2}".format(msg, len(msg.dump), os.linesep))
        with open('robot_crash_{0.errorCode}_{0.which}_{1:10d}.msg'.format(msg, int(time.time())), "wb") as dump:
            dump.write(msg.pack())
        self.logIndex += 1
        if self.logIndex < MAX_CRASH_LOGS:
            self.RequestLog(self.logIndex)
    
    def __init__(self):
        self.logIndex = 0
        robotInterface.SubscribeToConnect(self.OnConnect)
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.crashReport, self.OnData)
        
    @property
    def complete(self):
        return self.logIndex >= MAX_CRASH_LOGS

if __name__ == '__main__':
    robotInterface.Init()
    cd = CrashDownloader()
    robotInterface.Connect()

    try:
        while not cd.complete:
            sys.stdout.flush()
            time.sleep(1.0)
    except KeyboardInterrupt:
        robotInterface.Die()
