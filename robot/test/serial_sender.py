import sys
import time
import serial
import random

PERIOD=1.0/200

framecount = 1;

HEADER = "B2H"
extension = "ABCDEFGHIJKLMNOPQRSTUVWXYz"*7  #+"abcdefghijk"

start = time.time()
target = start + PERIOD
while True:
    now = time.time()
    payload = "|{}@{:.6f}{}".format(framecount,now-start,extension)
    if random.randint(0,6000)==0:
        payload = payload+"Z"
    framecount+=1
    out = "{}{}{}".format(HEADER,chr(len(payload)),payload)
    print(out)
#    if random.randint(0,9)==0:
#        print("..."*random.randint(1,8))
    if (target-now>0):
        time.sleep(max(target-now,0))
    target += PERIOD
