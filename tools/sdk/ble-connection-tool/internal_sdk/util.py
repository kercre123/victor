'''
Utility functions for internal SDK
'''
import logging
import re

from victorclad.externalInterface import messageExternalComms
ext_comms = messageExternalComms.Anki.Victor.ExternalComms

try:
    from termcolor import colored
except ImportError:
    def colored(msg, color):
        return msg

logger = logging.getLogger('internal_sdk.util')

def to_hex(b):
    return "".join("{:02x}".format(c) for c in bytes(b, 'utf-8'))


def from_hex(line):
    result = [line[i:i+2] for i in range(0, len(line), 2)]
    try:
        b = bytes([int(x.decode('ascii'), 16) for x in result])
        return b.decode('ascii')
    except AttributeError:
        return bytes([int(x, 16) for x in result]).decode('ascii')


def tagAndPackMessage(msg, version):
    if getattr(ext_comms, "RtsConnection", None): # TEMP: until factory-0.9 gets merged
        msg = ext_comms.RtsConnection(**{msg.__class__.__name__: msg})
    elif version == 1:
        msg = ext_comms.RtsConnection_1(**{msg.__class__.__name__: msg})
    elif version == 2:
        msg = ext_comms.RtsConnection_2(**{msg.__class__.__name__: msg})
        msg = ext_comms.RtsConnection(**{msg.__class__.__name__: msg})
    else:
        raise NotImplementedError(f"Version: {version} not yet implemented")
    msg = ext_comms.ExternalComms(**{msg.__class__.__name__: msg})
    logger.debug(f"{colored('Packing', 'cyan')}: {msg}")
    return msg.pack()


def log_byte_message(description, b, log=print):
    log(f"{colored(description, 'cyan')}: {formatted_bytes(b)}")


def formatted_bytes(b):
    if b is not None:
        return " ".join("{:02x}".format(c) for c in b)
    return None


if getattr(ext_comms, "RtsConnection", None): # TEMP: until factory-0.9 gets merged
    def is_rts_of(obj, t, parent=ext_comms.RtsConnection):
        if not isinstance(obj, parent):
            return False
        rts_type = parent.typeByTag(obj.tag)
        if rts_type == t:
            return True
        return False
else:
    def is_rts_of(obj, t, parent=ext_comms.RtsConnection_1):
        if not isinstance(obj, parent):
            if parent == ext_comms.RtsConnection_1:
                return is_rts_of(obj, t, ext_comms.RtsConnection_2)
            return False
        rts_type = parent.typeByTag(obj.tag)
        if rts_type == t:
            return True
        return False


class ColorlessFormatter(logging.Formatter):
    """
    This is a filter which strips color codes from the logs.
    """
    def format(self, record):
        s = super(ColorlessFormatter, self).format(record)
        ansi_escape = re.compile(r'\x1B\[[0-?]*[ -/]*[@-~]')
        s = ansi_escape.sub('', s)
        return s
