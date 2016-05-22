#!/usr/bin/python

# Simple script which reads in a motion primitive and outputs (to
# stdout) the same primitive, but with the resultion set to the given
# value, and all actions scaled to be the same (meaning the actions
# get longer if you increase the size of the cells)

import sys

if len(sys.argv) != 3:
    print "usage:",sys.argv[0]," input.mprim new_resolution"
    exit(-1)

factor = None;
newRes = float(sys.argv[2])

with open(sys.argv[1]) as file:
    for line in file:
        if line.find("resolution_m: ") == 0:
            oldRes = float(line[len("resolution_m: "):])
            factor = newRes / oldRes

        numThisLine = 0
        for word in line.split(' '):
            if word.find('.') >= 0:
                numThisLine = numThisLine + 1
                if numThisLine == 3:
                    print word,
                else:
                    f = float(word)
                    if word[-1] == '\n':
                        print f*factor
                    else:
                        print f*factor,
            else:
                print word,




