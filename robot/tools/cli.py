#!/usr/bin/env python3
"""
Python command line interface for Robot over the network
"""

import sys, os, socket, threading, time, select, math, muencode, pickle, re, struct

if sys.version_info.major < 3:
    sys.stdout.write("Python2.x is depricated{}".format(os.linesep))

TOOLS_DIR = os.path.join("tools")
CLAD_DIR  = os.path.join("generated", "cladPython", "robot")
ANKI_LOG_STRING_TABLE = os.path.join('..', 'resources', 'config', 'basestation', 'AnkiLogStringTables.json')

if not os.path.isdir(TOOLS_DIR):
    sys.exit("Cannot find tools directory \"{}\". Are you running from the base robot directory?".format(TOOLS_DIR))
elif not os.path.isdir(CLAD_DIR):
    sys.exit("Cannot find CLAD directory \"{}\". Are you running from the base robot directory?".format(CLAD_DIR))

sys.path.insert(0, TOOLS_DIR)
sys.path.insert(0, CLAD_DIR)

from ReliableTransport import *

from ankiLogPP import importTables

from clad.robotInterface.messageEngineToRobot import Anki
from clad.robotInterface.messageRobotToEngine import Anki as _Anki
Anki.update(_Anki.deep_clone())
RobotInterface = Anki.Cozmo.RobotInterface
AnimKeyFrame = Anki.Cozmo.AnimKeyFrame

reinterpret_cast = {
    "d": lambda x: x,
    "i": lambda x: x,
    "x": lambda x: x,
    "f": lambda x: struct.unpack("f", struct.pack("i", x))[0],
}

FORMATTER_KEY = re.compile(r'(?<!%)%[0-9.-]*([{}])'.format("".join(reinterpret_cast.keys()))) # Find singal % marks

def formatTrace(fmt, args):
    "Returns the formatted string from a trace, doing the nesisary type reinterpretation"
    convertedArgs = tuple([reinterpret_cast[t](a) for t, a in zip(FORMATTER_KEY.findall(fmt), args)])
    return fmt % convertedArgs

