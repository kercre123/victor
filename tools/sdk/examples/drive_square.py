import asyncio
import cozmo
from cozmo.util import degrees

'''This script is designed to show off the 'simple robot capabilites' of Cozmo
He will drive in a square by going forward and turning (left) 4 times.'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    for i in range(4):
        coz.drive_wheels(50, 50, duration=3)
        coz.turn_in_place(degrees(90)).wait_for_completed()

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
