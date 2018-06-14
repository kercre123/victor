#!/usr/bin/env python3

# This test should place an image of cozmo on vector's face for 4 seconds


import argparse
import os
from pathlib import Path
import sys
import time

try:
    from PIL import Image
except ImportError:
    sys.exit("Cannot import from PIL: Do `pip3 install --user Pillow` to install")

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()
    cert = Path(args.cert_file)
    cert.resolve()

    with vector.Robot(args.ip, str(cert)) as robot:
        print("------ begin testing oled ------")

        current_directory = os.path.dirname(os.path.realpath(__file__))
        image_path = os.path.join(current_directory, "test_assets", "cozmo_image.jpg")

        image_file = Image.open(image_path)
        screen_data = vector.oled_face.convert_image_to_screen_data(image_file)
        robot.set_oled_with_screen_data(screen_data, 4.0)

        print("------ finish testing oled ------")

if __name__ == "__main__":
    main()
