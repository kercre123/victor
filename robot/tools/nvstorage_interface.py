#!/usr/bin/env python3
"""
Test bench to stimulate robot nvstorage module
"""
import sys, os, time, struct, pickle, argparse, random

sys.path.insert(0, os.path.join("tools"))
import robotInterface
Anki = robotInterface.Anki
RI  = Anki.Cozmo.RobotInterface
NVS = Anki.Cozmo.NVStorage

class Testbench:
    
    def onOpResult(self, msg):
        "Callback when receiving an operation result"
        if len(msg.blob) == 0:
            blob = repr([])
        else:
            try:
                blob = msg.blob.decode()
            except:
                blob = " ".join(("{:02x}".format(b) for b in msg.blob))
        sys.stdout.write("{0.operation!s}: {0.result!s}{ls}\tAddress {0.address:x}{ls}\tOffset {0.offset:x}{ls}\tBlob {1}{ls}".format(msg, blob, ls=os.linesep))
        if msg.address in self.pendingOps:
            del self.pendingOps[msg.address]
        self.lastResult = msg.result

    def writefile(self, tag, filename):
        with open(filename,"rb") as f:
            data=f.read()
            for i in range(0, len(data), 1024):
                self.write(tag+i, data[i:i+1024])
                self.waitForPending()
            
    def write(self, tag, blob):
        robotInterface.Send(RI.EngineToRobot(commandNV=NVS.NVCommand(
            tag,
            0,
            NVS.NVOperation.NVOP_WRITE,
            blob = blob
        )))
        self.pendingOps[tag] = (blob, NVS.NVOperation.NVOP_WRITE)
    
    def erase(self, tag, length):
        robotInterface.Send(RI.EngineToRobot(commandNV=NVS.NVCommand(
            tag,
            length,
            NVS.NVOperation.NVOP_ERASE
        )))
        self.pendingOps[tag] = (None, NVS.NVOperation.NVOP_ERASE)
        
    def read(self, tag, length=1, record=False):
        robotInterface.Send(RI.EngineToRobot(commandNV=NVS.NVCommand(
            tag,
            length,
            NVS.NVOperation.NVOP_READ
        )))
        self.pendingOps[tag] = (None, NVS.NVOperation.NVOP_READ, record)
        if length > 1:
            self.pendingOps[tag + length] = (None, NVS.NVOperation.NVOP_READ, False)
    
    @property
    def done(self):
        return len(self.pendingOps) == 0 and self.testInProgress is False
    
    def __init__(self):
        "Store the specified parameters into the robot"
        self.pendingOps = {}
        self.lastResult = None
        self.testInProgress = False
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.nvOpResult, self.onOpResult)
        
    def __del__(self):
        if self.save is not None:
            pickle.dump(self.estimate, open(self.save, "wb"), -1)
        robotInterface.Disconnect()
    
    def waitForPending(self):
        while len(self.pendingOps):
            time.sleep(0.05)
        return self.lastResult
    
    @property
    def randomTag(self):
        return int(random.uniform(NVS.NVConst.NVConst_MIN_ADDRESS, NVS.NVConst.NVConst_MAX_ADDRESS-4096)/4)*4

def auto_int(s):
    '''converts string with base indicators to int'''
    return int(s,0)

if __name__ == "__main__":
    print("NVStorage Interface")
    parser = argparse.ArgumentParser(prog="NVStorage Interface", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("address",nargs='?', type=auto_int, help ="Address to act on. Default action is read")
    parser.add_argument("file", nargs='?', help="Write contents of this file to address")
    parser.add_argument("-erase_sectors", type=auto_int, help="erase N sectors at this address")
    args = parser.parse_args()
   
    
    robotInterface.Init()
    t = Testbench()
    robotInterface.Connect()
    while robotInterface.GetConnected() is False:
        time.sleep(0.1)
    try:
        if args.erase_sectors:
            t.erase(args.address, args.erase_sectors*4096)
            t.waitForPending()
        if args.file:
            t.writefile(args.address, args.file)
        elif args.address:
            t.read(args.address)
            t.waitForPending()
        else:
            parser.print_help()
        while t.done == False:
            time.sleep(0.050)
    except KeyboardInterrupt:
        pass
    robotInterface.Disconnect()
    time.sleep(0.1)
    sys.exit()
