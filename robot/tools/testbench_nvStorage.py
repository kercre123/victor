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
    TEST_FUNCTION_PREFIX = "test_"
    
    BIG_BLOB = """'Twas brillig, and the slithy toves
Did gyre and gimble in the wabe;
All mimsy were the borogoves,
And the mome raths outgrabe.

"Beware the Jabberwock, my son!
The jaws that bite, the claws that catch!
Beware the Jubjub bird, and shun
The frumious Bandersnatch!"

He took his vorpal sword in hand:
Long time the manxome foe he sought—
So rested he by the Tumtum tree,
And stood awhile in thought.

And as in uffish thought he stood,
The Jabberwock, with eyes of flame,
Came whiffling through the tulgey wood,
And burbled as it came!

One, two! One, two! and through and through
The vorpal blade went snicker-snack!
He left it dead, and with its head
He went galumphing back.

"And hast thou slain the Jabberwock?
Come to my arms, my beamish boy!
O frabjous day! Callooh! Callay!"
He chortled in his joy.

'Twas brillig, and the slithy toves
Did gyre and gimble in the wabe;
All mimsy were the borogoves,
And the mome raths outgrabe.""".encode()
    
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
        
    @classmethod
    def GetTestNames(cls):
        return [k[len(cls.TEST_FUNCTION_PREFIX):] for k, v in cls.__dict__.items() if k.startswith(cls.TEST_FUNCTION_PREFIX) and callable(v)]
        
    def runTest(self, testName):
        self.testInProgress = True
        ret = self.__class__.__dict__[self.TEST_FUNCTION_PREFIX + testName](self)
        self.testInProgress = False
        return ret
    
    def waitForPending(self):
        while len(self.pendingOps):
            time.sleep(0.05)
        return self.lastResult
    
    @property
    def randomTag(self):
        return int(random.uniform(NVS.NVEntryTag.NVConst_MIN_ADDRESS, NVS.NVEntryTag.NVConst_MAX_ADDRESS-4096)/4)*4
    
    def test_erwr(self):
        tag = self.randomTag
        sector = tag & 0x1FF000
        print("Erasing", hex(sector))
        self.erase(sector, 4096)
        self.waitForPending()
        print("Writing", hex(tag))
        self.write(tag, self.BIG_BLOB)
        self.waitForPending()
        print("Reading", hex(tag))
        self.read(tag, 4096)
        self.waitForPending()
    
    def test_write(self):
        tag = self.randomTag
        print("Writing tag {}".format(tag))
        self.write(tag, b"Hello world")
        self.waitForPending()
    
    def test_wrer(self):
        "Write, read, erase and try reading again"
        tag = self.randomTag
        print("Writing tag {}".format(tag))
        self.write(tag, b"Hello world")
        self.waitForPending()
        print("Reading back")
        self.read(tag)
        self.waitForPending()
        print("Erasing")
        self.erase(tag)
        self.waitForPending()
        print("Reading again (should fail)")
        self.read(tag)
        
    def test_overwrite(self):
        "Write an entry and then write it again then read and make sure it's the new value"
        tag = self.randomTag
        print("Writing tag {}".format(tag))
        self.write(tag, b"I am the very model of a modern major general.")
        if self.waitForPending() == 0:
            print("Overwriting")
            self.write(tag, b"I have information animal vegetable and mineral.")
            if self.waitForPending() == 0:
                print("Reading back")
                self.read(tag)
    
    def overflow(self, useTag=None):
        count = 0
        if useTag is None:
            tag = self.randomTag
        else:
            tag = useTag
        self.write(tag, self.BIG_BLOB)
        while self.waitForPending() == 0:
            count += 1
            self.write(tag, self.BIG_BLOB)
            if useTag is None:
                tag = self.randomTag
            time.sleep(0.1)
        print("Saved {} copies for a total of {} bytes of payload".format(count, len(self.BIG_BLOB)*count))
    
    def test_overflow(self):
        self.overflow(self.randomTag)
        
    def test_overflowUnique(self):
        self.overflow()
        
    def test_readAll(self):
        self.read(0, 0xFFFFfffe)
        self.waitForPending()
        
    def test_readFactory(self):
        self.read(0x80000000, 0xFFFFfffe)
        self.waitForPending()
        
    def test_eraseAll(self):
        self.erase(0, 0xFFFFfffe)
        self.waitForPending()
        
    def test_wipeAll(self):
        robotInterface.Send(RI.EngineToRobot(commandNV=NVS.NVCommand(
            0,
            0,
            NVS.NVOperation.NVOP_WIPEALL
        )))
        self.pendingOps[NVS.NVConst.NVConst_MIN_ADDRESS] = (None, NVS.NVOperation.NVOP_WIPEALL)
        time.sleep(30)
        
    def test_readCameraCalib(self):
        self.read(NVS.NVEntryTag.NVEntry_CameraCalib, record=True)
        self.waitForPending()
        
    def test_readRandomFixture(self):
        self.read(0xC0000000 + int(random.uniform(0, 16)))
        self.waitForPending()
        
    def test_readInvalidFixture(self):
        invalid1 = 0xC0001000
        print("Attempting to read 0x{0:x} ({0:d}), should fail".format(invalid1))
        self.read(invalid1)
        self.waitForPending()
        invalid2 = 0xC0000010
        print("Attempting to read from 0xC0000000 to 0x{0:x} ({0:d}), should fail".format(invalid2))
        self.read(0xC0000000, invalid2)
        self.waitForPending()
        
    def test_readAllFixture(self):
        self.read(0xC0000000, 16)
        self.waitForPending()
        
    def test_writeBirthCertificate(self):
        dts = time.gmtime()
        payload = struct.Struct("BBBBBBBBBB").pack(0, 0, 0, 1, dts.tm_year & 0xff, dts.tm_mon, dts.tm_mday, dts.tm_hour, dts.tm_min, dts.tm_sec)
        self.write(NVS.NVEntryTag.NVEntry_BirthCertificate, payload)
    
    def test_eraseBirthCertificate(self):
        self.erase(NVS.NVEntryTag.NVEntry_BirthCertificate)
        
    def test_readIMUCal(self):
        self.read(0xC0000004)
        self.waitForPending()

if __name__ == "__main__":
    print("NVStorage Testbench")
    parser = argparse.ArgumentParser(prog="NVStorage Testbench", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("test", nargs="+", choices=Testbench.GetTestNames(), help="Which tests to run")
    args = parser.parse_args()
    
    robotInterface.Init()
    t = Testbench()
    robotInterface.Connect()
    while robotInterface.GetConnected() is False:
        time.sleep(0.1)
    try:
        for test in args.test:
            t.runTest(test)
        while t.done == False:
            time.sleep(0.050)
    except KeyboardInterrupt:
        pass
    robotInterface.Disconnect()
    time.sleep(0.1)
    sys.exit()
