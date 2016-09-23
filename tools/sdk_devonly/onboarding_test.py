#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import cozmo

from cozmo import _clad
from cozmo._clad import _clad_to_engine_iface, _clad_to_game_iface

from cozmo.util import degrees

import sys

''' Utility to help with testing Cozmo onboarding by simulating what the unity app does for phases 1 and 2
'''

# literally any argument will disable animations.....
DO_ANIMS = len(sys.argv) == 1

# HACK to generate strings for enums, hopefully this will be supported in the clad emitter / SDK directly soon
OnboardingEnumToStringMap = {}

for k in _clad_to_game_iface.OnboardingStateEnum.__dict__:
    if k and isinstance(k, str) and k[0] != '_':
        v = _clad_to_game_iface.OnboardingStateEnum.__dict__[k]
        if v and isinstance(v, int):
            OnboardingEnumToStringMap[v] = k

def OnboardingEnumToString(enumVal):
    if enumVal in OnboardingEnumToStringMap:
        return OnboardingEnumToStringMap[ enumVal ]
    else:
        return "<UNKNOWN>"
            
def handle_onboarding_state(evt, *, msg):
    print("[onboarding state:", OnboardingEnumToString(msg.stateNum), "]")

def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    coz_conn.add_event_handler(_clad._MsgOnboardingState, handle_onboarding_state)
    
    if DO_ANIMS:
        anim = coz.play_anim_trigger(cozmo.anim.Triggers.OnboardingBirth)
        anim.wait_for_completed()

        anim = coz.play_anim_trigger(cozmo.anim.Triggers.OnboardingHelloWorld)
        anim.wait_for_completed()
    else:
        coz.set_head_angle(degrees(0)).wait_for_completed()

    # the behavior actually does this for us
    ## print("Waiting for cube...")
    ## cube = coz.world.wait_for_observed_light_cube()
    ## print("Found cube", cube.object_id)

    cube_tutorial = coz.start_behavior(cozmo.behavior.BehaviorTypes.OnboardingShowCube)
    print("running tutorial behavior")

    try:
        while True:
            input("press enter to send TransitionToNextOnboardingState...\n")
            coz_conn.send_msg( _clad_to_engine_iface.TransitionToNextOnboardingState() )
            print("sent")
            
    except KeyboardInterrupt:
        pass
    
    print("all done!")

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
