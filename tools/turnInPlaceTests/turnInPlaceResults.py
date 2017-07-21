#! /usr/bin/env python2

import os
import tarfile
from datetime import datetime
import re
from scanf import scanf
import matplotlib.pyplot as plt
import json
import numpy as np

logsDir = '../../build/mac/Debug/playbackLogs/webotsCtrlGameEngine/gameLogs/devLogger'
jsonDumpDir = 'testLogs'

def getMostRecentTarLog():
    logFiles = [f for f in os.listdir(logsDir) if f.endswith('.tar.gz')]
    datetimes = {}
    for file in logFiles:
        datetimes[file] = datetime.strptime(file[:19], '%Y-%m-%dT%H-%M-%S')
    logFiles.sort(key=datetimes.__getitem__, reverse=True)
    return os.path.join(logsDir, logFiles[0])

useCurrentOpenLog = True # if true, use untarred current log directory. if false, use most recent tar

tar = None
if useCurrentOpenLog:
    # get the first (and hopefully only) subdirectory
    logDir = next(os.walk(logsDir))[1][0]
    logFile = os.listdir(os.path.join(logsDir, logDir, 'print'))[0]
    fobj = open(os.path.join(logsDir, logDir, 'print', logFile))
else:
    logFileTar = getMostRecentTarLog()
    tar = tarfile.open(logFileTar)
    for member in tar:
        if member.name[:7] == '/print/':
            logFile = member
    fobj = tar.extractfile(logFile)

tests = []
for ln in fobj:
    res = scanf("%*s %*s %*s %*s BehaviorDevTurnInPlaceTest.TestStart %*s : index %d, run %d, angle_deg %f, speed_deg_per_sec %f, accel_deg_per_sec2 %f, tol_deg %f, startingHeading_deg %f", ln)
    if res:
        test = {}
        test['index'] = res[0]
        test['run'] = res[1]
        test['angle_deg'] = res[2]
        test['speed_deg_per_sec'] = res[3]
        test['accel_deg_per_sec2'] = res[4]
        test['tol_deg'] = res[5]
        test['startingHeading_deg'] = res[6]
        test['timestamp'] = []
        test['currAngle'] = []
        test['desAngle'] = []
        test['currSpeed'] = []
        test['desSpeed'] = []
        test['arcVel'] = []
        test['ff_term'] = []
        test['p_term'] = []
        test['i_term'] = []
        test['d_term'] = []
        test['errSum'] = []
        tests.append(test)

    res = scanf("%*d %*d %*s %*s RobotFirmware.SteeringController.ManagePointTurn.Controller %*s : timestamp %d, currAngle %f, desAngle %f, currSpeed %f, desSpeed %f, arcVel %d, ff %f, p %f, i %f, d %f, errSum %f", ln)
    if res and tests:
        tests[-1]['timestamp'].append(res[0])
        tests[-1]['currAngle'].append(res[1])
        tests[-1]['desAngle'].append((res[2]+180) % 360 - 180) #unwrap large angles
        tests[-1]['currSpeed'].append(res[3])
        tests[-1]['desSpeed'].append(res[4])
        tests[-1]['arcVel'].append(res[5])
        tests[-1]['ff_term'].append(res[6])
        tests[-1]['p_term'].append(res[7])
        tests[-1]['i_term'].append(res[8])
        tests[-1]['d_term'].append(res[9])
        tests[-1]['errSum'].append(res[10])

    res = scanf("%*s %*s %*s %*s BehaviorDevTurnInPlaceTest.TestComplete %*s : index %d, run %d, finalHeading_deg %f", ln)
    if res and tests:
        tests[-1]['finalHeading_deg'] = res[2]

fobj.close()
if tar:
    tar.close()

# dump to file
jsonDumpFile = os.path.join(jsonDumpDir, logDir + '.json')
if tests:
   with open(jsonDumpFile, 'w') as f:
       json.dump(tests, f, indent=2)

# collect some stats
duration_ms = []
finalError_deg = []
for test in tests:
    duration_ms.append(test['timestamp'][-1] - test['timestamp'][0])
    finalError_deg.append(test['desAngle'][-1] - test['currAngle'][-1])
    print "duration {} ms, error {} deg".format(duration_ms[-1], finalError_deg[-1])

print "\nTotal tests: {}".format(len(tests))
print "Median duration {} ms, median abs error {} deg".format(np.median(duration_ms), np.median(np.abs(finalError_deg)))

i = -1
fig = plt.figure(1)
fig.suptitle("TurnInPlace test {}".format(i), fontsize=16)

plt.subplot(221)
plt.plot(tests[i]['timestamp'], tests[i]['currAngle'], label='actual')
plt.plot(tests[i]['timestamp'], tests[i]['desAngle'], label='desired')
plt.xlabel('timestamp (ms)')
plt.ylabel('angular position (deg)')
plt.legend(loc=4,prop={'size':10})
plt.grid()

plt.subplot(222)
plt.plot(tests[i]['timestamp'], tests[i]['currSpeed'], label='actual')
plt.plot(tests[i]['timestamp'], tests[i]['desSpeed'], label='desired')
plt.xlabel('timestamp (ms)')
plt.ylabel('angular velocity (deg/sec)')
plt.legend(prop={'size':10})
plt.grid()

plt.subplot(223)
plt.plot(tests[i]['timestamp'], tests[i]['ff_term'], label='ff term')
plt.plot(tests[i]['timestamp'], tests[i]['p_term'], label='p term')
plt.plot(tests[i]['timestamp'], tests[i]['i_term'], label='i term')
plt.plot(tests[i]['timestamp'], tests[i]['d_term'], label='d term')
plt.plot(tests[i]['timestamp'], tests[i]['arcVel'], label='total')
plt.xlabel('timestamp (ms)')
plt.ylabel('control effort')
plt.legend(prop={'size':10})
plt.grid()


plt.show()
