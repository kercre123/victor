#!/usr/bin/env python3
#
# project/victor/scripts/victor-show-tombstone.py
#
import argparse
import os
import re
import subprocess

def exec(cmd):
  output = subprocess.check_output(cmd)
  output = output.decode().strip()
  return output

# Root folder path of repo
TOPLEVEL = exec(['git', 'rev-parse', '--show-toplevel'])

# Path to helper scripts
SCRIPTS = os.path.join(TOPLEVEL, 'project', 'victor', 'scripts')

# Path to robot_sh.sh
ROBOT_SH = os.path.join(SCRIPTS, 'robot_sh.sh')

# Path to vicos_which.sh
VICOS_WHICH = os.path.join(SCRIPTS, 'vicos_which.sh')

# Path to vicos addr2line
ADDR2LINE = exec([VICOS_WHICH, 'addr2line'])

def addr2line(obj, addr, options):
  # Does this look an anki object file?
  match = re.search('/anki/(\S+)', obj)
  if not match:
    return None

  # Construct path to object file
  objfn = os.path.join(TOPLEVEL, "_build", "vicos", options.configuration, match.group(1))

  # Call addr2line to get symbol info from object file
  cmd = [ADDR2LINE, '-sfCe', objfn, addr]
  output = exec(cmd).replace('\n', ' ')

  return output

def show_line (line, options):
  # Does this look like a backtrace?
  match = re.search('([#0-9]+) pc ([0-9a-f]+)  (\S+)', line)
  if not match:
    print(line)
    return
  if match.lastindex < 3:
    print(line)
    return

  # Convert obj+addr to symbol
  idx = match.group(1)
  addr = match.group(2)
  obj = match.group(3)
  symbol = addr2line(obj, addr, options)

  if not symbol:
    print(line)
    return

  print("    {} pc {} {} ({})".format(idx, addr, obj, symbol))

def show_tombstone(tombstone, options):
  # Normalize tombstone path
  if re.search('^\d+', tombstone):
    tombstone = "tombstone_{}".format(tombstone)
  if re.search('^tombstone', tombstone):
    tombstone = "/data/tombstones/{}".format(tombstone)
  # Fetch tombstone from robot
  cmd = [ROBOT_SH, "cat", tombstone]
  output = exec(cmd)
  output = output.split('\r\n')
  # Process tombstone
  for line in output:
    show_line(line, options)

def main():
  # Parse options
  parser = argparse.ArgumentParser(description='Show victor tombstone')
  parser.add_argument('-c',
                      dest='configuration',
                      action='store',
                      choices=['Debug', 'Release'],
                      help='Configuration name (%choices)',
                      required=True
  )
  parser.add_argument(dest='tombstones',
                      action='append',
                      help='Tombstone name...'
  )

  # Validate options
  options = parser.parse_args()
  if not options.configuration:
    raise "You must specify -c configuration"
  if not options.tombstones:
    raise "You must specify tombstone name(s)"

  # Process each tombstone
  for tombstone in options.tombstones:
    show_tombstone(tombstone, options)

main()
