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

"""Support for Vector's audio.

Vector has multiple built-in microphones which can detect sound and the direction
it originated from.  The robot then processing this data into a single-channel
audio feed available to the SDK.

The :class:`AudioComponent` class defined in this module is made available as
:attr:`anki_vector.robot.Robot.audio` and can be used to enable/disable audio
sending and process audio data being sent by the robot.
"""

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['AudioComponent']

import asyncio
from concurrent.futures import CancelledError
import sys

from . import util
from .messaging import protocol

try:
    import numpy as np
except ImportError as exc:
    sys.exit("Cannot import numpy: Do `pip3 install numpy` to install")


class AudioComponent(util.Component):
    """Represents Vector's audio feed.

    The AudioComponent object receives audio data from Vector's microphones.

    The :class:`anki_vector.robot.Robot` or :class:`anki_vector.robot.AsyncRobot` instance owns this audio component.

    .. code-block:: python

        from scipy.io import wavfile
        with anki_vector.Robot("my_robot_serial_number", enable_audio_feed=True) as robot:
            robot.loop.run_until_complete(utilities.delay_close(5))
            wavfile.write("outputfile.wav", anki_vector.protocol.PROCESSED_SAMPLE_RATE, robot.audio.raw_audio_waveform_history)

    :param robot: A reference to the owner Robot object.
    """

    def __init__(self, robot):
        super().__init__(robot)

        # @TODO: Add a variable tracking the current audio send mode once that is implemented
        self._raw_audio_waveform_history = np.empty(0, dtype=np.int16)
        self._latest_sample_id = None

        self._audio_feed_task: asyncio.Task = None

    # @TODO: Implement audio history as ringbuffer, caching only recent audio
    # @TODO: Add function to return a recent chunk of the audio history in a standardized python audio package format
    # @TODO: Add hook for feeding audio stream into external voice processing library

    @property
    def raw_audio_waveform_history(self) -> np.array:
        """:class:`np.array`: The most recent processed image received from the robot.

        Audio history as signed-16-bit-int waveform data

        The sample rate from the microphones is 15625, which the robot processes at 16khz.  As a result the pitch is
        altered by 2.4%.  The last 16000 values of the array represent the last second of audio amplitude.

        .. code-block:: python

            from scipy.io import wavfile
            with anki_vector.Robot("my_robot_serial_number", enable_audio_feed=True) as robot:
                robot.loop.run_until_complete(utilities.delay_close(5))
                wavfile.write("outputfile.wav", anki_vector.protocol.PROCESSED_SAMPLE_RATE, robot.audio.raw_audio_waveform_history)

        :getter: Returns the numpy array representing the audio history
        """

        return self._raw_audio_waveform_history

    @property
    def latest_sample_id(self) -> int:
        """The most recent recieved audio sample from the robot.

        :getter: Returns the id for the latest audio sample
        """
        return self._latest_sample_id

    def init_audio_feed(self) -> None:
        """Begin audio feed task"""
        if not self._audio_feed_task or self._audio_feed_task.done():
            self._audio_feed_task = self.robot.loop.create_task(self._request_and_handle_audio())

    # @TODO: Add a way to query the current audio send mode, and a way to register as a listener for it changing.

    def close_audio_feed(self) -> None:
        """Cancel audio feed task"""
        if self._audio_feed_task:
            self._audio_feed_task.cancel()
            self.robot.loop.run_until_complete(self._audio_feed_task)

    def _process_audio(self, msg: protocol.AudioFeedResponse) -> None:
        """Processes raw data from the robot into a more more useful image structure."""
        audio_data = msg.data

        size = protocol.SAMPLE_COUNTS_PER_MESSAGE

        # Constuct numpy array out of source data
        array = np.frombuffer(audio_data.signal_power, dtype=np.int16, count=size, offset=0)

        # Append to audio history
        self._raw_audio_waveform_history = np.append(self._raw_audio_waveform_history, array)

        self._latest_sample_id = len(self._raw_audio_waveform_history) - 1

    async def _request_and_handle_audio(self) -> None:
        """Queries and listens for audio feed events from the robot.
        Recieved events are parsed by a helper function."""
        try:
            req = protocol.AudioFeedRequest()
            async for evt in self.grpc_interface.AudioFeed(req):
                # If the audio feed is disabled after stream is setup, exit the stream
                # (the audio feed on the robot is disabled internally on stream exit)
                if not self.robot.enable_audio_feed:
                    self.logger.warning('Audio feed has been disabled. Enable the feed to start/continue receiving audio feed data')
                    return
                self._process_audio(evt)
        except CancelledError:
            self.logger.debug('Audio feed task was cancelled. This is expected during disconnection.')
