#!/usr/bin/env python2

from __future__ import print_function

import argparse
import io
import logging
import os
import struct
import sys
import textwrap

#set up default logger
Logger = logging.getLogger('fix_m4a_timestamps_in_wwise_file')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)

def get_bytes_from_file(filename):
  f = open(filename, "rb")
  msg = f.read()
  f.close()
  return msg

def write_bytes_to_file(filename, msg):
  f = open(filename, "wb")
  f.write(msg)
  f.close()

def unpack_little_endian_long(lng):
  return struct.unpack("<L", lng)[0]

def unpack_big_endian_long(lng):
  return struct.unpack(">L", lng)[0]

# M4A format data available at http://xhelmboyx.tripod.com/formats/mp4-layout.txt
# There are 3 boxes: mvhd, tkhd, and mdhd that contain created and modified dates
# They all need to be set to a constant value.  Below we set them to '__anki__'
# In the event that we encountered 64-bit versions of these fields, we set the additional
# 8 bytes to '__anki__' as well.
def process_m4a_data(msg_ba,offset,length):
  while offset < offset+length and offset < len(msg_ba):
    box_length = unpack_big_endian_long(msg_ba[offset:offset+4])
    box_type = msg_ba[offset+4:offset+8]
    if box_type == 'mvhd' or box_type == 'tkhd' or box_type == 'mdhd':
      version = msg_ba[offset+8]
      flags = msg_ba[offset+9:offset+12]
      msg_ba[offset+12:offset+20] = '__anki__'
      if version == 1:
        msg_ba[offset+20:offset+28] = '__anki__'
    elif box_type == 'moov' or box_type == 'trak' or box_type == 'mdia':
      process_m4a_data(msg_ba, offset+8, box_length-8)
    if box_length == 0:
      offset = len(msg_ba)
    else:
      offset += box_length
    # Note: we do NOT handle the special case of box_length == 1
    # We do not expect this tool to be used for files > 4.2 GB. See the above
    # referenced http://xhelmboyx.tripod.com/formats/mp4-layout.txt for more
    # details

# A WEM file appears to be layed out like a WAV file
# See http://soundfile.sapp.org/doc/WaveFormat for info
# Below we first verify that we are indeed looking at data from a WEM file.
# Secondly, if and only if we find the M4A header do we move on
def fix_m4a_timestamps_in_wem_data(msg_ba, offset, length, check_chunk_size = True):
  end_offset = offset + length
  riff = msg_ba[offset:offset+4]
  if riff != 'RIFF':
    raise ValueError("File does not start with 'RIFF'")
  offset += 4

  # Checking the chunk size of the WEM file is optional because sometimes
  # WEM data for referenced files embedded inside a BNK will have a chunk size for
  # the entire size of the file instead of just the header snippet that is included
  if check_chunk_size:
    chunk_size = unpack_little_endian_long(msg_ba[offset:offset+4])
    if chunk_size != length - 8:
      raise ValueError('chunk_size(' + str(chunk_size) + ') ' +
                       '!= file length - 8 (' + str(length - 8) + ')')
  offset += 4
  wave = msg_ba[offset:offset+4]
  if wave != 'WAVE':
    raise ValueError("File does not have 'WAVE' chunk")
  offset += 4
  fmt = msg_ba[offset:offset+4]
  if fmt != 'fmt ':
    raise ValueError("File does not have 'fmt ' sub chunk")
  offset += 4
  sub_chunk_size = unpack_little_endian_long(msg_ba[offset:offset+4])
  offset += 4
  offset += sub_chunk_size
  sub_chunk_id = msg_ba[offset:offset+4]
  offset += 4
  sub_chunk_size = unpack_little_endian_long(msg_ba[offset:offset+4])
  offset += 4

  # While normally a WAV file should just have 'fmt ' and 'data' subchunks, I found
  # one WEM file that had an additional chunk called 'akd ' that came before 'data'
  # Therefore we need to process all sub chunks until we find 'data' or quit
  while sub_chunk_id != 'data' and offset < (end_offset - sub_chunk_size - 8):
    offset += sub_chunk_size
    sub_chunk_id = msg_ba[offset:offset+4]
    offset += 4
    sub_chunk_size = unpack_little_endian_long(msg_ba[offset:offset+4])
    offset += 4

  if sub_chunk_id != 'data':
    raise ValueError("File does not have 'data' sub chunk")

  # Make sure that we are looking at M4A data.  In case we were called to process
  # a file using Vorbis (for Android) or something else.  Note, there are .wem files
  # in our iOS or Mac build in the top level directory that DO NOT have M4A content within
  # them.  We do not want to raise an error for these files if 'ftyp' is not found, so
  # instead we just return
  ftyp_offset = unpack_big_endian_long(msg_ba[offset:offset+4])
  ftyp_id = msg_ba[offset+4:offset+8]
  if ftyp_id != 'ftyp':
    return
  process_m4a_data(msg_ba, offset, sub_chunk_size)
  return

