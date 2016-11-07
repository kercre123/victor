#!/usr/bin/env python3
"""
Test bench to stimulate robot nvstorage module
"""
import sys, os, time, struct, pickle, argparse, random, argparse
from zlib import crc32
import PIL

sys.path.insert(0, os.path.join("tools"))

import robotInterface
import fota
import minipegReceiver
Anki = robotInterface.Anki
RI   = robotInterface.RI

class InfiniteUpgradeFile:
    
    def __init__(self):
        self.data = b""
        
    def read(self, bytes):
        if bytes > len(self.data):
            block = b"\xff" * 2048
            crc = crc32(block)
            self.data += block + struct.Struct("II").pack(0xFFFFfffc, crc)
        ret = self.data[:bytes]
        self.data = self.data[bytes:]
        return ret

class Testbench:
    
    TEST_FUNCTION_PREFIX = "test_"
    
    @classmethod
    def GetTestNames(cls):
        return [k[len(cls.TEST_FUNCTION_PREFIX):] for k, v in cls.__dict__.items() if k.startswith(cls.TEST_FUNCTION_PREFIX) and callable(v)]

    def runTest(self, testName):
        self.testInProgress = True
        ret = self.__class__.__dict__[self.TEST_FUNCTION_PREFIX + testName](self)
        self.testInProgress = False
        return ret

    def test_InfiniteOTA(self):
        "Send an infinite no-op OTA file to the robot"
        robotInterface.Init(True)
        up = fota.OTAStreamer(InfiniteUpgradeFile())
        robotInterface.Connect()
        up.main()
        del up

    def test_post:
        "Test for post OTA issues"
        

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog="FOTA Testbench", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('test', nargs='+', choices=Testbench.GetTestNames(), help="The test to run")
    parser.add_argument('--no-sync',   action='store_true', help="Do not send sync time message to robot on startup.")
    parser.add_argument('--sync-time', default=0, type=int, help="Manually specify sync time offset")
    parser.add_argument('-b', '--no-blinkers', action='store_true', help="Do not turn on backpack light test pattern")
    parser.add_argument('-v', '--image-request', action='store_true', help="Request video from the robot")
    parser.add_argument('-i', '--ip_address', default="172.31.1.1", help="Specify robot's ip address")
    parser.add_argument('-p', '--port', default=5551, type=int, help="Manually specify robot's port")
    args = parser.parse_args()
    
    t = Testbench()
    try:
        for test in args.test:
            t.runTest(test)
        while t.done == False:
            time.sleep(0.050)
    except KeyboardInterrupt:
        pass
