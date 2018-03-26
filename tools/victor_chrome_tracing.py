#!/usr/bin/env python2

# python2 tools/victor_chrome_tracing.py --debug --outfile test.json

import argparse
import json
import os
import requests
import sys

def load_truncated_json(file):
    # by definition files are truncated, the data is valid up to a certain point, then the
    # app exits/is stopped/crashes.

    with open (file, "r") as f:
      string = f.read()

    try:
        j = json.loads(string)
        return j
    except ValueError:
        pass

    # lines are atomic, array end is missing
    fixed_string = string[0:-2] + "]}"
    try:
        j = json.loads(fixed_string)
        return j
    except ValueError:
        pass

    # value truncated
    fixed_string = string + '\"}]}'
    try:
        j = json.loads(fixed_string)
        return j
    except ValueError:
        pass

    # key truncated
    fixed_string = string + '\":\"\"}]}'
    try:
        j = json.loads(fixed_string)
        return j
    except ValueError:
        pass

    # missing key
    fixed_string = string + '"missing_key":"missing_value"}]}'
    try:
        j = json.loads(fixed_string)
        return j
    except ValueError:
        pass

    # missing array end
    fixed_string = string + ']}'
    try:
        j = json.loads(fixed_string)
        return j
    except ValueError:
        pass

    return None

def sync_time(events):
    # rebase time relative to elapsed time and application time

    if "ts_epoch" in events and "app_epoch" in events:
        ts_epoch = events["ts_epoch"]
        app_epoch = events["app_epoch"]

        for event in events["traceEvents"]:
            ts = event["ts"]
            ts -= ts_epoch
            ts += app_epoch
            event["ts"] = ts

def main():

  parser = argparse.ArgumentParser(description='viewer for anki cpu tracing profiles')

  parser.add_argument('--release', '-r',
                      action='store_true',
                      help="Use tracing files from webots release build")

  parser.add_argument('--debug', '-d',
                      action='store_true',
                      help="Use tracing files from webots debug build")

  parser.add_argument('--outfile',
                      action='store',
                      type=argparse.FileType('w'),
                      required=True,
                      help='Pull tracing files from robot')


  options = parser.parse_args()

  if options.release:
    json_files = ["./_build/mac/Release/playbackLogs/webotsCtrlAnim/vic-anim-tracing.json",
                  "./_build/mac/Release/playbackLogs/webotsCtrlGameEngine2/vic-engine-tracing.json"]
  elif options.debug:
    json_files = ["./_build/mac/Debug/playbackLogs/webotsCtrlAnim/vic-anim-tracing.json",
                  "./_build/mac/Debug/playbackLogs/webotsCtrlGameEngine2/vic-engine-tracing.json"]
  else:
    json_files = []
    for file in ["vic-engine-tracing.json", "vic-anim-tracing.json"]:
      os.system("adb pull /data/data/com.anki.victor/cache/"+file+" /tmp/"+file)
      json_files.append("/tmp/"+file)

  if len(json_files) > 0:
    # load the file file and rebase the time stamps

    first = load_truncated_json(json_files[0])
    if first == None:
      print "Unable to load file, input is truncated in some unhandled way."
      return -1

    if "displayTimeUnit" not in first:
      # not a file we generated, ignore
      return 0

    sync_time(first)

    trace_events = first["traceEvents"]

  if len(json_files) > 1:
    # load subsequent files, rebasing time stamps and appending to events

    for i in range(1, len(json_files)):
      file = json_files[i]
      next = load_truncated_json(file)
      if next == None:
        print "Unable to load file, input is truncated in some unhandled way."
        continue

      if "displayTimeUnit" not in next:
        # not a file we generated, ignore
        continue

      sync_time(next)

      trace_events += next["traceEvents"]

  # sort events

  def getKey(customobj):
    return customobj["ts"]

  trace_events = sorted(trace_events, key=getKey)

  # write merged events

  result = {}
  result["displayTimeUnit"] = first["displayTimeUnit"]
  result["traceEvents"] = trace_events

  options.outfile.write(json.dumps(result, indent=4))
  options.outfile.close()

if __name__ == "__main__":
  main()
