import collections
import functools
import queue

import tkinter
from tkinter import *
from PIL import ImageTk
from PIL import Image, ImageDraw
import threading

from . import world


class TkThreadable:
    def __init__(self, *a, **kw):
        self._thread_queue = queue.Queue()
        self.after(50, self._thread_call_dispatch)

    def call_threadsafe(self, method, *a, **kw):
        self._thread_queue.put((method, a, kw))

    def _thread_call_dispatch(self):
        while True:
            try:
                method, a, kw = self._thread_queue.get(block=False)
                self.after_idle(method, *a, **kw)
            except queue.Empty:
                break
        self.after(50, self._thread_call_dispatch)


class TkImageViewer(Frame, TkThreadable):
    def __init__(self,
            tk_root=tkinter.Tk(), refresh_interval=10, image_scale = 2,
            window_name = "CozmoView", force_on_top=True):
        Frame.__init__(self, tk_root)
        TkThreadable.__init__(self)
        self._img_queue = collections.deque(maxlen=1)

        self._refresh_interval = refresh_interval
        self.scale = image_scale
        self.tk_root = tk_root
        self.display = Canvas(self)

        tk_root.wm_title(window_name)

        self.label  = tkinter.Label(self.tk_root,image=None)
        self.tk_root.protocol("WM_DELETE_WINDOW", self._delete_window)
        self._isRunning = True
        self.robot = None

        if force_on_top:
            # force window on top of all others, regardless of focus
            tk_root.wm_attributes("-topmost", 1)

        self._repeat_draw_frame()

    async def connect(self, coz_conn):
        self.robot = await coz_conn.wait_for_robot()
        self.robot.camera.image_stream_enabled = True
        self.handler = self.robot.world.add_event_handler(world.EvtNewCameraImage, self.image_event)

    def disconnect(self):
        self.handler.disable()
        self.call_threadsafe(self.quit)

    def image_event(self, evt, *, image, **kw):
        self._img_queue.append(image.raw_image)

    def _delete_window(self):
        # this is not thread safe :-(
        #self._handler.disable()

        self.tk_root.destroy()
        self.quit()
        self._isRunning = False
        sys.exit()

    def _draw_frame(self):
        try:
            image = self._img_queue.popleft()
        except IndexError:
            # no new image
            return

        if self.scale != 1:
            width  = image.width
            height = image.height

            image = image.resize((self.scale * width, self.scale * height), Image.ANTIALIAS)

        photoImage = ImageTk.PhotoImage(image)

        self.label.configure(image=photoImage)
        self.label.image = photoImage
        self.label.pack()

    def _repeat_draw_frame(self, event=None):
        self._draw_frame()
        self.display.after(self._refresh_interval, self._repeat_draw_frame)
