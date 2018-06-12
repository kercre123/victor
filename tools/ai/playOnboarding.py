#!/usr/bin/env python
import curses
import sys
import os
import select
import subprocess
import signal
import collections #for deque (circular buffer)
import requests
import time

def processLine(line):
  segments = line.split(' ')
  if len(segments)<2 or 'onboardingstatus' not in line.lower():
    return False,'','',''
  dateTime = segments[0] + ' ' + segments[1]
  channel = '[@Behaviors]'
  threadSeparator = '(tc'
  if channel in line:
    channelIdx = line.index(channel) 
    line = line[channelIdx + len(channel):]
    if threadSeparator in line:
      tcIdx = line.index(threadSeparator)
      msgName = line[:tcIdx]
      endSep = line[tcIdx:].index(')')
      endSep += tcIdx
      return True,dateTime, msgName, line[endSep+1:]
    else:
      return True,dateTime, line,''
  else:
    return False,dateTime,'',''

def padToLength(s, length):
  stringLen = len(s)
  if stringLen<length:
    return s.ljust(length)
  return s
  
def drawMenu(window, options, selectedIndex):
  idx = 0
  for opt in options:
    if selectedIndex == idx:
      window.attron(curses.color_pair(2))
    if isinstance(opt, tuple):
      entry = padToLength('[' + opt[0] + ']   ' + opt[1], 30)
    else:
      entry = padToLength(opt, 30)
    window.addstr(idx+1,2, entry)
    if selectedIndex == idx:
      window.attroff(curses.color_pair(2))
    idx += 1

def sendFunc(robotIP, func):
  try:
    uri = 'http://' + robotIP + ':8888/consolefunccall?func=' + func
    res = requests.get(uri)
    return res.status_code == 200
  except:
    return False

def sendRenameFace(robotIP, newName):
  return sendFunc(robotIP, 'RenameFace&args=' + newName)

def sendMoveToStage(robotIP, stageNum):
  # need to set a var then call func
  try:
    uri = 'http://' + robotIP + ':8888/consolevarset?key=DevMoveToStage&value=' + str(stageNum)
    res = requests.get(uri)
  except:
    return False
  return sendFunc(robotIP, 'MoveToStage')

_gameIndex = -1
def updateCaption(bottomBox, maxWidth):
  global _gameIndex
  _gameIndex += 1
  gameStrings = [' You are standing in an open field west of a white house, with a boarded front door. There is a small mailbox here. What do you do? ',
                 ' You are facing the north side of a white house. There is no door here, and all the windows are boarded up. To the north, a narrow path winds through the trees. ',
                 ' This is a path winding through a dimly lit forest. You hear in the distance the chirping of a song bird. ',
                 ' You are in a clearing, with a forest surrounding you on all sides. A path leads south. On the ground is a pile of leaves. ']
  if _gameIndex >= len(gameStrings):
    _gameIndex = 0
  bottomBox.box()
  gameStr = gameStrings[_gameIndex]
  bottomBox.addstr(0, 2, gameStr[:maxWidth])
  bottomBox.refresh()

