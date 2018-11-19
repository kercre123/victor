#!/usr/bin/env python3

"""
Test subscribing to seen faces
"""

import functools
import time

from anki_vector.events import Events  # pylint: disable=wrong-import-position
import anki_vector  # pylint: disable=wrong-import-position


def main():
    args = anki_vector.util.parse_command_args()

    print("------ begin testing face events ------")
    # Should see a stream of face events when the robot observes a face

    def test_subscriber(robot, event_type, event):
        """output when a face is seen"""
        # Print the stream data received
        print(f"Subscriber called for: {event_type} = {event}")

        # Print the visible face's attributes
        for face in robot.world.visible_faces:
            print("Face attributes:")
            print(f"Face id: {face.face_id}")
            print(f"Updated face id: {face.updated_face_id}")
            print(f"Name: {face.name}")
            print(f"Expression: {face.expression}")
            print(f"Timestamp: {face.last_observed_time}")
            print(f"Pose: {face.pose}")
            print(f"Image Rect: {face.last_observed_image_rect}")
            print(f"Expression score: {face.expression_score}")
            print(f"Left eye: {face.left_eye}")
            print(f"Right eye: {face.right_eye}")
            print(f"Nose: {face.nose}")
            print(f"Mouth: {face.mouth}")

    with anki_vector.Robot(args.serial, enable_face_detection=True) as robot:
        test_subscriber = functools.partial(test_subscriber, robot)
        robot.events.subscribe(test_subscriber, Events.robot_changed_observed_face_id)
        robot.events.subscribe(test_subscriber, Events.robot_observed_face)

        print("------ waiting for face events, press ctrl+c to exit early ------")
        try:
            time.sleep(10)
        except KeyboardInterrupt:
            print("------ finished testing face events ------")
            robot.disconnect()


if __name__ == '__main__':
    main()
