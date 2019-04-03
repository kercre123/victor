#!/usr/bin/env python2

import csv
import numpy as np
import sys



filename = sys.argv[1]

validVADS = ["WebRTC","Vector"]#,"Alexa"]

# how often there is a false activation
# how often there is NO activation
# how often at least one activation
# delay activating (account for r/t time) for first activation after start of sound
# delay deactivating (account for r/t time) for last activation. bell curve

class TimeMap:
    def __init__(self):
        self.x = np.array([])
        self.y = np.array([])
    def Add(self,x,y):
        self.x = np.append(self.x, x)
        self.y = np.append(self.y, y)
    def Interp(self, x):
        return np.interp(x, self.x, self.y)

rtTimes = TimeMap()

jobDesc = ""

audioTimes = []
vadTimes = []
activations = dict()

hasVoice = False
voiceChan = -1

numFalseAllActivations = 0
numSkipAllActivations = 0
numJobs = 0

def AnalyzeJob():
    global numSkipAllActivations
    global numFalseAllActivations
    global numJobs
    numJobs += 1
    print("JOB: " + str(jobDesc))
    
    # look at when all VADs are active/inactive
    allActive = []
    times = []
    delta = []
    names = []
    for key,val in activations.items():
        for v in val:
            times.append(v[0])
            times.append(v[1])
            delta.append(1)
            delta.append(-1)
            names.append(key)
            names.append(key)
    zipped = zip(times, delta, names)
    zipped.sort(key = lambda t: t[0])
    times = [t for t,d,n in zipped]
    delta = [d for t,d,n in zipped]
    names = [n for t,d,n in zipped]
    #max difference in activation time

    cum = dict()
    startAll = -1
    endAll = -1
    startDiff = float('inf')
    for i in xrange(len(delta)):
        

        if delta[i] == 1:
            cum[names[i]] = times[i]
            if len(cum)>=len(validVADS):
                startDiffHere = -float('inf')
                for k1,v1 in cum.items():
                    for k2,v2 in cum.items():
                        if k2 < k1:
                            diff = abs(v1-v2)
                            if diff > startDiffHere:
                                startDiffHere = diff
                if startDiffHere <= startDiff:
                    startDiff = startDiffHere
                

        if delta[i] == -1:
            del cum[names[i]]
        assert(len(cum) <= len(activations))
        if len(cum) >= len(validVADS):
            if startAll < 0:
                startAll = times[i]
                
        elif startAll >= 0:
            endAll = times[i]
            if endAll - startAll > 1000:
                if startDiff <= 500:
                    allActive.append([startAll, endAll])
                else: 
                    print("max start diff="+ str(startDiff))
            #print("ALL ACTIVE during " + str([startAll, endAll]) )
            startAll = -1
                

    
    if not hasVoice:
        # how often there is a false activation
        for key,val in activations.items():
            if len(val)>0:
                print(key + " had " + str(len(val)) + " FALSE activations")
        if len(allActive)>0:
            print(str(len(allActive)) + " FALSE ALL_ACTIVATIONS")
            numFalseAllActivations += 1
    else:
        if len(allActive)==0:
            print("NO ALL_ACTIVATIONS")
            numSkipAllActivations += 1
        for key,val in activations.items():
            if len(val) == 0:
                # how often there is NO activation
                print(key + " had NO activations despite there being a voice")
            else:
                if len(vadTimes) == 0:
                    print("NO VAD!")
                    sys.exit(1)
                # ignore stops and starts since some users speak slowly
                actualVad = [vadTimes[0][0], vadTimes[len(vadTimes)-1][1]]
                numTooEarly = 0
                numTooLate = 0
                idxFirst = 0
                idxLast = -1
                physDelay = rtTimes.Interp(actualVad[0])*0.5
                for v in val:
                    if v[0]+physDelay < actualVad[0]:
                        numTooEarly += 1
                        idxFirst += 1
                    if v[0]+physDelay < actualVad[1]:
                        idxLast += 1
                    else:
                        numTooLate += 1
                if startDiff > 500:
                    print("overlap startDiff=" + str(startDiff))
                    numSkipAllActivations += 1
                else:
                    if idxFirst < len(val):
                        print(key + " had " + str(len(val)) + " activations with voice (" + str(len(vadTimes)) + "), " + str(numTooEarly) + " too early and " + str(numTooLate) + " too late")
                        detectedVad = [val[idxFirst][0], val[idxLast][1]]
                        delay = detectedVad[0] - actualVad[0] - physDelay
                        #print(delay)
                    else:
                        print(key + " had " + str(len(val)) + " activations TOO EARLY despite there being a voice")



with open(filename) as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    lineCount = 0
    activationStart = {a: -1 for a in validVADS}
    for row in csv_reader:
        epochTime = int(row[1])
        if row[0] == 'job':
            if lineCount > 0:
                AnalyzeJob()

            hasVoice = False
            for rowStr in row[3:]:
                if "Voice" in rowStr:
                   hasVoice = True
            audioTimes = []
            vadTimes = []
            activations = {a:[] for a in validVADS}
            jobDesc = row[3:]
            voiceChan = -1
            vadStart = -1
        elif row[0] == 'audioStart':
            if "Voice" in row[2]:
                audioStartTime = epochTime
                voiceChan = int(row[3])
        elif row[0] == 'audioEnd':
            if "Voice" in row[2]:
                audioEndTime = epochTime
                audioTimes.append([audioStartTime, audioEndTime])
        elif row[0] == 'audioVad':
            if int(row[2]) == voiceChan:
                if int(row[3]) == 1:
                    vadStart = epochTime
                elif int(row[3]) == 0 and vadStart >= 0.0:
                    vadEnd = epochTime
                    vadTimes.append([vadStart, vadEnd])
        elif row[0] == 'time':
            rtTime = int(row[5])
            rtTimes.Add(epochTime, rtTime)
        elif row[0] == 'VAD':
            if row[3] in validVADS:
                if int(row[4]) == 1: # active
                    activationStart[row[3]] = epochTime
                else:
                    activationEnd = epochTime
                    activations[row[3]].append([activationStart[row[3]], activationEnd])
        # elif row[0] == 'noise':
        # elif row[0] == 'trigger':


        lineCount += 1
    AnalyzeJob()
    print("Processed {0} lines.", lineCount)
print("numSkipAllActivations=" + str(numSkipAllActivations) + "/" + str(numJobs) + "=" + str(100*float(numSkipAllActivations)/numJobs) + "%")
print("numFalseAllActivations=" + str(numFalseAllActivations) + "/" + str(numJobs) + "=" + str(100*float(numFalseAllActivations)/numJobs) + "%")