import asyncio
import time
import cozmo
from cozmo.util import degrees, Pose

'''This script shows off the turn_towards_face action. It will wait for a face
and then constantly turn towards it to keep it in frame.'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    
    face = coz.world.wait_for_observed_face(timeout=30)
    
    while True:
        coz.turn_towards_face(face).wait_for_completed()
        time.sleep(.1)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
