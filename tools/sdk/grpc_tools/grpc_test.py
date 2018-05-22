#!/usr/bin/env python3

import grpc
import aiogrpc
import asyncio

import external_interface_pb2 as external
import external_interface_pb2_grpc as connection

import sys


class Action:
    def __init__(self, that, func, *args, **kwargs):
        self.remove_pending = that.remove_pending
        self.loop = that.loop
        self.task = self.loop.create_task(func(*args, **kwargs))

    def wait_for_completed(self):
        try:
            self.loop.run_until_complete(self.task)
        except grpc._channel._Rendezvous as e:
            raise e
        finally:
            self.remove_pending(self)

def _actionable(func):
    def waitable(*args, **kwargs):
        that = args[0]
        if that.complicated:
            action = Action(that, func, *args, **kwargs)
            that.add_pending(action)
            return action
        that.loop.run_until_complete(func(*args, **kwargs))
        return None # TODO: Make this a shared object
    return waitable

class Robot:
    def __init__(self, ip='192.168.42.81:443', loop=None, complicated=False):
        self.complicated = complicated
        self.loop = asyncio.get_event_loop()
        with open('trust.cert', 'rb') as f:
            trusted_certs = f.read()
        credentials = aiogrpc.ssl_channel_credentials(root_certificates=trusted_certs)
        channel = aiogrpc.secure_channel(ip, credentials)

        self.connection = connection.ExternalInterfaceStub(channel)
        self.pending = []

    @_actionable
    async def drive_wheels(self, left_wheel_mmps=100.0, right_wheel_mmps=100.0):
        motors = external.DriveWheelsRequest(left_wheel_mmps=left_wheel_mmps,
                                 right_wheel_mmps=right_wheel_mmps,
                                 left_wheel_mmps2=200.0,
                                 right_wheel_mmps2=200.0)
        result = await self.connection.DriveWheels(motors)
        print(f'{type(result)}: {str(result).strip()}')

    @_actionable
    async def play_animation(self, name, loops=1):
        anim = external.Animation(name=name)
        req = external.PlayAnimationRequest(animation=anim, loops=loops)
        result = await self.connection.PlayAnimation(req)
        print(f'{type(result)}: {str(result).strip()}')

    @_actionable
    async def list_animations(self):
        print("List animations: Start")
        animList = await self.connection.ListAnimations()
        print(f'{type(animList)}: {str(animList).strip()} : {animList.animations}')
        print("List animations: Done")

    def disconnect(self):
        for task in self.pending:
            task.wait_for_completed()
        self.loop.close()

    def add_pending(self, task):
        self.pending += [task]

    def remove_pending(self, task):
        self.pending = [x for x in self.pending if x is not task]

class AsyncRobot(Robot):
    def __init__(self, *args, **kwargs):
        super(AsyncRobot, self).__init__(*args, **kwargs, complicated=True)

def main():
    try:
        robot = AsyncRobot()
        robot.play_animation("anim_poked_giggle").wait_for_completed()
        robot.drive_wheels().wait_for_completed()
    finally:
        if robot is not None:
            robot.disconnect()

if __name__ == '__main__':
    main()
