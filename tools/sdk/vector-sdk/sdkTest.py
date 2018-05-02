#!/usr/bin/env python3

import sys, os
# @TODO: we should import clad properly when we standardize the build process
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'vector-clad', 'clad', 'externalInterface'))
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', 'message-buffers', 'support', 'python'))

import argparse
import asyncio
import math
import struct

import websockets

import messageExternalComms
cladMessage = messageExternalComms.Anki.Cozmo.ExternalComms

class Robot:
    def __init__(self, socket):
        self.socket = socket

    # Unexposed low level message wrappers
    async def set_wheel_motors(self, left_wheel_speed, right_wheel_speed, left_wheel_accel, right_wheel_accel):
        message = cladMessage.DriveWheels(lwheel_speed_mmps=left_wheel_speed, rwheel_speed_mmps=right_wheel_speed, lwheel_accel_mmps2=left_wheel_accel, rwheel_accel_mmps2=right_wheel_accel)
        innerWrappedMessage = cladMessage.MotorControl(DriveWheels=message)
        outerWrappedMessage = cladMessage.ExternalComms(MotorControl=innerWrappedMessage)
        await self.socket.send(outerWrappedMessage.pack())

    async def set_wheel_motors_turn(self, turn_speed, turn_accel):
        message = cladMessage.DriveArc(speed=turn_speed, accel=turn_accel)
        innerWrappedMessage = cladMessage.MotorControl(DriveArc=message)
        outerWrappedMessage = cladMessage.ExternalComms(MotorControl=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_head_motor(self, speed):
        message = cladMessage.MoveHead(speed_rad_per_sec=speed)
        innerWrappedMessage = cladMessage.MotorControl(MoveHead=message)
        outerWrappedMessage = cladMessage.ExternalComms(MotorControl=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_lift_motor(self, speed):
        message = cladMessage.MoveLift(speed_rad_per_sec=speed)
        innerWrappedMessage = cladMessage.MotorControl(MoveLift=message)
        outerWrappedMessage = cladMessage.ExternalComms(MotorControl=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    # User facing exposed functions
    async def drive_straight(self, distance_mm, speed_mmps=100.0, accel_mmps2=0.0):
        # @TODO: When actions are exposed, this should be done through an explicit drive-straight action
        speed_sign = (distance_mm / abs(distance_mm)) if (distance_mm != 0) else 0
        wheel_speed = speed_mmps * speed_sign
        drive_time = abs(distance_mm) / speed_mmps

        await self.set_wheel_motors(wheel_speed, wheel_speed, accel_mmps2, accel_mmps2)
        await asyncio.sleep(drive_time)
        await self.set_wheel_motors(0.0, 0.0, 0.0, 0.0)

        self.action_in_progress = False

    async def turn_in_place(self, angle_radians, speed_radps=math.pi, accel_radps2=0.0):
        # @TODO: When actions are exposed, this should be done through an explicit turn-in-place action
        turn_sign = (angle_radians / abs(angle_radians)) if (angle_radians != 0) else 0
        turn_speed = speed_radps * turn_sign
        turn_accel = accel_radps2 * turn_sign
        turn_time = abs(angle_radians) / speed_radps

        await self.set_wheel_motors_turn(turn_speed, turn_accel)
        await asyncio.sleep(turn_time)
        await self.set_wheel_motors_turn(0.0, 0.0)
        self.action_in_progress = False

async def _bootstrap(main_function, uri):
    async with websockets.connect(uri) as websocket:
        robot = Robot(websocket)
        await main_function(robot)

def run_program(main_function, uri):
    try:
        loop = asyncio.get_event_loop()
        loop.run_until_complete(_bootstrap(main_function, uri))
    finally:
        loop.close()


async def main(robot):
    await robot.set_head_motor(5.0)
    await asyncio.sleep(1.0)
    await robot.set_head_motor(-5.0)
    await asyncio.sleep(1.0)
    await robot.set_head_motor(0.0)

    await robot.set_lift_motor(5.0)
    await asyncio.sleep(1.0)
    await robot.set_lift_motor(-5.0)
    await asyncio.sleep(1.0)
    await robot.set_lift_motor(0.0)

    await robot.drive_straight(100.0)
    await robot.turn_in_place(math.pi*2)
    await robot.drive_straight(-100.0)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Runs sdk python code on robot.')
    parser.add_argument('ip')
    args = parser.parse_args()
    connection_uri = 'ws://' + args.ip
    run_program(main, connection_uri)
