'''
Shared utilities for running tests.
'''

import asyncio
import argparse
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
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    parser.add_argument("--port", default="443")
    args = parser.parse_args()

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
