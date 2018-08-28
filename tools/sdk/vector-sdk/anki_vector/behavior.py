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
Behaviors represent a complex task which requires Vector's
internal logic to determine how long it will take. This
may include combinations of animation, path planning or
other functionality. Examples include drive_on_charger,
set_lift_height, fist_bump, etc.

The :class:`BehaviorComponent` class in this module contains
functions for all the behaviors.
"""

# TODO:
# __all__ should order by constants, event classes, other classes, functions.
__all__ = ["BehaviorComponent"]


from . import objects, sync, util
from .messaging import protocol


class BehaviorComponent(util.Component):
    def __init__(self, robot):
        super().__init__(robot)
        self._current_priority = None
        self._is_active = False

        self._motion_profile_map = {}

    @property
    def motion_profile_map(self) -> dict:
        return self._motion_profile_map

    @motion_profile_map.setter
    def motion_profile_map(self, motion_profile_map: dict):
        """Tells Vector how to drive when receiving navigation and movement actions
        such as go_to_pose and dock_with_cube.

        :param motion_prof_map: Provide custom speed, acceleration and deceleration
            values with which the robot goes to the given pose.
            speed_mmps (float)
            accel_mmps2 (float)
            decel_mmps2 (float)
            point_turn_speed_rad_per_sec (float)
            point_turn_accel_rad_per_sec2 (float)
            point_turn_decel_rad_per_sec2 (float)
            dock_speed_mmps (float)
            dock_accel_mmps2 (float)
            dock_decel_mmps2 (float)
            reverse_speed_mmps (float)
            is_custom (bool)
        """
        self._motion_profile_map = motion_profile_map

    def _motion_profile_for_proto(self) -> protocol.PathMotionProfile:
        """Packages the current motion profile into a proto object

        Returns:
            A profile object describing motion which can be passed with any proto action message.
        """
        # TODO: This should be made into its own class
        default_motion_profile = {
            "speed_mmps": 100.0,
            "accel_mmps2": 200.0,
            "decel_mmps2": 500.0,
            "point_turn_speed_rad_per_sec": 2.0,
            "point_turn_accel_rad_per_sec2": 10.0,
            "point_turn_decel_rad_per_sec2": 10.0,
            "dock_speed_mmps": 60.0,
            "dock_accel_mmps2": 200.0,
            "dock_decel_mmps2": 500.0,
            "reverse_speed_mmps": 80.0,
            "is_custom": 1 if self._motion_profile_map else 0
        }
        default_motion_profile.update(self._motion_profile_map)

        return protocol.PathMotionProfile(**default_motion_profile)

    @property
    def current_priority(self):
        return self._current_priority

    @property
    def is_active(self):
        return self._is_active

    # Navigation actions
    @sync.Synchronizer.wrap
    async def drive_off_charger(self):
        drive_off_charger_request = protocol.DriveOffChargerRequest()
        return await self.interface.DriveOffCharger(drive_off_charger_request)

    @sync.Synchronizer.wrap
    async def drive_on_charger(self):
        drive_on_charger_request = protocol.DriveOnChargerRequest()
        return await self.interface.DriveOnCharger(drive_on_charger_request)

    @sync.Synchronizer.wrap
    async def go_to_pose(self,
                         pose: util.Pose,
                         relative_to_robot: bool = False) -> protocol.GoToPoseResponse:
        """Tells Vector to drive to the specified pose and orientation.

        If relative_to_robot is set to True, the given pose will assume the
        robot's pose as its origin.

        Since the robot understands position by monitoring its tread movement,
        it does not understand movement in the z axis. This means that the only
        applicable elements of pose in this situation are position.x position.y
        and rotation.angle_z.

        :param pose: The destination pose.
        :param relative_to_robot: Whether the given pose is relative to
                                  the robot's pose.
        :param num_retries: Number of times to re-attempt action in case of a failure.

        Returns:
            An object which provides an action result.
        """
        if relative_to_robot and self.robot.pose:
            pose = self.robot.pose.define_pose_relative_this(pose)

        motion_prof = self._motion_profile_for_proto()
        go_to_pose_request = protocol.GoToPoseRequest(x_mm=pose.position.x,
                                                      y_mm=pose.position.y,
                                                      rad=pose.rotation.angle_z.radians,
                                                      motion_prof=motion_prof)
        return await self.interface.GoToPose(go_to_pose_request)

    @sync.Synchronizer.wrap
    async def dock_with_cube(self,
                             target_object: objects.LightCube,
                             approach_angle: util.Angle = None,
                             alignment_type: protocol.AlignmentType = protocol.ALIGNMENT_TYPE_LIFT_PLATE,
                             distance_from_marker: util.Distance = None) -> protocol.DockWithCubeResponse:
        """Tells Vector to dock with a light cube with a given approach angle and distance.

        :param target_object: The LightCube object to dock with.
        :param approach_angle: Angle to approach the dock with.
        :param alignment_type: Which part of the robot to align with the object.
        :param distance_from_marker: How far from the object to approach (0 to dock)
        :param num_retries: Number of times to re-attempt action in case of a failure.

        Returns:
            An object providing an action result.

        .. code-block:: python

            connected_cubes = robot.world.connected_light_cubes
            if connected_cubes:
                robot.behavior.dock_with_cube(object_id=connected_cube[0])
        """
        motion_prof = self._motion_profile_for_proto()

        dock_request = protocol.DockWithCubeRequest(object_id=target_object.object_id, alignment_type=alignment_type, motion_prof=motion_prof)
        if approach_angle is not None:
            dock_request.use_approach_angle = True
            dock_request.use_pre_dock_pose = True
            dock_request.approach_angle = approach_angle.radians
        if distance_from_marker is not None:
            dock_request.distance_from_marker = distance_from_marker.distance_mm

        return await self.interface.DockWithCube(dock_request)

    # Movement actions
    @sync.Synchronizer.wrap
    async def drive_straight(self, distance, speed, should_play_anim=False):
        '''Tells Vector to drive in a straight line

        Vector will drive for the specified distance (forwards or backwards)

        Vector must be off of the charger for this movement action.

        Args:
            distance (anki_vector.util.Distance): The distance to drive
                (>0 for forwards, <0 for backwards)
            speed (anki_vector.util.Speed): The speed to drive at
                (should always be >0, the abs(speed) is used internally)
            should_play_anim (bool): Whether to play idle animations
                whilst driving (tilt head, hum, animated eyes, etc.)

        Returns:
           A :class:`protocol.DriveStraightResponse` object which provides the result description.
        '''
        drive_straight_request = protocol.DriveStraightRequest(speed_mmps=speed.speed_mmps,
                                                               dist_mm=distance.distance_mm,
                                                               should_play_animation=should_play_anim)
        return await self.interface.DriveStraight(drive_straight_request)

    @sync.Synchronizer.wrap
    async def turn_in_place(self, angle, speed=util.Angle(0.0), accel=util.Angle(0.0), angle_tolerance=util.Angle(0.0), is_absolute=0):
        '''Turn the robot around its current position.

        Vector must be off of the charger for this movement action.

        Args:
            angle (anki_vector.util.Angle): The angle to turn. Positive
                values turn to the left, negative values to the right.
            speed (anki_vector.util.Angle): Angular turn speed (per second).
            accel (anki_vector.util.Angle): Acceleration of angular turn
                (per second squared).
            angle_tolerance (anki_vector.util.Angle): angular tolerance
                to consider the action complete (this is clamped to a minimum
                of 2 degrees internally).
            is_absolute (bool): True to turn to a specific angle, False to
                turn relative to the current pose.

        Returns:
            A :class:`protocol.TurnInPlaceResponse` object which provides the result description.
        '''
        turn_in_place_request = protocol.TurnInPlaceRequest(angle_rad=angle.radians,
                                                            speed_rad_per_sec=speed.radians,
                                                            accel_rad_per_sec2=accel.radians,
                                                            tol_rad=angle_tolerance.radians,
                                                            is_absolute=is_absolute)
        return await self.interface.TurnInPlace(turn_in_place_request)

    @sync.Synchronizer.wrap
    async def set_head_angle(self, angle, accel=10.0, max_speed=10.0, duration=0.0):
        '''Tell Vector's head to turn to a given angle.

        Args:
            angle: (:class:`anki_vector.util.Angle`): Desired angle for
                Vector's head. (:const:`MIN_HEAD_ANGLE` to
                :const:`MAX_HEAD_ANGLE`).
            max_speed (float): Maximum speed of Vector's head in radians per second.
            accel (float): Acceleration of Vector's head in radians per second squared.
            duration (float): Time for Vector's head to turn in seconds. A value
                of zero will make Vector try to do it as quickly as possible.

        Returns:
            A :class:`protocol.SetHeadAngleResponse` object which provides the result description.
        '''
        set_head_angle_request = protocol.SetHeadAngleRequest(angle_rad=angle.radians,
                                                              max_speed_rad_per_sec=max_speed,
                                                              accel_rad_per_sec2=accel,
                                                              duration_sec=duration)
        return await self.interface.SetHeadAngle(set_head_angle_request)

    @sync.Synchronizer.wrap
    async def set_lift_height(self, height, accel=10.0, max_speed=10.0, duration=0.0):
        '''Tell Vector's lift to move to a given height

        Args:
            height (float): desired height for Vector's lift 0.0 (bottom) to
                1.0 (top) (we clamp it to this range internally).
            max_speed (float): Maximum speed of Vector's lift in radians per second.
            accel (float): Acceleration of Vector's lift in radians per
                second squared.
            duration (float): Time for Vector's lift to move in seconds. A value
                of zero will make Vector try to do it as quickly as possible.

        Returns:
            A :class:`protocol.SetLiftHeightResponse` object which provides the result description.
        '''
        set_lift_height_request = protocol.SetLiftHeightRequest(height_mm=height,
                                                                max_speed_rad_per_sec=max_speed,
                                                                accel_rad_per_sec2=accel,
                                                                duration_sec=duration)
        return await self.interface.SetLiftHeight(set_lift_height_request)
