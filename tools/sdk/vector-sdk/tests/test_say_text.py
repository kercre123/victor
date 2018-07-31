#!/usr/bin/env python3

'''
Test say text
'''

import os
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector  # pylint: disable=wrong-import-position


def main():
    args = vector.util.parse_test_args()

    print("------ begin testing text-to-speech ------")

    with vector.Robot(args.name, args.ip, str(args.cert), port=args.port) as robot:
        robot.say_text("hello", use_vector_voice=True)
        time.sleep(1)  # Avoid overlapping messages
        robot.say_text("hello", use_vector_voice=False)

    print("------ end testing text-to-speech ------")


if __name__ == "__main__":
    main()
