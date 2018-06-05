# Copyright (c) 2018 Anki, Inc.

'''
SDK for programming with Anki's Vector Robot.
'''

import sys
import logging

from . import messaging
from .robot import Robot, AsyncRobot

logger = logging.getLogger('victor')

if sys.version_info < (3, 5, 1):
    sys.exit('victor requires Python 3.5.1 or later')

__all__ = ['logger', 'AsyncRobot', 'Robot', 'messaging']
