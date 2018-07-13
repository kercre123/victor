'''
SDK-specific exception classes for Vector

Copyright (c) 2018 Anki, Inc.
'''

from grpc import RpcError, StatusCode

__all__ = ["VectorException", "VectorConnectionException"]


class VectorException(Exception):
    '''Base class of all Vector SDK exceptions.'''


class VectorConnectionException(VectorException):
    def __init__(self, cause):
        self._status = cause.code()
        self._details = cause.details()
        doc_str = self.__class__.__doc__
        msg = (f"{self._status}: {self._details}"
               f"\n\n{doc_str if doc_str else 'Unknown error'}")
        super().__init__(msg)

    @property
    def status(self):
        return self._status

    @property
    def details(self):
        return self._details


class VectorUnauthenticatedException(VectorConnectionException):
    '''Failed to authenticate request'''


class VectorUnavailableException(VectorConnectionException):
    '''Unable to reach Vector'''


class VectorUnimplementedException(VectorConnectionException):
    '''Vector does not handle this message'''


class VectorTimeoutException(VectorConnectionException):
    '''Message took too long to complete'''


def connection_error(rpc_error: RpcError) -> VectorConnectionException:
    code = rpc_error.code()
    if code is StatusCode.UNAUTHENTICATED:
        return VectorUnauthenticatedException(rpc_error)
    if code is StatusCode.UNAVAILABLE:
        return VectorUnavailableException(rpc_error)
    if code is StatusCode.UNIMPLEMENTED:
        return VectorUnimplementedException(rpc_error)
    if code is StatusCode.DEADLINE_EXCEEDED:
        return VectorTimeoutException(rpc_error)
    return VectorConnectionException(rpc_error)


class VectorNotReadyException(VectorException):
    '''Vector tried to do something before it was ready'''

    def __init__(self, cause):
        msg = (f"{self.__class__.__doc__}: {cause if cause else 'Unknown error'}")
        super().__init__(msg)
