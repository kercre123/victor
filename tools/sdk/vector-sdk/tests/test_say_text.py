#!/usr/bin/env python3

'''
Test say text
'''

import os
import sys
import time
import utilities

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector


def main():
    args = utilities.parse_args()

    print("------ begin testing text-to-speech ------")

    with vector.Robot(args.ip, str(args.cert), port=args.port) as robot:
        robot.say_text("hello", use_vector_voice=True)
        time.sleep(1) # Avoid overlapping messages
        robot.say_text("hello", use_vector_voice=False)

    print("------ end testing text-to-speech ------")




if __name__ == "__main__":
    main()
