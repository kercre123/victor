# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

__all__ = []

from . import event

from ._internal.clad.externalInterface import messageEngineToGame as messageEngineToGame
from ._internal.clad.externalInterface import messageGameToEngine as messageGameToEngine

# Shortcut access to CLAD classes
_clad_to_engine_anki = messageGameToEngine.Anki
_clad_to_engine_cozmo = messageGameToEngine.Anki.Cozmo
_clad_to_engine_iface = messageGameToEngine.Anki.Cozmo.ExternalInterface
_clad_to_game_anki = messageEngineToGame.Anki
_clad_to_game_cozmo = messageEngineToGame.Anki.Cozmo
_clad_to_game_iface = messageEngineToGame.Anki.Cozmo.ExternalInterface

# Register event types for engine to game messages
# (e.g. _MsgObjectMoved)
for _name in vars(_clad_to_game_iface.MessageEngineToGame.Tag):
    attrs = {
        '__doc__': 'Internal protocol message',
        'msg': 'Message data'
    }
    _name = '_Msg' + _name
    cls = event._register_dynamic_event_type(_name, attrs)
    globals()[_name] = cls
