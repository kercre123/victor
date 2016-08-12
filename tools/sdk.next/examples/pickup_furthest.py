import asyncio
import time
import cozmo

'''This script shows simple math with object's poses. 
It waits for 3 cubes, and then attempts to pick up the furthest one. 
It calculates this based on the reported poses of the cubes.'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    
    cubes = coz.world.wait_until_observe_num_objects(num=3, object_type=cozmo.objects.LightCube, timeout=30)

    max_dst, targ = 0, None
    for cube in cubes:
        translation = coz.pose - cube.pose
        dst = translation.position.x ** 2 + translation.position.y ** 2
        if dst > max_dst:
            max_dst, targ = dst, cube

    coz.pickup_object(targ).wait_for_completed()


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
