'''
Shared utilities for running tests.
'''

import asyncio
import argparse
import os
from pathlib import Path


def parse_args(parser=None):
    '''
    Provides the command line interface for all the tests

    Args:
    parser (argparse.ArgumentParser): To add new arguments,
         pass an argparse parser with the new options
         already defined. Leave empty to use the defaults.
    '''
    if parser is None:
        parser = argparse.ArgumentParser()
    parser.add_argument("-n", "--name", nargs='?', default=os.environ.get('VECTOR_ROBOT_NAME', None))
    parser.add_argument("-i", "--ip", nargs='?', default=os.environ.get('VECTOR_ROBOT_IP', None))
    parser.add_argument("-c", "--cert_file", nargs='?', default=os.environ.get('VECTOR_ROBOT_CERT', None))
    parser.add_argument("--port", nargs='?', default="443")
    args = parser.parse_args()
    if args.port == "8443":
        args.name = os.environ.get('VECTOR_ROBOT_NAME_MAC', args.name)
        args.ip = os.environ.get('VECTOR_ROBOT_IP_MAC', args.ip)
        args.cert_file = os.environ.get('VECTOR_ROBOT_CERT_MAC', args.cert_file)

    if args.name is None or args.ip is None or args.cert_file is None:
        parser.error('the following arguments are required: name, ip, cert_file '
                     'or they may be set with the environment variables: '
                     'VECTOR_ROBOT_NAME, VECTOR_ROBOT_IP, VECTOR_ROBOT_CERT '
                     'respectively')

    cert = Path(args.cert_file)
    args.cert = cert.resolve()
    return args


async def delay_close(time=30, log_fn=print):
    if time <= 0:
        raise ValueError("time must be > 0")
    try:
        for countdown in range(time, 0, -1):
            await asyncio.sleep(1)
            log_fn(f"{countdown:2} second{'s' if countdown > 1 else ''} remaining...")
    except Exception as e:
        raise e
