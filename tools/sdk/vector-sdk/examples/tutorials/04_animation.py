#!/usr/bin/env python3

# Copyright (c) 2018 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Play some animations on Vector

Play an animation using a trigger, and then another animation by name.
"""

import os
from pathlib import Path
import sys
import vector

def main():
    args = vector.util.parse_test_args()
    with vector.Robot(args.name, args.ip, str(args.cert), port=args.port) as robot:
        # Play an animation via a Trigger - see list of available triggers here:
        # TODO show list of animation triggers
        # A trigger can pick from several appropriate animations for variety.
        # TODO add call to play_animation_trigger
        #print("Playing animation by trigger")
        #robot.play_anim_trigger(cozmo.anim.Triggers.CubePounceLoseSession).wait_for_completed()

        # Play an animation via its Name.
        # Warning: Future versions of the app might change these, so for future-proofing
        # we recommend using play_anim_trigger above instead.
        # See the remote_control_cozmo.py example in apps for an easy way to see
        # the available animations.
        print("Playing animation by name")
        robot.anim.play_animation("anim_weather_snow_01")

if __name__ == "__main__":
    main()