class CozmoCLI(IDataReceiver):
    "A class for managing the CLI REPL"

    def __init__(self, unreliableTransport, robotAddress, statePrintInterval, printAll=False, stateIntervalStats=False, dryRun=False):
        self.robot = robotAddress
        if not dryRun:
            sys.stdout.write("Connecting to robot at: {}{}".format(repr(robotAddress), os.linesep))
            unreliableTransport.OpenSocket()
            self.transport = ReliableTransport(unreliableTransport, self)

            self.transport.Connect(robotAddress)
            self.transport.start()
        else:
            sys.stdout.write("Dry run! (no robot connection){}".format(os.linesep))
            self.transport = None
        self.input = input
        self.lastStatePrintTime = 0.0 if statePrintInterval > 0.0 else float('Inf')
        self.statePrintInterval = statePrintInterval
        self.printAll = printAll
        if stateIntervalStats:
            self.lastStateRXTimes = [0.0, 0.0]
        else:
            self.lastStateRXTimes = None
        self.nameTable, self.formatTable = importTables(ANKI_LOG_STRING_TABLE)

    def __del__(self):
        if self.transport != None:
            self.transport.KillThread()

    def OnConnectionRequest(self, sourceAddress):
        raise Exception("CozmoCLI wasn't expecing a connection request")

    def OnConnected(self, sourceAddress):
        sys.stdout.write("Connected to robot at {}{}".format(repr(sourceAddress), os.linesep))

    def OnDisconnected(self, sourceAddress):
        sys.stdout.write("Lost connection to robot at {}{}".format(repr(sourceAddress), os.linesep))

    def ReceiveData(self, buffer, sourceAddress):
        now = time.time()
        try:
            msg = RobotInterface.RobotToEngine.unpack(buffer)
        except:
            if len(buffer):
                sys.stderr.write("Couldn't unpack message 0x{:x}[{:d}]{linesep}".format(buffer[0], len(buffer), linesep=os.linesep))
            else:
                sys.stderr.write("Can't unpack an empty message")
                sys.stderr.write(os.linesep)
            sys.stderr.flush()
            return
        if msg.tag == msg.Tag.printText:
            sys.stdout.write("ROBOT PRINT ({:d}): {}".format(msg.printText.level, msg.printText.text))
        elif msg.tag == msg.Tag.trace:
            base = "ROBOT TRACE"
            if not msg.trace.name in self.nameTable:
                sys.stderr.write("{} unknown trace name ID {:d}{}".format(base, msg.trace.name, os.linesep))
            elif not msg.trace.stringId in self.formatTable:
                kwds = {'linesep':  os.linesep,
                        'base':     base,
                        'level':    msg.trace.level,
                        'name':     self.nameTable[msg.trace.name],
                        'stringId': msg.trace.stringId,
                        'vals':     repr(msg.trace.value)
                }
                sys.stderr.write("{base} {level:d} {name}: Unknown format string id {stringId:d}.{linesep}\tValues = {vals}{linesep}".format(**kwds))
            elif len(msg.trace.value) != self.formatTable[msg.trace.stringId][1]:
                kwds = {'linesep':  os.linesep,
                        'base':     base,
                        'level':    msg.trace.level,
                        'name':     self.nameTable[msg.trace.name],
                        'fmt':      self.formatTable[msg.trace.stringId][0],
                        'nargs':    self.formatTable[msg.trace.stringId][1],
                        'vals':     repr(msg.trace.value),
                        'nvals':  len(msg.trace.value)
                }
                sys.stderr.write("{base} {level:d} {name}: Number of args ({nvals:d}) doesn't match format string ({nargs:d}){linesep}\tFormat:{fmt}{linesep}\t{vals}{linesep}".format(**kwds))
            else:
                kwds = {'linesep':   os.linesep,
                        'base':      base,
                        'level':     msg.trace.level,
                        'name':      self.nameTable[msg.trace.name],
                        'formatted': formatTrace(self.formatTable[msg.trace.stringId][0], msg.trace.value)
                }
                sys.stdout.write("{base} ({level:d}) {name}: {formatted}{linesep}".format(**kwds))
        elif msg.tag == msg.Tag.crashReport:
            sys.stdout.write("Received crash report {:d}{linesep}".format(msg.crashReport.which, linesep=os.linesep))
            sys.stdout.flush()
            fh = open("robot WiFi crash report {}.p".format(time.ctime()), "wb")
            pickle.dump(msg.crashReport.dump, fh, 2)
            fh.close()
        elif msg.tag == msg.Tag.activeObjectConnectionState:
            sys.stdout.write(repr(msg.activeObjectConnectionState))
            sys.stdout.write(os.linesep)
            sys.stdout.flush()
        elif msg.tag == msg.Tag.activeObjectTapped:
            sys.stdout.write(repr(msg.activeObjectTapped))
            sys.stdout.write(os.linesep)
            sys.stdout.flush()
        elif msg.tag == msg.Tag.activeObjectMoved:
            sys.stdout.write(repr(msg.activeObjectMoved))
            sys.stdout.write(os.linesep)
            sys.stdout.flush()
        elif msg.tag == msg.Tag.activeObjectStopped:
            sys.stdout.write(repr(msg.activeObjectStopped))
            sys.stdout.write(os.linesep)
            sys.stdout.flush()
        elif msg.tag == msg.Tag.state and now - self.lastStatePrintTime > self.statePrintInterval:
            sys.stdout.write(repr(msg.state))
            sys.stdout.write(os.linesep)
            sys.stdout.flush()
            self.lastStatePrintTime = now
        elif self.lastStateRXTimes is not None and msg.tag == msg.Tag.state:
            sys.stdout.write("{0}: {1:0.2f}ms{2}".format(msg.tag_name, msg.state.timestamp-self.lastStateRXTimes[0], os.linesep))
            self.lastStateRXTimes[0] = msg.state.timestamp
        elif self.lastStateRXTimes is not None and msg.tag == msg.Tag.animState:
            sys.stdout.write("{0}: {1:0.2f}ms{2}".format(msg.tag_name, (msg.animState.timestamp-self.lastStateRXTimes[1])/1000.0, os.linesep))
            self.lastStateRXTimes[1] = msg.animState.timestamp
        elif self.printAll:
            sys.stdout.write("{}{}".format(msg.tag_name, os.linesep))
        

    def send(self, msg):
        if self.transport != None:
            self.transport.SendData(True, False, self.robot, msg.pack())
        else:
            msg.pack()


    def _helpFtnHelper(self, cmd, oneLine = False):
        outgoingMessageType = RobotInterface.EngineToRobot

        if not hasattr(outgoingMessageType, cmd):
            sys.stdout.write("No message of type '{}'{}".format(cmd, os.linesep))
            return False

        msgTag = getattr(outgoingMessageType.Tag, cmd)
        msgType = outgoingMessageType.typeByTag(msgTag)

        # grab the arguments passed into the init function, excluding "self"
        try:
            args = msgType.__init__.__code__.co_varnames[1:]
        except AttributeError:
            # in case it's a primitive type like 'int'
            #TODO: once we have enum to string, use that here and list possible values
            args = [msgType.__name__]

        defaults = []
        try:
            defaults = msgType.__init__.__defaults__
        except AttributeError:
            pass


        if not oneLine:
            print("{}: {} command".format(cmd, msgType.__name__))
            print("usage: {} {}".format(cmd, ' '.join(args)))
            if len(args) > 0:
                print()

            for argnum in range(len(args)):
                arg = args[argnum]
                if len(defaults) > argnum:
                    defaultStr = "default = {}".format(defaults[argnum])
                else:
                    defaultStr = ""

                typeStr = ""
                try:
                    doc = getattr(msgType, arg).__doc__
                except AttributeError:
                    pass
                else:
                    if doc:
                        docSplit = doc.split()
                        if len(docSplit) > 0:
                            typeStr = docSplit[0]

                    print("    {}: {} {}".format(arg, typeStr, defaultStr))
        else:
            print ("  {} {}".format(cmd, ' '.join(args)))

        return True

    def helpFtn(self, *args):
        "Prints out help text on CLI functions"
        if len(args) == 0:
            print("type 'help command' to get help for command")
            print("type 'list' to see a list of commands")
            print("type 'exit' to exit")
            print("type 'tone' to send a tone to the animation controller")
            return True

        return self._helpFtnHelper(args[0])

    def listFtn(self):
        "Lists available commands"
        outgoingMessageType = RobotInterface.EngineToRobot
        print("available commands (use help for more details):")
        for cmd in outgoingMessageType._tags_by_name:
            self._helpFtnHelper(cmd, oneLine = True)


    def exitFtn(self, *args):
        "Exit the REPL"
        raise EOFError
        return False
        
    def sendTone(self, *args):
        "Sends a tone to the animaation controller"
        SAMPLES_PER_SECOND  = 7440 * 4
        SAMPLES_PER_MESSAGE = Anki.Cozmo.AnimConstants.AUDIO_SAMPLE_SIZE
        try:
            freq = float(eval(args[0]))
        except:
            sys.stderr.write("tone requres a floating point frequencey argument" + os.linesep)
            return False
        sys.stdout.write("Tone frequency {:d}Hz{}".format(int(freq), os.linesep))
        try:
            duration = float(eval(args[1]))
        except:
            duration = SAMPLES_PER_MESSAGE / SAMPLES_PER_SECOND
        def ToneGenerator(period, length):
            p = 0.0
            while True:
                samples = []
                while len(samples) < length:
                    samples.append(muencode.encodeSample(int((math.sin(p))*0x200)))
                    p += 2*math.pi/period
                yield samples
        def soundGenerator(length):
            raw = open("beethoven.mu", "rb")
            while True:
                yield raw.read(length)
        def SawtoothGenerator(length):
            phase = 0
            while True:
                samples = []
                while len(samples) < length:
                    samples.append(phase)
                    if phase < 255:
                        phase += 1
                    else:
                        phase = 0;
                yield samples
        def SquareGenerator(period, length):
            phase = 0
            while True:
                samples = []
                while len(samples) < length:
                    if phase < period/2:
                        samples.append(0)
                    else:
                        samples.append(0x3f)
                    if phase < period-1:
                        phase += 1
                    else:
                        phase = 0
                yield samples
        #tonerator = ToneGenerator(SAMPLES_PER_SECOND / freq, SAMPLES_PER_MESSAGE)
        #tonerator = soundGenerator(SAMPLES_PER_MESSAGE)
        #tonerator = SawtoothGenerator(SAMPLES_PER_MESSAGE)
        tonerator = SquareGenerator(round(freq), SAMPLES_PER_MESSAGE)
        buffersSent = 1
        self.send(RobotInterface.EngineToRobot(animAudioSample=AnimKeyFrame.AudioSample(tonerator.send(None))))
        self.send(RobotInterface.EngineToRobot(animStartOfAnimation=AnimKeyFrame.StartOfAnimation(0xaa)))
        sentTime = SAMPLES_PER_MESSAGE / SAMPLES_PER_SECOND
        while sentTime < duration:
            sentTime += SAMPLES_PER_MESSAGE / SAMPLES_PER_SECOND
            sys.stdout.write("{:f} ({:f}){}".format(sentTime, duration, os.linesep))
            self.send(RobotInterface.EngineToRobot(animAudioSample=AnimKeyFrame.AudioSample(tonerator.send(None))))
            buffersSent += 1
        self.send(RobotInterface.EngineToRobot(animEndOfAnimation=AnimKeyFrame.EndOfAnimation()))
        sys.stdout.write("Sent {} buffers, {} total samples{}".format(buffersSent, buffersSent*SAMPLES_PER_MESSAGE, os.linesep))
        return True

    def REP(self):
        "Read, eval, print"
        args = self.input("COZMO>>> ").split()
        if not len(args):
            return None
        else:
            if args[0] == 'help':
                return self.helpFtn(*args[1:])
            elif args[0] == 'list':
                return self.listFtn()
            elif args[0] == 'exit':
                return self.exitFtn(*args[1:])
            elif args[0] == 'tone':
                return self.sendTone(*args[1:])
            elif not hasattr(RobotInterface.EngineToRobot, args[0]):
                sys.stderr.write("Unrecognized command \"{}\", try \"help\"{}".format(args[0], os.linesep))
                return False
            else:
                try:
                    params = [eval(a) for a in args[1:]]
                except Exception as e:
                    sys.stderr.write("Couldn't parse command arguments for '{0}':{2}\t{1}{2}".format(args[0], e, os.linesep))
                    return False
                else:
                    t = getattr(RobotInterface.EngineToRobot.Tag, args[0])
                    y = RobotInterface.EngineToRobot.typeByTag(t)
                    try:
                        p = y(*params)
                    except Exception as e:
                        sys.stderr.write("Couldn't create {0} message from params *{1}:{3}\t{2}{3}".format(args[0], repr(params), str(e), os.linesep))
                        return False
                    else:
                        m = RobotInterface.EngineToRobot(**{args[0]: p})
                        return self.send(m)
                    
    def loop(self):
        "Loops read eval print"
        while True:
            try:
                ret = self.REP()
            except EOFError:
                return
            except KeyboardInterrupt:
                return
            else:
                if ret not in (True, False, None):
                    sys.stdout.write("\t{}{}".format(str(ret), os.linesep))

if __name__ == '__main__':
    transport = UDPTransport()
    #transport = UartSimRadio("com4", 115200)
    spi = 5.0 if '-s' in sys.argv else 0.0
    printAll = '-a' in sys.argv
    stateIntervalStats = '-i' in sys.argv
    dryRun = '-n' in sys.argv
    cli = CozmoCLI(transport, ("172.31.1.1", ROBOT_PORT), spi, printAll, stateIntervalStats, dryRun)
    cli.loop()
    del cli
    # cli.transport.KillThread()
