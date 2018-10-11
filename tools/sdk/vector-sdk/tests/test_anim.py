#!/usr/bin/env python3

"""
Test playing and listing animations
"""

import os
from pathlib import Path
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import anki_vector  # pylint: disable=wrong-import-position

CUSTOM_ANIM_FOLDER = Path("test_assets", "custom_animations")


def main():
    """main execution"""
    args = anki_vector.util.parse_command_args()

    print("------ begin testing animations ------")

    with anki_vector.Robot(args.serial, cache_animation_list=False) as robot:
        print("------ testing sync animations ------")
        robot.anim.play_animation("anim_pounce_01")

    with anki_vector.Robot(args.serial, cache_animation_list=False) as robot:
        print("------ testing sync load animations ------")
        anim_names = robot.anim.anim_list
        if not anim_names:
            sys.exit("Error: no animations loaded")

    with anki_vector.AsyncRobot(args.serial, cache_animation_list=False) as robot:
        print("------ testing async animations ------")
        robot.run_until_complete(robot.anim.play_animation("anim_pounce_01"))

    with anki_vector.AsyncRobot(args.serial, cache_animation_list=False) as robot:
        print("------ testing async load animations ------")
        anim_names = robot.anim.anim_list
        if not anim_names:
            sys.exit("Error: no animations loaded")

    print("------ finish testing animations ------")


if __name__ == "__main__":
    main()
