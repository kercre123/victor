## Quick test to see if python module is working

import sys
import spine
import signal


r = spine.init(sys.argv[1], 3000000)

while True:

    # send "df" data frame with random data.
    spine.send_frame(0x6466, "1234567689")
    frame = spine.read_frame()
    print(frame)
