#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import cozmo
import time

'''Animate and display Cozmo's procedural face
'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    
    # bounce left eye between each side
    l_eye_cen_x = -100.0
    l_eye_cen_vx = 1.0
    coz.set_procedural_face_anim_params()
    while True:
        coz.display_procedural_face(l_eye_cen_x = l_eye_cen_x, duration_ms = 100)
        l_eye_cen_x += l_eye_cen_vx
        if (l_eye_cen_x > 100.0) and (l_eye_cen_vx > 0.0):
            l_eye_cen_vx = -l_eye_cen_vx
        if (l_eye_cen_x < -60.0) and (l_eye_cen_vx < 0.0):
            l_eye_cen_vx = -l_eye_cen_vx
        time.sleep(0.1)
        #print("l_eye_cen_x = %s" % l_eye_cen_x)


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
