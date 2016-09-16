import asyncio
import cozmo
from cozmo.util import degrees

'''This is a demo that people who are 'learning to code' could use.
It steps them through several things for Cozmo to do, growing in complexity.
Driving straight, 
Saying a for loop (1,2,3,4)
Driving in a square
Picking up and putting down objects
Playing an animation
'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    #######################################################

    # coz.drive_wheels(50,50, duration=3)

    #######################################################

    # for i in range(4):
    #     coz.say_text(str(i+1)).wait_for_completed()

    #######################################################

    # for i in range(4):
    #     coz.drive_wheels(50,50, duration=3)
    #     coz.turn_in_place(degrees(90)).wait_for_completed()

    #######################################################

    # cubes = coz.world.wait_until_observe_num_objects(num=2, object_type=cozmo.objects.LightCube, timeout=30)
    
    # coz.pickup_object(cubes[0]).wait_for_completed()
    # coz.place_on_object(cubes[1]).wait_for_completed()

    #######################################################

    # coz.play_anim_trigger(cozmo.anim.Triggers.CubePounceLoseSession).wait_for_completed()
    # AdmireStackAttemptGrabThirdCube moves a lot, yells, funny
    # MajorWin no sound
    # MajorFail no eyes?
    # OnWiggle move a little
    # StackBlocksSuccess move a little
    # SimonPointLeftBig turns

    #######################################################

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
