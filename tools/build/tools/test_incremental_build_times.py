#!/usr/bin/env python3

import sys
import operator
import os
import subprocess
import time

def get_header_classes(path):
    '''
    Return a list of all header files within the codebase
    '''
    
    headers = []
    
    for root, dirs, files in os.walk(path):
        # skip "helper" folders which may contain things that are named like behaviors
        # e.g. BehaviorScoringWrapper (which is a wrapper for behavior scores, not a
        # behavior itself)
        if "helpers" in dirs:
            dirs.remove("helpers")
            
        for filename in files:
            if os.path.splitext(filename)[1] in ['.h', '.hpp']:
                full_path = os.path.join(root, filename)
                headers.append( full_path )    
    return headers

BUILD_INVOCATION = "./project/victor/build-victor.sh -p mac"
OUTPUT_PATH = "incrementalOutput.txt"

if __name__ == "__main__":
	# Clear out the build directory and do a clean build to start
	subprocess.call("rm -rf _build", shell=True)
	cleanStartTime = time.time()
	subprocess.call(BUILD_INVOCATION, shell=True)
	print("Clean build took " + str(time.time() - cleanStartTime) + " seconds")

	# Gather all header files in engine
	allHeaders = get_header_classes("engine")
	print("There are " + str(len(allHeaders)) + " headers within engine")

	# Alter each header slightly and log rebuild time
	rebuildTimeMap = []
	for header in allHeaders:
		print("Touching " + header + " and then rebuilding")
		startTime = time.time()
		with open(header, "a") as appendToHeader:
			appendToHeader.write(" ")
		subprocess.call(BUILD_INVOCATION, shell=True)
		print("Build for " + header + " finished after " + str(time.time() - startTime) + " seconds")
		rebuildTimeMap.append((header, time.time() - startTime))

		# Sort by rebuild time and write to file every time
		sortedRebuildMap = sorted(rebuildTimeMap, key=operator.itemgetter(1), reverse=True)

		with open(OUTPUT_PATH, "w") as outputResults:
			for header, buildTime in sortedRebuildMap:
				outputResults.write("Rebuilding after touching " + header + " took " + str(buildTime) + " seconds\n")



