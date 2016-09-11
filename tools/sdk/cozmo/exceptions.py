__all__ = ['CozmoSDKException', 'AnimationsNotLoaded', 'StopPropogation',
        'AnimationsNotLoaded', 'ActionError', 'NotPickupable', 'RobotBusy']


class CozmoSDKException(Exception):
    '''Base class of all Cozmo SDK exceptions.'''

class ConnectionAborted(CozmoSDKException):
    '''Raised if the connection to the device is unexpectedly lost.'''

class SDKVersionMismatch(CozmoSDKException):
    '''Raised if the Cozmo SDK version is not compatible with the software running on the device.'''

class StopPropogation(CozmoSDKException):
    '''Raised by event handlers to prevent further handlers from being triggered.'''

class AnimationsNotLoaded(CozmoSDKException):
    '''Raised if an attempt is made to play a named animation before animations have been received.'''

class ActionError(CozmoSDKException):
    '''Base class for errors that occur with robot actions.'''

class NotPickupable(ActionError):
    '''Raised if an attempt is made to pickup or place an object that can't be picked up by Cozmo'''

class CannotPlaceObjectsOnThis(ActionError):
    '''Raised if an attempt is made to place an object on top of an invalid object'''

class RobotBusy(ActionError):
    '''Raised if an attempt is made to perform an action while another action is still running.'''

