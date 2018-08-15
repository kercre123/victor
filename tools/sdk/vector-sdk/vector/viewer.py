# Copyright (c) 2018 Anki, Inc.

"""Module to render camera feed received from Vector's camera.
"""

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['ViewerComponent']

import asyncio
from concurrent.futures import CancelledError
import sys
import time

try:
    import cv2
except ImportError as exc:
    sys.exit("pip3 install opencv-python")

from .exceptions import VectorCameraFeedDisabledException
from . import util


class ViewerComponent(util.Component):
    """This component is used to render a video using the images
    obtained from Vector's camera

    .. code-block:: python
        with vector.Robot("Vector-XXXX", "XX.XX.XX.XX", "/some/path/robot.cert", show_viewer=True) as robot:
            robot.loop.run_until_complete(utilities.delay_close(5))

    :param robot: A reference to the owner Robot object. (May be :class:`None`)
    """

    def __init__(self, robot):
        super().__init__(robot)
        self.render_task: asyncio.Task = None

    @staticmethod
    def _close_window(window_name: str) -> None:
        # Close the openCV window
        cv2.destroyWindow(window_name)
        cv2.waitKey(1)

    async def _render_frames(self, timeout) -> None:
        latest_image_id = None
        opencv_window_name = "Vector Camera Feed"
        cv2.namedWindow(opencv_window_name, cv2.WINDOW_NORMAL)
        start_time = time.time()
        try:
            while True:
                # Stop rendering if feed is disabled
                if not self.robot.enable_camera_feed:
                    raise VectorCameraFeedDisabledException()

                if timeout and ((time.time() - start_time) >= timeout):
                    # Close the openCV window
                    self._close_window(opencv_window_name)
                    break

                # Render image only if new image is available
                if self.robot.camera.latest_image_id != latest_image_id:
                    cv2.imshow(opencv_window_name, self.robot.camera.latest_image)
                    cv2.waitKey(1)
                    latest_image_id = self.robot.camera.latest_image_id
                await asyncio.sleep(0.1)
        except CancelledError:
            self.logger.debug('Event handler task was cancelled. This is expected during disconnection.')
            # Close the openCV window
            self._close_window(opencv_window_name)

    def show_video(self, timeout: float = None) -> None:
        """Render a video stream using the images obtained from
        Vector's camera feed.

        .. code-block:: python

            with vector.Robot("Vector-XXXX", "XX.XX.XX.XX", "/some/path/robot.cert") as robot:
                robot.viewer.show_video()
                robot.loop.run_until_complete(utilities.delay_close(5))

        :param timeout: Render video for the given time. (Renders forever, if timeout not given)
        """
        if not self.render_task or self.render_task.done():
            self.render_task = self.robot.loop.create_task(self._render_frames(timeout))

    def stop_video(self) -> None:
        """Stop rendering video of Vector's camera feed

        with vector.Robot("Vector-XXXX", "XX.XX.XX.XX", "/some/path/robot.cert", show_viewer=True) as robot:
                robot.loop.run_until_complete(utilities.delay_close(5))
                robot.viewer.stop_video()
                robot.loop.run_until_complete(utilities.delay_close(5))
        """
        if self.render_task:
            self.render_task.cancel()
            self.robot.loop.run_until_complete(self.render_task)
