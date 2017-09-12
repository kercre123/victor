import os
import sys
import csv
import argparse
from matplotlib import pyplot
import numpy as np


class SpineEvent:
    def __init__(self,eventid, duration, time):
        self.id = eventid
        self.duration = duration/1e6
        self.time = time/1e6

    def merge(self, duration, time):
        self.duration += duration/1e6
        self.time = time/1e6

EventNames = ['READ','SYNC','FRAME','SEND']
#EventTags = ['R','Wait','Rcv','Run']
EventTags = EventNames
EventColors = ['red','orange','yellow','green']
YSCALE = 10

    
if __name__ == '__main__':
    parser = argparse.ArgumentParser("Spine Time Plotter")
    parser.add_argument('eventfile', type=argparse.FileType('r'), help="event file to parse")

    args = parser.parse_args()
    print("args==",args)

    events=[]
    datareader = csv.reader(args.eventfile )
    print(args.eventfile)
    print(datareader)
    last_time = 0;
    next_duration = 0
    for line in datareader:
        timestamp,eventid = [int(v) for v in line]
        if last_time == 0:
            base_time = timestamp
        else:
            duration = timestamp - last_time;
            if events and eventid == events[-1].id:
                next_duration += duration
                #                events[-1].merge(duration, timestamp-base_time)
            else:
                events.append(SpineEvent(eventid, duration+next_duration, timestamp-base_time))
                next_duration = 0
        last_time = timestamp;


    fig = pyplot.figure()
    ax = fig.add_subplot(111)
        
        
    # for id in range(4):
    #     x = [e.time for e in events if e.id == id]
    #     y = [e.duration for e in events if e.id  == id]
    #     pyplot.plot(x,y, label=EventNames[id])
    #     if id == 3:
    #         avg = []
    #         for i in range(len(y)):
    #             avg.append(np.mean(y[:i]))
    #         pyplot.plot(x,avg, label='running avg SEND')
            
                
    # pyplot.legend(loc='upper right')
#    pyplot.show()



    t = 0;
    xs= []
    ys=[]
    
    for e in events:
        if e.id == 3:
            delta = e.time - t;
            t = e.time
            xs.append(t)
            ys.append(delta) #-e.duration)
#            deltas.append((t,))

    pyplot.plot(xs,ys)

    start = None
    period = []
    for e in events:
        if e.id == 1:
            if not start:
                start = e.time
            period.append(start)
            start+=1000/195.0
    
#    print(period)
    ys=[YSCALE*2 for v in period]
    pyplot.plot(period, ys, marker='*')

#    pyplot.show()

    # data = [[1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0],
    #         [0, 0, 2, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0], 
    #         [0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 3, 0, 3]]
#    data = [[3,0],[4,1],[7,2],[8.1,3],[11.5,0]]

    data = [[e.time,e.id] for e in events]

#    ax.axes.get_yaxis().set_visible()
#    ax.set_aspect(1)

    def avg(a, b):
        return (a + b) / 2.0


    
    lastt = 0
    for pair in data:
        time,event = pair
        x1 = [lastt,time ]
        lastt = time
        y1 = np.array([YSCALE*event, YSCALE*event])
        y2 = y1+YSCALE
#        print(x1,y1,y2)
        pyplot.fill_between(x1, y1, y2=y2, color=EventColors[event])

    LabelNum = 0
    def NextLabel(v):
        global LabelNum
        print(LabelNum)
        r= EventTags[LabelNum//2] if LabelNum%2>0 and LabelNum//2<len(EventTags) else ''
        LabelNum = LabelNum+1
        return r

    
    labels = [NextLabel(item) for item in ax.get_yticklabels()]
    print(labels)
    ax.set_yticklabels(labels)        

    pyplot.ylim(len(EventNames)*YSCALE, 0)
    pyplot.show()    


    print("Average of frame-to-frame = ",np.mean(ys))
        
        
        
