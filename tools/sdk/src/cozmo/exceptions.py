# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

__all__ = ['CozmoSDKException', 'SDKShutdown', 'StopPropogation',
        'AnimationsNotLoaded', 'ActionError', 'ConnectionError',
        'ConnectionAborted', 'ConnectionCheckFailed', 'NoDevicesFound',
        'SDKVersionMismatch', 'NotPickupable', 'CannotPlaceObjectsOnThis',
        'RobotBusy']


class CozmoSDKException(Exception):
    '''Base class of all Cozmo SDK exceptions.'''

class SDKShutdown(CozmoSDKException):
    '''Raised when the SDK is being shut down'''

class StopPropogation(CozmoSDKException):
    '''Raised by event handlers to prevent further handlers from being triggered.'''

class AnimationsNotLoaded(CozmoSDKException):
    '''Raised if an attempt is made to play a named animation before animations have been received.'''

class ActionError(CozmoSDKException):
    '''Base class for errors that occur with robot actions.'''

class ConnectionError(CozmoSDKException):
    '''Base class for errors regarding connection to the device.'''

class ConnectionAborted(ConnectionError):
    '''Raised if the connection to the device is unexpectedly lost.'''

class ConnectionCheckFailed(ConnectionError):
    '''Raised if the connection check has failed.'''

class NoDevicesFound(ConnectionError):
    '''Raised if no devices connected running Cozmo in SDK mode'''

class SDKVersionMismatch(ConnectionError):
    '''Raised if the Cozmo SDK version is not compatible with the software running on the device.'''

class NotPickupable(ActionError):
    '''Raised if an attempt is made to pick up or place an object that can't be picked up by Cozmo'''

class CannotPlaceObjectsOnThis(ActionError):
    '''Raised if an attempt is made to place an object on top of an invalid object'''

class RobotBusy(ActionError):
    '''Raised if an attempt is made to perform an action while another action is still running.'''
