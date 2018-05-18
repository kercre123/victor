#!/usr/bin/env python3

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):
    print("------ testing animations ------")
    print("receiving all loaded animations")
    anim_names = await robot.get_anim_names(enable_diagnostics=False)
    if len(anim_names) == 0:
        print("Error: no animations loaded")
    else:
        for n in range(0, len(anim_names)):
            s = anim_names[n].decode("utf_8")
            s = s[s.find("anim_"):]
            anim_names[n] = s
            print("%d: %s" % (n+1, anim_names[n])),
    print("playing animation by name: anim_poked_giggle")
    await robot.play_anim("anim_poked_giggle")

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)