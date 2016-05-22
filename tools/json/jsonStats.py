# This is a simple tool which takes in a bunch of json data which consists of a single list

import argparse
import json
import numbers
import sys

parser = argparse.ArgumentParser(description='computes simple stats over a json list')
parser.add_argument('--filename', '-f', metavar='filename.json', default=None, 
                    help='Reads from filename.json instead of stdin. This file must contain a single json list. stats will be computed on each of the values within the keys, which may be a dictionary. Missing values will not be counted.')
parser.add_argument('--group', '-g', default=None, metavar='groupKey',
                    help='If specified, stats will be computer per value of this key')
parser.add_argument('--json', '-j', dest='jsonOutput', action='store_true', default=False,
                    help='If specified, output in a json format')

args = parser.parse_args()

def cleanJson(s):
    "cleans up the json so it looks like a nice list..."
    clean = s.translate(None, ' \t\r\n')
    if clean[0] != '[':
        clean = '[' + clean + ']'
    clean = clean.replace('}{', '},{')
    clean = clean.replace('},]', '}]')
    return clean
    

if args.filename:
    infile = open(args.filename, 'r')
else:
    infile = sys.stdin

# read the entire file (so it works with stdin)
jsonStr = cleanJson(infile.read())

J = json.loads(jsonStr, strict=False)

if not args.filename:
    infile.close()


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
                

if args.group == None:
    grouped[''] = data

import numpy

outputJ = {}

maxCount=0

for group in grouped:
    if group:
        outputJ[group] = {}
        groupOutput = outputJ[group]
    else:
        groupOutput = outputJ

    sortedKeys = grouped[group].keys()
    if args.group and args.group in sortedKeys:
        sortedKeys.remove(args.group)
        sortedKeys.insert(0, args.group)

    for key in sortedKeys:
        groupOutput[key] = {}

        if grouped[group][key]:
            count = len(grouped[group][key])
            if count > maxCount:
                maxCount = count

            if isinstance(grouped[group][key][0], numbers.Number):
                groupOutput[key]["count"] = count
                groupOutput[key]["mean"] = numpy.mean(grouped[group][key])
                groupOutput[key]["std"] = numpy.std(grouped[group][key])
            else:
                u = set(grouped[group][key])
                groupOutput[key]["count"] = count
                groupOutput[key]["unique"] = len(u)
                if len(u) == 1:
                    groupOutput[key]["value"] = grouped[group][key][0]

if args.jsonOutput:
    print json.dumps(outputJ, indent=2)
else:
    if args.group:
        # gather up keys as columsn, assume the first one has all of them
        cols = None
        widths = None
        groupWidth = max( [ len(x) for x in outputJ.keys() ] )
        for group in outputJ:
            if not cols:
                cols = outputJ[group].keys()
                cols.remove(args.group)
                
                widths = [ max( 10, len(x)) for x in cols ]
                header = args.group.rjust(groupWidth) + ' '
                for c,w in zip(cols, widths):
                    header += c.rjust(w) + ' '
                print header

            line = group.rjust(groupWidth) + ' '
            for c, w in zip(cols, widths):
                if c in outputJ[group]:
                    if "value" in outputJ[group][c]:
                        line += outputJ[group][c]["value"].rjust(w) + ' '
                    elif "mean" in outputJ[group][c]:
                        mean = outputJ[group][c]["mean"]
                        if mean % 1.0 < 1e-3:
                            line += ("%6d    " % mean).rjust(w) + ' '
                        else:
                            line += ("%10.3f" % mean).rjust(w) + ' '
                    elif "unique" in outputJ[group][c]:
                        line ++ ("[%d]" % outputJ[group][c]["unqiue"]).rjust(w) + ' '
                    else:
                        line += w*' ' + ' '
                else:
                    line += w*' ' + ' '
            print line
    else:
        print "default count:", maxCount

        for k in outputJ:
            line = "%20s: " % k
            if "mean" in outputJ[k]:
                line += "%10.3f " % outputJ[k]["mean"]
                if "std" in outputJ[k] and outputJ[k]["std"] > 1e-8:
                    line += "+/- %f " % outputJ[k]["std"]
            elif "value" in outputJ[k]:
                line += outputJ[k]["value"]
            elif "unique values" in outputJ[k]:
                line += "%4d unique " % outputJ[k]["unique"]
            if "count" in outputJ[k] and outputJ[k]["count"] != maxCount:
                line += "(count %d) " % outputJ[k]["count"]
            print line
    

    
