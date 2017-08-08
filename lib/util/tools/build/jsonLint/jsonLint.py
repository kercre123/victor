#!/usr/bin/env python2

import sys
import os
import re
import argparse

scriptPath = os.path.dirname(os.path.realpath(__file__))
sys.path.append(scriptPath + "/demjson")
import demjson


parser = argparse.ArgumentParser(description='check json files for vailidty')
parser.add_argument('-v', default=False, action='store_true',
                    help='verbose. List files being checked')
parser.add_argument('directory', default='resources', nargs='?',
                    help='directory to search (recursively) for .json files')

args = parser.parse_args()

JS = demjson.JSON()
JS._set_strictness(True)
JS.allow("comments")

jsonDir = args.directory
match = ".json"

def findWholeWord(w):
    return re.compile(r'\b({0})\b'.format(w), flags=re.IGNORECASE).search

def checkFile(filename):
    with open(filename, 'r') as f:
        data = ""
        origData = ""
        linenum = 1
        for line in f:
            origData = origData + line
            if line[-1] != '\n':
                line = line + '\n'
            c = 0
            modline = ""
            while c >= 0:
                oldC = c
                c = line.find('\"', c+1)
                if c > 0:
                    c = line.find('\"', c+1)
                if c > 0:
                    modline = modline + line[oldC:c] + "##" + str(linenum)
                else :
                    modline = modline + line[oldC:]
                
                # print linenum, c, modline

            # data = data + line[:-1] + 
            data = data + modline[:-1] + "// ##" + str(linenum) + "\n"
            linenum = linenum + 1
        # print data
        try:
            JS.decode(data)
        except demjson.JSONDecodeError as e:            
            # print e
            err = e[1]
            msg = e[0]
            n = err.find('##')
            endN = err.find('\n', n+1)
            if endN < 0:
                endN = len(err)
            line = err[n+2:endN]
            line = line.strip()
            line = line.replace("\n", "")
            line = line.replace("\"", "")
            if len(line) == 0:
                # try to find the messed up thing to identify the line
                reg = findWholeWord(e[1])(origData)
                if reg == None:
                    line = "0"
                    msg = msg + ": " + e[1]
                else:                    
                    n = reg.start(0)
                    print n
                    print e[1]
                    print reg.group(0)
                    line = str(origData[:n].count('\n'))
            print os.getcwd() + "/" + filename + ":" + line + ": error: "+msg
            return True
        else:
            if args.v:
                print filename + " JSON valid!"
            return False



hasError = False

numChecked = 0

if args.v:
    print "walking directory at '%s' searching for files ending in '%s'" % ( jsonDir, match )        

for root, dirs, files in os.walk(jsonDir):
    # print root
    # print files
    for file in files:
        matchIdx = file.find(match)
        if matchIdx > 0 and matchIdx == len(file) - len(match) and file.find('/.#') == -1:
            numChecked += 1
            if checkFile(root + "/" + file):
                hasError = True
if hasError:
    if args.v:
        print "error: some files failed the json check!"
    sys.exit(-1)
else:
    if args.v:
        print "Done. Checked %d files, no errors" % numChecked
