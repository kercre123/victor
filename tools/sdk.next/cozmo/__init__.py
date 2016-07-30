import logging

#: The general purpose logger logs high level information about cozmo events
logger = logging.getLogger('cozmo.general')

#: The protocol logger logs low level messages that are sent back and forth to Cozmo
logger_protocol = logging.getLogger('cozmo.protocol')

from .run import *
from .exceptions import *


__all__ = ['logger', 'logger_protocol'] + \
        (run.__all__ + exceptions.__all__)
