#!/usr/bin/python

"""argparse_help.py

Helper functionality for argparse

Author: Lee Crippen
10-14-2015
"""

import os, argparse
    

""" Simple check on the directory to make sure it's valid """
def is_directory_writable(value):
    if value == None:
        return value

    if not os.path.isdir(value):
        raise argparse.ArgumentTypeError("{0} is not a valid directory!".format(value))
    elif not os.access(value, os.W_OK):
        raise argparse.ArgumentTypeError("{0} is not a writable directory!".format(value))
    return value
