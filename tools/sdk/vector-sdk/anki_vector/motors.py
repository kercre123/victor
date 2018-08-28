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

'''
Control the motors of Victor
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['MotorComponent']

from . import sync, util
from .messaging import protocol


class MotorComponent(util.Component):
    '''Controls the low-level motor functions'''
    @sync.Synchronizer.wrap
    async def set_wheel_motors(self, left_wheel_speed, right_wheel_speed, left_wheel_accel=0.0, right_wheel_accel=0.0):
        motors = protocol.DriveWheelsRequest(left_wheel_mmps=left_wheel_speed,
                                             right_wheel_mmps=right_wheel_speed,
                                             left_wheel_mmps2=left_wheel_accel,
                                             right_wheel_mmps2=right_wheel_accel)
        return await self.interface.DriveWheels(motors)

    @sync.Synchronizer.wrap
    async def set_wheel_motors_turn(self, turn_speed, turn_accel=0.0, curvature_radius_mm=0):
        drive_arc_request = protocol.DriveArcRequest(speed=turn_speed, accel=turn_accel, curvature_radius_mm=int(curvature_radius_mm))
        return await self.interface.DriveArc(drive_arc_request)

    @sync.Synchronizer.wrap
    async def set_head_motor(self, speed):
        set_head_request = protocol.MoveHeadRequest(speed_rad_per_sec=speed)
        return await self.interface.MoveHead(set_head_request)

    @sync.Synchronizer.wrap
    async def set_lift_motor(self, speed):
        set_lift_request = protocol.MoveLiftRequest(speed_rad_per_sec=speed)
        return await self.interface.MoveLift(set_lift_request)
