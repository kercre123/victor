import asyncio
import time
import cozmo
from cozmo.util import degrees, Pose


def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    coz.go_to_pose_relative_robot(Pose(100,50,0,angle_z=degrees(-90)))

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
