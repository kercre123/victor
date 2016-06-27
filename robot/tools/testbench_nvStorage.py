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

class StorageStateEstimate:
    def __init__(self):
        self.store = {}
        
    def append(self, key, value, writeNotErase=True):
        if key in self.store:
            self.store[key].append((value, writeNotErase))
        else:
            self.store[key] = [(value, writeNotErase)]
            
    def get(self, key, index=-1):
        return self.store[key][index]
        
    def gc(self):
        self.store = {k: v[-1:] for k,v in self.store.items() if v[-1][1]}
        
    def calcSize(self):
        sum = 0
        for tag in self.store.values():
            for entry, op in tag:
                sum += len(entry) + 16 # + 12 for size of NVEntryHeader + 4 for control word
        return sum

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
    
    def onConnect(self, dest):
        "Callback when connected to the robot"
        self.waitingForConnect = False
        
    def onOpResult(self, msg):
        "Callback when receiving an operation result"
        tag = msg.report.tag
        if tag == NVS.NVEntryTag.NVEntry_Invalid:
            sys.stderr.write("Received nvResult for invalid tag:{linesep}\t{}{linesep}".format(repr(msg.report), linesep=os.linesep))
        elif tag not in self.pendingOps:
            sys.stderr.write("Received unexpected nvResult:{linesep}\t{}{linesep}".format(repr(msg.report), linesep=os.linesep))
        else:
            if self.pendingOps[tag][1] == "read":
                sys.stdout.write("Pending read of {0:x} returned {1}{linesep}".format(tag, repr(msg.report), linesep=os.linesep))
                if (msg.report.result < 0):
                    del self.pendingOps[tag]
            else:
                sys.stdout.write("Pending {} of {:x} returned {}{linesep}".format(self.pendingOps[tag][1], tag, repr(msg.report), linesep=os.linesep))
                self.estimate.append(tag, self.pendingOps[tag][0], self.pendingOps[tag][1] == "write")
                del self.pendingOps[tag]
        self.lastResult = msg.report.result
    
    def onReadData(self, msg):
        "Callback when receiving read data"
        tag = msg.blob.tag
        if tag == NVS.NVEntryTag.NVEntry_Invalid:
            sys.stderr.write("Recieved nvData for invalid tag:{linesep}\t{}{linesep}".format(repr(msg.blob), linesep=os.linesep))
        elif tag not in self.pendingOps or self.pendingOps[tag][1] != "read":
            sys.stderr.write("Received unexpected nvData: tag = {:x} ({:d}){linesep}\t{:s}{linesep}".format(tag, len(msg.blob.blob), bytes(msg.blob.blob).decode(errors="ignore")[:100], linesep=os.linesep))
        else:
            sys.stdout.write("Pending read of {:x} returned ({:d}){linesep}\t{:s}{linesep}".format(tag, len(msg.blob.blob), bytes(msg.blob.blob).decode(errors="ignore"), linesep=os.linesep))
            if self.pendingOps[tag][2]:
                open("{:x}.nvstorage".format(tag), 'wb').write(bytes(msg.blob.blob))
            del self.pendingOps[tag]
    
    def write(self, tag, blob):
        robotInterface.Send(RI.EngineToRobot(writeNV=NVS.NVStorageWrite(
            NVS.NVReportDest.ENGINE,
            True,
            True,
            False,
            NVS.NVEntryTag.NVEntry_Invalid,
            NVS.NVStorageBlob(tag, blob)
        )))
        self.pendingOps[tag] = (blob, "write")
    
    def erase(self, tag, tagEnd=NVS.NVEntryTag.NVEntry_Invalid):
        robotInterface.Send(RI.EngineToRobot(writeNV=NVS.NVStorageWrite(
            NVS.NVReportDest.ENGINE,
            False,
            True,
            True,
            tagEnd,
            NVS.NVStorageBlob(tag)
        )))
        self.pendingOps[tag] = (None, "erase")
        
    def read(self, tag, tagEnd=NVS.NVEntryTag.NVEntry_Invalid, record=False):
        robotInterface.Send(RI.EngineToRobot(readNV=NVS.NVStorageRead(
            tag,
            tagEnd,
            NVS.NVReportDest.ENGINE
        )))
        self.pendingOps[tag] = (None, "read", record)
        if tagEnd != NVS.NVEntryTag.NVEntry_Invalid:
            self.pendingOps[tagEnd] = (None, "read", False)
    
    @property
    def done(self):
        return len(self.pendingOps) == 0 and self.testInProgress is False
        
    def __init__(self, load=None, save=None, gc=False):
        "Store the specified parameters into the robot"
        if load is None:
            self.estimate = StorageStateEstimate()
            if gc: self.estimate.gc()
        else:
            self.estimate = pickle.load(open(load, "rb"))
        self.save = save
        self.pendingOps = {}
        self.lastResult = None
        self.testInProgress = False
        self.waitingForConnect = True
        robotInterface.Init()
        robotInterface.SubscribeToConnect(self.onConnect)
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.nvResult, self.onOpResult)
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.nvData,   self.onReadData)
        robotInterface.Connect()
        while self.waitingForConnect: time.sleep(0.05)
        
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
        return int(random.uniform(1, 2**31))
    
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
        to = Anki.Cozmo.NVStorage.NVReportDest.ENGINE
        robotInterface.Send(RI.EngineToRobot(wipeAllNV=Anki.Cozmo.NVStorage.NVWipeAll(to, True, "Yes I really want to do this!")))
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
        self.read(0xC0000000, 0xC000000F)
        self.waitForPending()
        
    def test_writeBirthCertificate(self):
        dts = time.gmtime()
        payload = struct.Struct("BBBBBBBBBB").pack(0, 0, 0, 1, dts.tm_year & 0xff, dts.tm_mon, dts.tm_mday, dts.tm_hour, dts.tm_min, dts.tm_sec)
        self.write(NVS.NVEntryTag.NVEntry_BirthCertificate, payload)

if __name__ == "__main__":
    print("NVStorage Testbench")
    parser = argparse.ArgumentParser(prog="NVStorage Testbench", formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-s", "--save", default=None, help="File to store expected NVStorage state to")
    parser.add_argument("-l", "--load", default=None, help="File to load expected NVStorage state from")
    parser.add_argument("-g", "--gc",   default=False, action="store_true", help="Whether we should update the estimated state with a garbage collection")
    parser.add_argument("test", nargs="+", choices=Testbench.GetTestNames(), help="Which tests to run")
    args = parser.parse_args()
    
    t = Testbench(args.load, args.save, args.gc)
    time.sleep(1.0)
    try:
        for test in args.test:
            t.runTest(test)
        while t.done == False:
            time.sleep(0.050)
    except KeyboardInterrupt:
        pass
    robotInterface.Disconnect()
    time.sleep(0.015)
    sys.exit()
