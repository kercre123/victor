#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Blink each light on a cube until the cube is tapped.

Cozmo first looks around for a cube. Once a cube is found,
the cube's lights blink green in a circular fashion. The
script then waits for the cube to be tapped.
'''

import asyncio
import cozmo

class BlinkyCube(cozmo.objects.LightCube):
    '''Subclass LightCube and add a light-chaser effect.'''
    def __init__(self, *a, **kw):
        super().__init__(*a, **kw)
        self._chaser = None

    def start_light_chaser(self):
        '''
        Cycles the lights around the cube with 1 corner lit up green,
        changing to the next corner every 0.1 seconds.
        '''
        if self._chaser:
            raise ValueError("Light chaser already running")
        async def _chaser():
            while True:
                for i in range(4):
                    cols = [cozmo.lights.off_light] * 4
                    cols[i] = cozmo.lights.green_light
                    self.set_light_corners(*cols)
                    await asyncio.sleep(0.1, loop=self._loop)
        self._chaser = asyncio.ensure_future(_chaser(), loop=self._loop)

    def stop_light_chaser(self):
        if self._chaser:
            self._chaser.cancel()
            self._chaser = None


# Make sure World knows how to instantiate the subclass
cozmo.world.World.light_cube_factory = BlinkyCube



async def run(coz_conn):
    '''The run method runs once Cozmo is connected.'''
    cube = None
    coz = await coz_conn.wait_for_robot()
    print("Got initialized Cozmo")

    look_around = coz.start_behavior(cozmo.behavior.BehaviorTypes.LookAround)

    try:
        cube = await coz.world.wait_for_observed_light_cube(timeout=30)
    except asyncio.TimeoutError:
        print("Didn't find a cube :-(")
        return
    finally:
        look_around.stop()

    cube.start_light_chaser()
    try:
        print("Waiting for cube to be tapped")
        await cube.wait_for_tap(timeout=10)
        print("Cube tapped")
    except asyncio.TimeoutError:
        print("No-one tapped our cube :-(")
    finally:
        cube.stop_light_chaser()
        cube.set_lights_off()

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
