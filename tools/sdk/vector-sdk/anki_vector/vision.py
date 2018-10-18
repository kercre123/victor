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

"""Support for Vector's vision

Vector's can detect various types of objects through his camera feed.

The :class:`VisionComponent` class defined in this module is made available as
:attr:`anki_vector.robot.Robot.vision` and can be used to enable/disable vision
processing on the robot.
"""

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['VisionComponent']

from . import util, sync
from .messaging import protocol


class VisionComponent(util.Component):  # pylint: disable=too-few-public-methods
    """VisionComponent exposes controls for the robot's internal image processing.

    The :class:`anki_vector.robot.Robot` or :class:`anki_vector.robot.AsyncRobot` instance owns this vision component.

    :param robot: A reference to the owner Robot object.
    """

    def __init__(self, robot):
        super().__init__(robot)

        self._vision_enabled = False

    @property
    def vision_enabled(self):
        return self._vision_enabled

    @sync.Synchronizer.wrap
    async def enable_vision_mode(self, enable: bool):
        """Enable facial and custom object detection on the robot's camera

        :param enable: Enable/Disable vision.

        .. testcode::

            import anki_vector
            with anki_vector.Robot("my_robot_serial_number") as robot:
                robot.enable_vision_mode(True)
        """
        self._vision_enabled = enable

        enable_vision_mode_request = protocol.EnableVisionModeRequest(mode=protocol.VisionMode.Value("VISION_MODE_DETECTING_MARKERS"), enable=enable)
        await self.grpc_interface.EnableVisionMode(enable_vision_mode_request)

        enable_vision_mode_request = protocol.EnableVisionModeRequest(mode=protocol.VisionMode.Value("VISION_MODE_FULL_FRAME_MARKER_DETECTION"), enable=enable)
        await self.grpc_interface.EnableVisionMode(enable_vision_mode_request)

        enable_vision_mode_request = protocol.EnableVisionModeRequest(mode=protocol.VisionMode.Value("VISION_MODE_DETECTING_FACES"), enable=enable)
        return await self.grpc_interface.EnableVisionMode(enable_vision_mode_request)
