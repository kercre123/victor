# Copyright (c) 2018 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Synchronizer related classes and functions.

A synchronizer is used to make functions waitable outside
of the scope of the event loop. This allows for more
advanced use cases where multiple commands may be sent in
parallel (see :class:`AsyncRobot` in robot.py), but also
simpler use cases where everything executes synchronously.
"""

__all__ = ['Synchronizer']

import asyncio
import functools

import grpc

from . import exceptions


class Synchronizer:
    @staticmethod
    def _wait(task, loop):
        if loop.is_running():
            for result in asyncio.as_completed([task], loop=loop):
                return result
        return loop.run_until_complete(task)

    @staticmethod
    def wrap(log_messaging=True):
        def synchronous_decorator(func):
            if not asyncio.iscoroutinefunction(func):
                raise Exception("how would I sync that which is not async?")  # TODO: better error

            @functools.wraps(func)
            async def log_handler(func, logger, *args, **kwargs):
                """Log the input and output of messages (and wrap the rpc_errors)"""  # TODO: improve docs
                result = None
                if log_messaging:
                    logger.debug(f'Outgoing {func.__name__}{args[1:]}')
                try:
                    result = await func(*args, **kwargs)
                except grpc.RpcError as rpc_error:
                    raise exceptions.connection_error(rpc_error) from rpc_error
                if log_messaging:
                    logger.debug(f'Incoming {type(result).__name__}: {str(result).strip()}')
                return result

            @functools.wraps(func)
            def result(*args, **kwargs):
                """Handler that determines sync vs async execution"""  # TODO: improve docs
                self = args[0]  # Get the self reference from the function call
                wrapped_func = log_handler(func, self.logger, *args, **kwargs)
                task = self.loop.create_task(wrapped_func)
                if self.force_async:
                    return task
                return Synchronizer._wait(task, self.loop)
            return result
        return synchronous_decorator
