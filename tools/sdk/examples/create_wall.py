import asyncio
import cozmo
import time
from cozmo.util import degrees, Pose

'''This script is meant to show off custom objects. 
It creates a wall in front of cozmo and tells him to drive around it.
It also binds an object to the STAR5_Box object, 
so you could use that to create an additional wall.
He will plan a path to drive 200mm in front of himself after these objects are created.
'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    fixed_object = coz.world.create_custom_fixed_object(Pose(100,0,0,angle_z=degrees(0)), 
                                                        10, 100, 100, relative_to_robot=True)


    coz.world.define_custom_object(cozmo.objects.CustomObjectTypes.Custom_STAR5_Box, 
                                   10, 300, 10)

    coz.go_to_pose(Pose(200,0,0,angle_z=degrees(0)), relative_to_robot=True).wait_for_completed()


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
