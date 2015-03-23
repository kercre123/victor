import sys, time

open('ov2686_code_timestamp.h', 'w').write("""
#define CODE_TIMESTAMP "%s"
""" % time.ctime())
