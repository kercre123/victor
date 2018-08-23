'''
Behaviors represent a complex task which requires Vector's
internal logic to determine how long it will take. This
may include combinations of animation, path planning or
other functionality. Examples include drive_on_charger,
set_lift_height, fist_bump, etc.

The :class:`BehaviorComponent` class in this module contains
functions for all the behaviors.

Copyright (c) 2018 Anki, Inc.
'''

# TODO:
# __all__ should order by constants, event classes, other classes, functions.
__all__ = ["BehaviorComponent"]


from . import sync, util
from .messaging import protocol


class BehaviorComponent(util.Component):
    def __init__(self, robot):
        super().__init__(robot)
        self._current_priority = None
        self._is_active = False

    @property
    def current_priority(self):
        return self._current_priority

    @property
    def is_active(self):
        return self._is_active

    @sync.Synchronizer.wrap
    async def drive_off_charger(self):
        drive_off_charger_request = protocol.DriveOffChargerRequest()
        return await self.interface.DriveOffCharger(drive_off_charger_request)

    @sync.Synchronizer.wrap
    async def drive_on_charger(self):
        drive_on_charger_request = protocol.DriveOnChargerRequest()
        return await self.interface.DriveOnCharger(drive_on_charger_request)

    @sync.Synchronizer.wrap
    async def go_to_pose(self, pose, relative_to_robot=False, motion_prof_map=None):
        '''Tells Vector to drive to the specified pose and orientation.

        If relative_to_robot is set to True, the given pose will assume the
        robot's pose as its origin.

        Since the robot understands position by monitoring its tread movement,
        it does not understand movement in the z axis. This means that the only
        applicable elements of pose in this situation are position.x position.y
        and rotation.angle_z.

        Args:
            pose: (:class:`anki_vector.util.Pose`): The destination pose.
            relative_to_robot (bool): Whether the given pose is relative to
                the robot's pose.
            motion_prof_map (dict): Provide custom speed, acceleration and deceleration
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

        Returns:
            A :class:`anki_vector.messaging.external_interface_pb2.GoToPoseResult` object which
            provides an action result.
        '''
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
            "is_custom": 0
        }
        if motion_prof_map is None:
            motion_prof_map = {}
        if relative_to_robot and self.robot.pose:
            pose = self.robot.pose.define_pose_relative_this(pose)
        default_motion_profile.update(motion_prof_map)
        motion_prof = protocol.PathMotionProfile(**default_motion_profile)
        go_to_pose_request = protocol.GoToPoseRequest(x_mm=pose.position.x,
                                                      y_mm=pose.position.y,
                                                      rad=pose.rotation.angle_z.radians,
                                                      motion_prof=motion_prof)
        return await self.interface.GoToPose(go_to_pose_request)

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
