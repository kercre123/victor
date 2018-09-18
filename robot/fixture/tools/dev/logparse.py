#! python
import sys, os, glob
import re
import time
from tqdm import tqdm

#globals
class g:
  directory = os.path.dirname(os.path.realpath(__file__))
  debug = 0
  currentFile = ""
  fileCnt = 0
  currentLine = 0
  lineCnt = 0
  parsers = None # List of enabled parser classes
  parseTread = 0
  parseRange = 0
  parsePPlog = 0

#======================================================================
#   Regex
#======================================================================

def match( text, pattern ):
  p = re.compile(pattern)
  m = p.search(text) #m = re.search(pattern, text)
  match = m.groups()[0] if m else ""
  #print( "match = '{}'".format(match) )
  return match

def match_value(text, compiled_pattern, err_value):
  m = compiled_pattern.search(text)
  value = m.groups()[0] if m else err_value
  #print("match_value: {}".format(value))
  return value

def match_result(text):
  m = rePattern.result.search(text)
  rval = m.groups()[0] if m else -1
  #print("match_result: {}".format(rval))
  return rval

def match_mode(text):
  m = rePattern.mode.search(text)
  modeNum = m.groups()[0] if m and m.groups()[0] else -1
  modeName = m.groups()[1] if m and m.groups()[1] else "UNKNOWN"
  #print("match_mode: num={} name={}".format(modeNum, modeName))
  return [modeNum, modeName]
  
#precompiled patterns
class rePattern:
  result = re.compile('RESULT:([0-9]+)') #sample: [014.747754] [RESULT:000]
  mode = re.compile('mode,([0-9]+),(.+)') #sample: [000.005468] fixture,fw,030,release,mode,33,PACKOUT
  
#======================================================================
#   Functions
#======================================================================

def file_len(fname):
  i=-1
  with open(fname) as f:
    for i, l in enumerate(f): pass
  return i + 1

#======================================================================
#   Parser: Tread
#======================================================================
filebase_tread="robotmotor_tread";

def tread_reset():
  if g.debug: print("tread: cleaning output files")
  for fname in glob.glob("./{}*.csv".format(filebase_tread)):
    if g.debug: print("  os.remove({})".format(fname))
    os.remove(fname)
  return 0

def tread_start():
  return
  
def tread_line(line):
  return
  
def tread_result(result):
  return
  
def tread_end():
  return

#======================================================================
#   Parser: Range
#======================================================================
filebase_range="robotmotor_range";

def range_reset():
  if g.debug: print("range: cleaning output files")
  for fname in glob.glob("./{}*.csv".format(filebase_range)):
    if g.debug: print("  os.remove({})".format(fname))
    os.remove(fname)
  return 0

def range_start():
  return
  
def range_line(line):
  return
  
def range_result(result):
  return
  
def range_end():
  return

#======================================================================
#   Parser: PPLog
#======================================================================
filebase_pplog="pplog";

def pplog_reset():
  if g.debug: print("pplog: cleaning output files")
  for fname in glob.glob("./{}*.csv".format(filebase_pplog)):
    if g.debug: print("  os.remove({})".format(fname))
    os.remove(fname)
  return 0

def pplog_start():
  return

def pplog_line(line):
  return

def pplog_result(result):
  return
  
def pplog_end():
  return
  
#======================================================================
#   Global Parser
#======================================================================

def parser_reset():
  if g.parseTread: tread_reset()
  if g.parseRange: range_reset()
  if g.parsePPlog: pplog_reset()

def parser_start():
  if g.parseTread: tread_start()
  if g.parseRange: range_start()
  if g.parsePPlog: pplog_start()

def parser_line(line):
  if g.parseTread: tread_line(line)
  if g.parseRange: range_line(line)
  if g.parsePPlog: pplog_line(line)

def parser_result(result):
  if g.parseTread: tread_result(result)
  if g.parseRange: range_result(result)
  if g.parsePPlog: pplog_result(result)

def parser_end():
  if g.parseTread: tread_end()
  if g.parseRange: range_end()
  if g.parsePPlog: pplog_end()

def parse_file(fname):
  g.currentFile=fname
  g.currentLine=0
  print( "processing {1}".format( g.currentFile, os.path.join(g.directory, g.currentFile) ) )
  numlines = file_len(g.currentFile)
  progressbar = tqdm(total=numlines)
  displayMult = 500 #throttle progress updates so they don't register as significant processing overhead
  
  started=0
  with open(g.currentFile) as f:
    for line in f:
      g.currentLine += 1
      if "[TEST:START]" in line: parser_start(); started=1
      if started: parser_line(line)
      if "fixture,fw" in line:
        modeNum, modeName = match_mode(line)
      if "[RESULT:" in line:
        rval = match_result(line)
        parser_result(rval)
      if "[TEST:END]" in line: parser_end()
      if g.currentLine % displayMult == 0: progressbar.update(displayMult)
  
  progressbar.update(g.currentLine % displayMult)
  progressbar.close()
  g.fileCnt += 1
  g.lineCnt += g.currentLine
  return

#======================================================================
#   Program Entry
#======================================================================
#cli usage: logparse [option] [parser1] [parserN]

argCnt=len(sys.argv); showhelp=0
for i in range(1,argCnt): #arg0 is script name
  arg = sys.argv[i]
  if arg.lower()=="debug": g.debug=1
  if arg.lower()=="--help": showhelp=1
  if arg.lower()=="tread": g.parseTread=1
  if arg.lower()=="range": g.parseRange=1
  if arg.lower()=="pplog": g.parsePPlog=1
  if g.debug: print("arg[" + str(i) +"]='" + sys.argv[i] + "'")

#if g.debug: print("arg[0]='" + sys.argv[0] + "'")
if g.debug: print( "argCnt={} parseRange={} parseTread={} parsePPlog={} showhelp={}".format(argCnt, g.parseRange, g.parseTread, g.parsePPlog, showhelp) )

if showhelp > 0 or (g.parseRange==0 and g.parseTread==0 and g.parsePPlog==0):
  print("Usage: logparse.py [option] [parser1] [parserN]")
  print("BREIF: parses logfiles from ROBOT fixtures")
  print("Options:")
  print("  --help     display this help message")
  print("  tread      parse tread data for Left + Right treads")
  print("  range      parse range data for Head + Lift motors")
  print("  pplog      parse playpen log data")
  print("  debug      enable debug printing")
  print("")
  if showhelp > 0: sys.exit(0)
  if g.debug == 0: sys.exit(1)

os.chdir(g.directory) #always start from script directory
parser_reset() #reset all enabled parsers

#parse logfiles (*.log or *.txt formats)
Tstart = time.time()
for fname in os.listdir(g.directory):
  if fname.endswith(".log") or fname.endswith(".txt"): parse_file(fname)
Telapsed = time.time() - Tstart

if g.lineCnt==0: g.lineCnt = 1
print( "processed {} files, {} lines in {:.3f}s, avg {:.3f}us per line".format(g.fileCnt, g.lineCnt, Telapsed, Telapsed/g.lineCnt*1000000) )
sys.exit(0)

