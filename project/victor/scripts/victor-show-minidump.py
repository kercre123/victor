#!/usr/bin/env python3
#
# project/victor/scripts/victor-show-minidump.py
#
# Fetch & display a minidump with debug symbols
# Requires /anki/bin/minidump_stackwalk on robot
# Requires addr2line and symbol files on build host
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

def objfn(obj, options):
  fn = os.path.join(TOPLEVEL, "_build", "vicos", options.configuration, "bin", obj)
  if os.path.exists(fn):
    return fn
  fn = os.path.join(TOPLEVEL, "_build", "vicos", options.configuration, "lib", obj)
  if os.path.exists(fn):
    return fn
  return None

def addr2line(obj, addr, options):
  # Can we find this object in our build tree?
  fn = objfn(obj, options)
  if not fn:
    return None

  # Call addr2line to get symbol info from object file
  cmd = [ADDR2LINE, '-sfCe', fn, addr]
  output = exec(cmd).replace('\n', ' ')

  return output

def show_line (line, options):
  # Does this look like a backtrace?
  match = re.search('([#0-9]+)  ([0-9a-zA-Z\-\+\.]+) \+ (0x[0-9a-f]+)', line)
  if not match:
    print(line)
    return
  if match.lastindex < 3:
    print(line)
    return

  idx = match.group(1)
  obj = match.group(2)
  addr = match.group(3)

  # Convert obj+addr to symbol
  symbol = addr2line(obj, addr, options)

  if not symbol:
    print(line)
    return

  print("{} ({})".format(line, symbol))

def show_minidump(minidump, options):
  # Normalize minidump path
  if not re.search('^/', minidump):
    minidump = "/data/data/com.anki.victor/cache/crashDumps/{}".format(minidump)
  # Fetch minidump from robot
  cmd = [ROBOT_SH, "/anki/bin/minidump_stackwalk", minidump]
  output = exec(cmd)
  output = output.split('\n')
  # Process minidump
  for line in output:
    show_line(line, options)

def main():
  # Parse options
  parser = argparse.ArgumentParser(description='Show victor minidump')
  parser.add_argument('-c',
                      dest='configuration',
                      action='store',
                      choices=['Debug', 'Release'],
                      help='Configuration name (%choices)',
                      required=True
  )
  parser.add_argument(dest='minidumps',
                      action='append',
                      help='Minidump name...'
  )

  # Validate options
  options = parser.parse_args()
  if not options.configuration:
    raise "You must specify -c configuration"
  if not options.minidumps:
    raise "You must specify minidump name(s)"

  # Process each minidump
  for minidump in options.minidumps:
    show_minidump(minidump, options)

main()
