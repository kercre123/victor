#!/usr/bin/env python3

import base64
import sys, os

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'vector-sdk'))
import anki_vector

color = anki_vector.color.Color(rgb=[255, 128, 0])
raw_565_data = bytes(color.rgb565_bytepair * 17664)
encoded_data = (base64.b64encode(raw_565_data)).decode('utf-8')

jsonText = '{"face_data":"' + encoded_data + '", "duration_ms":2000, "interrupt_running":false}'

f = open('test_assets/oled_color.json', 'w')
f.write(jsonText)
f.close()
