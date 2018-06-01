# Copyright (c) 2018 Anki, Inc.

'''
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['setup_basic_logging', 'get_class_logger']

import logging
import os
import sys

MODULE_LOGGER = logging.getLogger(__name__)

def setup_basic_logging(custom_handler=None, general_log_level=None, target=sys.stderr):
    '''Helper to perform basic setup of the Python logging machinery.
    Args:
        general_log_level (str): 'DEBUG', 'INFO', 'WARN', 'ERROR' or an equivalent
            constant from the :mod:`logging` module.  If None then a
            value will be read from the COZMO_LOG_LEVEL environment variable.
        target (object): The stream to send the log data to; defaults to stderr
    '''
    if general_log_level is None:
        general_log_level = os.environ.get('VICTOR_LOG_LEVEL', logging.DEBUG)

    handler = custom_handler
    if handler is None:
        handler = logging.StreamHandler(stream=target)
        formatter = logging.Formatter('%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
        handler.setFormatter(formatter)

    vector_logger = logging.getLogger('vector')
    if not vector_logger.handlers:
        vector_logger.addHandler(handler)
        vector_logger.setLevel(general_log_level)

def get_class_logger(module, obj):
    '''Helper to create logger for a given class (and module)'''
    return logging.getLogger(".".join([module, type(obj).__name__]))
