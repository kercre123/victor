#!/usr/bin/env python3

"""
Test playing and listing animations
"""

from pathlib import Path
import sys

import anki_vector  # pylint: disable=wrong-import-position

CUSTOM_ANIM_FOLDER = Path("test_assets", "custom_animations")


def main():
    args = anki_vector.util.parse_command_args()

    print("------ begin testing animations ------")

    with anki_vector.Robot(args.serial, cache_animation_lists=False) as robot:
        print("playing animation by name: anim_pounce_success_02")
        robot.anim.play_animation("anim_pounce_success_02")

    with anki_vector.AsyncRobot(args.serial, cache_animation_lists=False) as robot:
        print("------ testing load async animations ------")

        print("receiving all loaded animations")
        anim_names = robot.anim.anim_list
        if not anim_names:
            sys.exit("Error: no animations loaded")
        else:
            for idx, name in enumerate(anim_names):
                print("(%d: %s)" % (idx, name), end=" ")
            print()
        robot.anim.play_animation("anim_pounce_success_02").result()

        print("------ finish testing animations ------")


if __name__ == "__main__":
    main()
