# Copyright (c) 2018 Anki, Inc.

'''
SDK for programming with Anki's Vector Robot.
'''

import sys
import logging

from . import messaging
from .robot import Robot, AsyncRobot
from . import oled_face

logger = logging.getLogger('victor')  # pylint: disable=invalid-name

if sys.version_info < (3, 5, 1):
    sys.exit('victor requires Python 3.5.1 or later')

__all__ = ['logger', 'AsyncRobot', 'Robot', 'messaging']
