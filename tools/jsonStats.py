# This is a simple tool which takes in a bunch of json data which consists of a single list

import argparse
import json
import numbers
import sys

parser = argparse.ArgumentParser(description='computes simple stats over a json list')
parser.add_argument('filename', metavar='filename.json', default=None,
                    help='Reads filename.json (default stdin). This file must contain a single json list. stats will be computed on each of the values within the keys, which may be a dictionary. Missing values will not be counted.')
parser.add_argument('--group', default=None, metavar='groupKey',
                    help='If specified, stats will be computer per value of this key')

args = parser.parse_args()

if args.filename:
    infile = open(args.filename, 'r')
else:
    infile = sys.stdin

J = json.load(infile)

# for each key, this holds a list of values
data = {}

# for each group, hold the same stuff as in data
grouped = {}

for obj in J:
    for k in obj:
        v = obj[k]
        if k not in data:
            data[k] = [v]
        else:
            data[k].append(v)

    if args.group:
        if args.group in obj:
            g = obj[args.group]
            if g not in grouped:
                grouped[g] = {}

            for k in obj:
                v = obj[k]
                if k not in grouped[g]:
                    grouped[g][k] = [v]
                else:
                    grouped[g][k].append(v)
                

# import pprint
# pprint.pprint(data)

if args.group == None:
    grouped[''] = data

import numpy

for group in grouped:
    if group:
        print ""

    sortedKeys = grouped[group].keys()
    if args.group and args.group in sortedKeys:
        sortedKeys.remove(args.group)
        sortedKeys.insert(0, args.group)

    for key in sortedKeys:
        if grouped[group][key]:
            if isinstance(grouped[group][key][0], numbers.Number):
                # do number stats
                print "%20s: count:%3d, avg:%11.3f, std:%f" % (
                    key,
                    len(grouped[group][key]), 
                    numpy.mean(grouped[group][key]),
                    numpy.std(grouped[group][key]) )
            else:
                u = set(grouped[group][key])
                if len(u) == 1:
                    print "%20s: count:%3d, '%s'" % (
                        key,
                        len(grouped[group][key]),
                        grouped[group][key][0])

                else:
                    print "%20s: count:%3d, unique:%3d" % (
                        key,
                        len(grouped[group][key]),
                        len(u))

if infile != sys.stdin:
    infile.close()

    
