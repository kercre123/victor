import os
import sys
import subprocess
import glob
import shutil

"""
A python wrapper for the the wavtool
"""
def run_wav(sin, refin=None, sout="sout.wav", rout="rout.wav", json_config=None, refin_delay_sec=0, exe_file="build/wav"):
    args=[exe_file]

    arg_sin="--sin=%s" % sin
    args.append(arg_sin)

    if refin != None:
        arg_refin="--refin=%s" % refin
        args.append(arg_refin)

    arg_sout = "--sout=%s" % sout
    args.append(arg_sout)

    arg_rout = "--rout=%s" % rout
    args.append(arg_rout)

    if json_config != None:
        arg_json = "--diag-settings=%s" % json_config
        args.append(arg_json)

    args.append("--refin-delay=%d" % refin_delay_sec)      

    print(args)
    subprocess.call(args)

