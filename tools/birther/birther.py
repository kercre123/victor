#!/usr/bin/env python3

import argparse
import struct
import time

# Below is the C structure for the birth certificate file found on
# the Victor robot.
#
# EMR structure definition taken from <victor>/robot/include/anki/cozmo/shared/factory/emr.h
#
# typedef struct {
#   uint32_t ESN;
#   uint32_t HW_VER;
#   uint32_t MODEL;
#   uint32_t LOT_CODE;
#   uint32_t PLAYPEN_READY_FLAG; //fixture approves all previous testing. OK to run playpen
#   uint32_t PLAYPEN_PASSED_FLAG;
#   uint32_t PACKED_OUT_FLAG;
#   uint32_t PACKED_OUT_DATE; //Unix time?
#   uint32_t reserved[47];
#   uint32_t playpenTestDisableMask;
#   uint32_t playpen[8];
#   uint32_t fixture[192];
# } EMR;

emr_format="<LLLLLLLL47LL8L192L"

def auto_int(x):
  return int(x, 0)

parser = argparse.ArgumentParser(description='Create a fake birth certificate for Victor.',
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('--esn', action='store', type=auto_int, default=0, help="serial number")
parser.add_argument('--hw-ver', action='store', type=auto_int, default=0, help="hardware version")
parser.add_argument('--model', action='store', type=auto_int, default=0, help="model number")
parser.add_argument('--lot-code', action='store', type=auto_int, default=0, help="lot code")
parser.add_argument('--playpen-ready-flag', action='store', type=auto_int, default=0, help="ready flag")
parser.add_argument('--playpen-passed-flag', action='store', type=auto_int, default=0, help="passed flag")
parser.add_argument('--packed-out-flag', action='store', type=auto_int, default=1, help="packed out flag")
parser.add_argument('--packed-out-date', action='store', type=auto_int, default=int(time.time()),
                    help="packed out date (seconds since unix epoch)")
parser.add_argument('--playpen-test-disable-mask', action='store', type=auto_int, default=0, help="playpen test disable mask")
parser.add_argument('--out', action='store', type=argparse.FileType('wb'), default="./birthcertificate",
                    help="file to store birthcertificate in")
args = parser.parse_args()

reserved_zeros = ([0] * 47)
playpen_zeros  = ([0] * 8)
fixture_zeros  = ([0] * 192)

str = struct.pack(emr_format,
                  args.esn,
                  args.hw_ver,
                  args.model,
                  args.lot_code,
                  args.playpen_ready_flag,
                  args.playpen_passed_flag,
                  args.packed_out_flag,
                  args.packed_out_date,
                  *reserved_zeros,
                  args.playpen_test_disable_mask,
                  *playpen_zeros,
                  *fixture_zeros);

args.out.write(str)
args.out.close()



