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

# IP of robot
ANKI_ROBOT_HOST = os.environ.get('ANKI_ROBOT_HOST')

# Path to vicos_which.sh
VICOS_WHICH = os.path.join(SCRIPTS, 'vicos_which.sh')

# Path to vicos addr2line
ADDR2LINE = exec([VICOS_WHICH, 'addr2line'])

#
# Global cache of objfn lookup results
#
objfn_cache = {}

#
# Helper function to locate an object file in our build tree.
# Maintain a cache of results so we don't have to hit the filesystem each time.
#
def get_objfn(obj, options):
  if obj in objfn_cache:
    return objfn_cache[obj]
  objfn = os.path.join(TOPLEVEL, "_build", "vicos", options.configuration, "bin", obj)
  if os.path.exists(objfn):
    objfn_cache[obj] = objfn
    return objfn
  objfn = os.path.join(TOPLEVEL, "_build", "vicos", options.configuration, "lib", obj)
  if os.path.exists(objfn):
    objfn_cache[obj] = objfn
    return objfn
  objfn_cache[obj] = None
  return None

#
# Global cache of symfn lookup results
#
symfn_cache = {}

#
# Helper function to locate symbol file for a given object file.
# If objfn.full exists, use it, else use objfn itself.
# Maintain a cache of lookup results so we don't have to hit the filesystem every time.
#
def get_symfn(objfn):
  if objfn in symfn_cache:
    return symfn_cache[objfn]
  symfn = objfn + ".full"
  if os.path.exists(symfn):
    symfn_cache[objfn] = symfn
    return symfn
  symfn_cache[objfn] = objfn
  return objfn

def addr2line(obj, addr, options):
  # Can we find this object in our build tree?
  objfn = get_objfn(obj, options)
  if not objfn:
    return None

  # Do we have a symbol file for this object file?
  symfn = get_symfn(objfn)

  # Call addr2line to get symbol info
  cmd = [ADDR2LINE, '-sfCe', symfn, addr]
  output = exec(cmd).replace('\n', ' ')

  return output

def show_line (line, options):
  # Does this look like a backtrace?
  match = re.search('([#0-9]+)  ([0-9a-zA-Z\_\-\+\.]+) \+ (0x[0-9a-f]+)', line)
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
  if not options.skip_copy:
    if not re.search('^/', minidump):
      remote_minidump = "/data/data/com.anki.victor/cache/crashDumps/{}".format(minidump)

      # Fetch minidump from robot
      cmd = ["scp", "-p", "root@"+ANKI_ROBOT_HOST+":"+remote_minidump,"."]
      subprocess.call(cmd)

  minidump_stackwalk = os.path.join(TOPLEVEL, 'tools/crash-tools/linux/minidump_stackwalk')
  symbols = os.path.join(TOPLEVEL, '_build/vicos', options.configuration, 'symbols')
  cmd = ['noah', minidump_stackwalk, minidump, symbols]
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
  parser.add_argument('-d',
                      dest='dump_syms',
                      action='store_true',
                      help='Generate symbols'
  )
  parser.add_argument('-s',
                      dest='skip_copy',
                      action='store_true',
                      help='Skip copying dump file from robot'
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

  # Process symbols
  if options.dump_syms:
    DUMP_SYMS = os.path.join(TOPLEVEL, 'project/victor/scripts/dump-syms.sh')
    cmd = [DUMP_SYMS, options.configuration]
    subprocess.call(cmd)

  # Process each minidump
  for minidump in options.minidumps:
    show_minidump(minidump, options)

main()
