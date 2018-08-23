# Copyright (c) 2018 Anki, Inc.

"""Support for Vector's camera.

Vector has a built-in camera which he uses to observe the world around him.

The :class:`CameraComponent` class defined in this module is made available as
:attr:`anki_vector.robot.Robot.camera` and can be used to enable/disable image
sending and observe images being sent by the robot.
"""

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['CameraComponent']

import asyncio
from concurrent.futures import CancelledError
import sys

try:
    import cv2
except ImportError as exc:
    sys.exit("Cannot import opencv-python: Do `pip3 install opencv-python` to install")

from . import util
from .messaging import protocol

try:
    import numpy as np
except ImportError as exc:
    sys.exit("Cannot import numpy: Do `pip3 install numpy` to install")


class CameraComponent(util.Component):
    """Represents Vector's camera.

    The CameraComponent object receives images from Vector's camera, unpacks the data,
     composes it and makes it available as latest_image.

    The :class:`anki_vector.robot.Robot` or :class:`anki_vector.robot.AsyncRobot` instance observes the camera.

    .. code-block:: python

        from PIL import Image
        with anki_vector.Robot("Vector-XXXX", "XX.XX.XX.XX", "/some/path/robot.cert") as robot:
            image = Image.fromarray(camera.latest_image)
            image.show()

    :param robot: A reference to the owner Robot object. (May be :class:`None`)
    """

    def __init__(self, robot):
        super().__init__(robot)

        self._latest_image: np.ndarray = None
        self._latest_image_id: int = None
        self._partial_data: np.ndarray = None
        self._partial_image_id: int = None
        self._partial_invalid: bool = False
        self._partial_size: int = 0
        self._partial_metadata: protocol.ImageChunk = None
        self._last_chunk_id: int = -1
        self._camera_feed_task: asyncio.Task = None

    # TODO For Cozmo, latest_image was of Cozmo type CameraImage. np.ndarray is less friendly to work with. Should we change it and maybe bury np.ndarray to a less accessible location, like CameraImage.raw_image?
    @property
    def latest_image(self) -> np.ndarray:
        """:class:`numpy.ndarray`: The most recent processed image received from the robot, represented as an N-dimensional array of bytes.

        .. code-block:: python

            with anki_vector.Robot("Vector-XXXX", "XX.XX.XX.XX", "/some/path/robot.cert") as robot:
                image = Image.fromarray(robot.camera.latest_image)
                image.show()
        """

        return self._latest_image

    @latest_image.setter
    def latest_image(self, img) -> None:
        self._latest_image = img

    @property
    def latest_image_id(self) -> int:
        """int: The most recent processed image's id received from the robot."""
        return self._latest_image_id

    @latest_image_id.setter
    def latest_image_id(self, img_id) -> None:
        self._latest_image_id = img_id

    def init_camera_feed(self) -> None:
        if not self._camera_feed_task or self._camera_feed_task.done():
            self._camera_feed_task = self.robot.loop.create_task(self._handle_image_chunks())

    def close_camera_feed(self) -> None:
        if self._camera_feed_task:
            self._camera_feed_task.cancel()
            self.robot.loop.run_until_complete(self._camera_feed_task)

    def _reset_partial_state(self) -> None:
        self._partial_data = None
        self._partial_image_id = None
        self._partial_invalid = False
        self._partial_size = 0
        self._partial_metadata = None
        self._last_chunk_id = -1

    def _unpack_image_chunk(self, msg: protocol.CameraFeedResponse) -> None:
        if self._partial_image_id is not None and msg.chunk_id == 0:
            if not self._partial_invalid:
                self.logger.debug("Lost final chunk of image; discarding")
            self._partial_image_id = None

        if self._partial_image_id is None:
            if msg.chunk_id != 0:
                if not self._partial_invalid:
                    self.logger.debug("Received chunk of broken image")
                self._partial_invalid = True
                return
            # discard any previous in-progress image
            self._reset_partial_state()
            self._partial_image_id = msg.image_id
            self._partial_metadata = msg

            max_size = msg.width * msg.height * 3  # 3 bytes (RGB/BGR) per pixel
            self._partial_data = np.empty(max_size, dtype=np.uint8)

        if msg.chunk_id != (self._last_chunk_id + 1) or msg.image_id != self._partial_image_id:
            self.logger.debug("Image missing chunks; discarding (last_chunk_id=%d partial_image_id=%s)",
                              self._last_chunk_id, self._partial_image_id)
            self._reset_partial_state()
            self._partial_invalid = True
            return

        offset = self._partial_size

        self._partial_data[offset:offset + len(msg.data)] = list(msg.data)
        self._partial_size += len(msg.data)
        self._last_chunk_id = msg.chunk_id

        if msg.chunk_id == (msg.image_chunk_count - 1):
            self._process_completed_image()
            self._reset_partial_state()

    def _process_completed_image(self) -> None:
        data = self._partial_data[0:self._partial_size]

        # flag set to return the loaded image as is
        self.latest_image = cv2.imdecode(data, -1)
        self.latest_image_id = self._partial_metadata.image_id

    async def _handle_image_chunks(self) -> None:
        try:
            req = protocol.CameraFeedRequest()
            async for evt in self.interface.CameraFeed(req):
                # If the camera feed is disabled after stream is setup, exit the stream
                # (the camera feed on the robot is disabled internally on stream exit)
                if not self.robot.enable_camera_feed:
                    self.logger.debug('Camera feed has been disabled. Enable the feed to start/continue receiving camera feed data')
                    return
                self._unpack_image_chunk(evt.image_chunk)
        except CancelledError:
            self.logger.debug('Camera feed task was cancelled. This is expected during disconnection.')
