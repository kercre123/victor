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
#   uint32_t reserved[43];
#   uint32_t playpenTouchSensorMinValid;
#   uint32_t playpenTouchSensorMaxValid;
#   float    playpenTouchSensorRangeThresh;
#   float    playpenTouchSensorStdDevThresh;
#   uint32_t playpenTestDisableMask;
#   uint32_t playpen[8];
#   uint32_t fixture[192];
# } EMR;

emr_format="<LLLLLLLL43LLLffL8L192L"

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
parser.add_argument('--playpen-touch-min-valid', action='store', type=auto_int, default=0, help="min valid touch sensor value")                    
parser.add_argument('--playpen-touch-max-valid', action='store', type=auto_int, default=0, help="max valid touch sensor value")                    
parser.add_argument('--playpen-touch-range-thresh', action='store', type=float, default=0, help="playpen touch sensor max-min threshold")
parser.add_argument('--playpen-touch-stddev-thresh', action='store', type=float, default=0, help="playpen touch sensor stdDev threshold")
parser.add_argument('--playpen-test-disable-mask', action='store', type=auto_int, default=0, help="playpen test disable mask")
parser.add_argument('--out', action='store', type=argparse.FileType('wb'), default="./birthcertificate",
                    help="file to store birthcertificate in")
args = parser.parse_args()

reserved_zeros = ([0] * 43)
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
                  args.playpen_touch_min_valid,
                  args.playpen_touch_max_valid,
                  args.playpen_touch_range_thresh,
                  args.playpen_touch_stddev_thresh,
                  args.playpen_test_disable_mask,
                  *playpen_zeros,
                  *fixture_zeros);

args.out.write(str)
args.out.close()



