# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import logging as _logging

#: The general purpose logger logs high level information about cozmo events
logger = _logging.getLogger('cozmo.general')

#: The protocol logger logs low level messages that are sent back and forth to Cozmo
logger_protocol = _logging.getLogger('cozmo.protocol')

del _logging

from . import action
from . import anim
from . import behavior
from . import conn
from . import event
from . import exceptions
from . import lcd_face
from . import lights
from . import objects
from . import robot
from . import run
from . import util
from . import world

from .exceptions import *
from .run import *

from .version import __version__


__all__ = ['logger', 'logger_protocol'] + \
    ['action', 'anim', 'behavior', 'conn', 'event', 'exceptions', 'lcd_face'] + \
    ['lights', 'objects', 'robot', 'run', 'util', 'world'] + \
        (run.__all__ + exceptions.__all__)