def fix_m4a_timestamps_in_wem_file(wem_file):
  Logger.info("Processing " + wem_file)
  msg = get_bytes_from_file(wem_file)
  msg_ba = bytearray(msg)
  fix_m4a_timestamps_in_wem_data(msg_ba, 0, len(msg_ba))
  msg_new = str(msg_ba)
  if msg_new != msg:
    write_bytes_to_file(wem_file, msg_new)

# Format of WWise .bnk file from http://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)
def fix_m4a_timestamps_in_bnk_data(msg_ba):
  offset = 0
  section_id = msg_ba[offset:offset+4]
  if section_id != 'BKHD':
    raise ValueError("File does NOT begin with 'BKHD'")
  offset += 4
  section_length = unpack_little_endian_long(msg_ba[offset:offset+4])
  offset += 4

  save_offset = offset
  save_section_length = section_length
  data_offset = 0
  # find DATA section
  while section_id != 'DATA' and offset < (len(msg_ba) - section_length - 8):
    offset += section_length
    section_id = msg_ba[offset:offset+4]
    offset += 4
    section_length = unpack_little_endian_long(msg_ba[offset:offset+4])
    offset += 4

  if section_id != 'DATA':
    return
  data_offset = offset

  # find DIDX section
  offset = save_offset
  section_length = save_section_length
  while section_id != 'DIDX' and offset < (len(msg_ba) - section_length - 8):
    offset += section_length
    section_id = msg_ba[offset:offset+4]
    offset += 4
    section_length = unpack_little_endian_long(msg_ba[offset:offset+4])
    offset += 4

  if section_id != 'DIDX':
    return

  didx_end = offset + section_length
  while offset < didx_end:
    wem_file_id = unpack_little_endian_long(msg_ba[offset:offset+4])
    offset += 4
    wem_file_offset = unpack_little_endian_long(msg_ba[offset:offset+4]) + data_offset
    offset += 4
    wem_file_length = unpack_little_endian_long(msg_ba[offset:offset+4])
    offset += 4
    # Check data type
    dataType = msg_ba[wem_file_offset:wem_file_offset + 4]
    if dataType == 'RIFF':
      fix_m4a_timestamps_in_wem_data(msg_ba, wem_file_offset, wem_file_length, False)

def fix_m4a_timestamps_in_bnk_file(bnk_file):
  Logger.info("Processing " + bnk_file)
  msg = get_bytes_from_file(bnk_file)
  msg_ba = bytearray(msg)
  fix_m4a_timestamps_in_bnk_data(msg_ba)
  msg_new = str(msg_ba)
  if msg_new != msg:
    write_bytes_to_file(bnk_file, msg_new)

def fix_m4a_timestamps_in_wwise_file(wwise_file):
  (filename, ext) = os.path.splitext(wwise_file)
  if ext == ".wem":
    fix_m4a_timestamps_in_wem_file(wwise_file)
  elif ext == ".bnk":
    fix_m4a_timestamps_in_bnk_file(wwise_file)

def fix_m4a_timestamps_in_wwise_files_in_dir(dir):
  for f in os.listdir(dir):
    entry = os.path.join(dir, f)
    if os.path.isfile(entry):
      fix_m4a_timestamps_in_wwise_file(entry)
    elif os.path.isdir(entry):
      fix_m4a_timestamps_in_wwise_files_in_dir(entry)

def parse_args(argv=[], print_usage=False):

  version = '1.0'
  parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description='fix M4A timestamps inside a WWise file',
    epilog=textwrap.dedent('''
      options description:
        [wwise file]
  '''),
    version=version
    )

  parser.add_argument('--debug', '-d', '--verbose', dest='debug', action='store_true',
                      help='prints debug output')

  parser.add_argument('wwise_file', action='store',
                      help='The wwise file or a directory of wwise files.')

  if print_usage:
    parser.print_help()
    sys.exit(2)

  args = parser.parse_args(argv)

  if (args.debug):
    print(args)
    Logger.setLevel(logging.DEBUG)
  else:
    Logger.setLevel(logging.INFO)

  return args

def run(args):
  if os.path.isfile(args.wwise_file):
    return fix_m4a_timestamps_in_wwise_file(args.wwise_file)
  elif os.path.isdir(args.wwise_file):
    return fix_m4a_timestamps_in_wwise_files_in_dir(args.wwise_file)

if __name__ == '__main__':
  args = parse_args(sys.argv[1:])
  sys.exit(run(args))


