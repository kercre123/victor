"""
Python library for streaming animations to the robot
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, math
import robotInterface, muencode
Anki = robotInterface.Anki

class AnimationStreamer:
    
    BUFFER_SIZE = Anki.Cozmo.AnimConstants.KEYFRAME_BUFFER_SIZE
    
    def getMore(self, available):
        raise Exception("AnimationStreamer subclasses must implement getMore method")
    
    def handleAnimState(self, asm):
        "Process incoming animation state messages"
        if self.startingNumBytesPlayed is None:
            self.startingNumBytesPlayed = asm.numAnimBytesPlayed
        self.numBytesPlayed = asm.numAnimBytesPlayed - self.startingNumBytesPlayed
        #print("AnimState: {}".format(str(asm)))
        self.getMore(self.bufferAvailable)
        
    @property
    def bufferAvailable(self):
        return self.BUFFER_SIZE - (self.numBytesSent - self.numBytesPlayed)
    
    def __init__(self):
        self.numBytesPlayed = 0
        self.numBytesSent   = 0
        self.startingNumBytesPlayed = None
        self.haveStarted    = False
        robotInterface.SubscribeToTag(Anki.Cozmo.RobotInterface.RobotToEngine.Tag.animState, self.handleAnimState)

    def send(self, msg, tag=0):
        size = len(msg.pack())
        #print(size, self.numBytesSent, self.numBytesPlayed, self.bufferAvailable)
        assert self.bufferAvailable >= size
        self.numBytesSent += size
        robotInterface.Send(msg)
        if not self.haveStarted:
            self.haveStarted = True
            soa = Anki.Cozmo.AnimKeyFrame.StartOfAnimation(tag)
            self.send(Anki.Cozmo.RobotInterface.EngineToRobot(animStartOfAnimation=soa))
            
    def end(self):
        eoa = Anki.Cozmo.AnimKeyFrame.EndOfAnimation()
        self.send(Anki.Cozmo.RobotInterface.EngineToRobot(animEndOfAnimation=eoa))
        self.haveStarted = False

class ToneStreamer(AnimationStreamer):
    "Sends a constant tone to the robot"

    CHUNK_SIZE = Anki.Cozmo.AnimConstants.AUDIO_SAMPLE_SIZE

    def getMore(self, available):
        "Sends tone data"
        if self.tonerator is None:
            return
        elif self.tonerator is False:
            self.tonerator = None
            self.end()
        else:
            while self.bufferAvailable > (self.CHUNK_SIZE + self.BUFFER_SIZE*0.1):
                samples = next(self.tonerator)
                if not samples:
                    self.tonerator = None
                    self.end()
                    break
                else:
                    self.sendSamples(samples)
                
    def sendSamples(self, samples):
        audioSamples = Anki.Cozmo.AnimKeyFrame.AudioSample(samples)
        self.send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSample=audioSamples))

    def __init__(self):
        self.tonerator = None
        AnimationStreamer.__init__(self)
        
    def start(self, frequency=440):
        def ToneGenerator(period, length):
            p = 0.0
            while True:
                samples = []
                while len(samples) < length:
                    samples.append(muencode.encodeSample(int((math.sin(p))*0x7f00)))
                    p += 2*math.pi/period
                yield samples
        self.tonerator = ToneGenerator(Anki.Cozmo.AnimConstants.AUDIO_SAMPLE_RATE/frequency, self.CHUNK_SIZE)
        
    def stop(self):
        self.tonerator = False
    