def display(stdscr):
  robotIP = os.environ['ANKI_ROBOT_HOST']
  if robotIP is None:
    print('Robot IP must be set!     export ANKI_ROBOT_HOST="x.x.x.x"')
    sys.exit(1)

  path = subprocess.check_output(['git','rev-parse','--show-toplevel']).strip()
  command = path + '/project/victor/scripts/victor_log.sh'
  readProcess = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, shell=True, preexec_fn=os.setsid)

  stdscr.clear()
  stdscr.timeout(500)

  maxy, maxx = stdscr.getmaxyx()
  curses.newwin(2,maxx,3,1)

  # invisible cursor
  curses.curs_set(0)

  if curses.has_colors():
    curses.start_color()
    curses.use_default_colors()
    curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_WHITE)

  stdscr.refresh()

  

  options = [('q', 'Quit'),
             ('c', 'Continue', 'Continue'), #3rd elem is console func name, if applicable
             ('s', 'Skip', 'Skip'),
             ('m', 'Move to stage', 'MoveToStage'),
             ('r', 'Reset onboarding', 'ResetOnboarding'),
             ('b', 'retry Battery charging', 'RetryCharging'),
             ('f', 'rename Face')]
  stages = ['NotStarted','FinishedComeHere','FinishedMeetVictor','Complete','DevDoNothing']

  bottomHeight = 12
  selectedIndex = 0
  menuMode = 'main' # or rename or stages
  menu = options

  bottomBox = curses.newwin(bottomHeight,maxx-2,maxy-bottomHeight,1)
  updateCaption( bottomBox, maxx - 4 )
  bottomWindow = curses.newwin(bottomHeight-2,maxx-4,maxy-bottomHeight+1,2)

  topHeight = maxy - bottomHeight
  topBox = curses.newwin(topHeight,maxx-2,0,1)
  topBox.box()
  topBox.addstr(0,2,' Onboarding status of robot ' + robotIP + ' ')
  topBox.refresh()
  topWindow = curses.newwin(topHeight-2,maxx-4,1,2)

  drawMenu(bottomWindow, menu, selectedIndex)

  bottomWindow.refresh()
  

  outputLines = collections.deque(maxlen=topHeight - 4)

  newName = ''

  keepRunning = True
  while keepRunning:

    changed = False
    while sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
      char = sys.stdin.read(1)

      if menuMode == 'main' or menuMode == 'stages':
        if char == '\x1b':
          arrow = sys.stdin.read(2)
          if arrow == 'OB':
            # down, platform specific!
            selectedIndex = min(len(menu)-1, selectedIndex+1)
            changed = True
          elif arrow == 'OA':
            # up
            selectedIndex = max(0, selectedIndex-1)
            changed = True

      if menuMode == 'rename':
        if char == ' ':
          sendRenameFace(robotIP, newName)
          newName = ''
          menuMode = 'main'
          bottomWindow.clear()
          updateCaption( bottomBox, maxx - 4 )
          changed = True
        else:
          newName += char
          bottomWindow.addstr(1,20,newName)
          changed = True
      elif menuMode == 'main':
        if char == ' ':
          char = menu[selectedIndex][0]

        if char == 'q':
          keepRunning = False
        elif char == 'm':
          menuMode = 'stages'
          bottomWindow.clear()
          menu = stages
          selectedIndex = 0
          changed = True
        elif char == 'f':
          bottomWindow.clear()
          menuMode = 'rename'
          bottomWindow.addstr(1,2,'Enter new name: ')
          newName = ''
          selectedIndex = 0
          changed=True
        elif char in 'csrb':
          func = [x for x in menu if x[0] == char][0][2]
          sendFunc(robotIP, func)
          selectedIndex = 0
          changed=True
          updateCaption( bottomBox, maxx - 4 )
      elif menuMode == 'stages':
        if char == ' ':
          sendMoveToStage(robotIP, selectedIndex)
          bottomWindow.clear()
          selectedIndex = 0
          menu = options
          updateCaption( bottomBox, maxx - 4 )
          menuMode = 'main'
          changed = True


    if changed:
      if menuMode == 'main' or menuMode == 'stages':
        drawMenu(bottomWindow, menu, selectedIndex)
      bottomWindow.refresh()

    changed = False
    while readProcess.stdout in select.select([readProcess.stdout], [], [], 0)[0]:
      line = readProcess.stdout.readline()
      parsed,dateTime,msgName,msgDetail = processLine(line)
      if parsed:
        dateTime = padToLength(dateTime, 20)
        msgName = padToLength(msgName, 50)
        msgDefailt = padToLength(msgDetail,30)
        line = dateTime + msgName + msgDetail
        outputLines.append(line[:(maxx - 4)])
        changed = True
    if changed:
      topWindow.attron(curses.color_pair(1))
      for i in xrange(len(outputLines)):
        topWindow.addstr(1+i,2,outputLines[i])
      topWindow.attroff(curses.color_pair(1))
      topWindow.refresh()
    time.sleep(0.1)

  # kill the victor_log process
  os.killpg(os.getpgid(readProcess.pid), signal.SIGTERM)

def main():
  if len(sys.argv)>=2 and '-h' in sys.argv[1]:
    print('Tool for onboarding. Use space to select options.')
    sys.exit(0)
  curses.wrapper(display)
  print('Thanks for playing')

if __name__ == '__main__':
  main()