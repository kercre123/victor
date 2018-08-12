'''
Photo related classes, functions, events and values.

Copyright(c) 2018 Anki, Inc.
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ["PhotographComponent"]


from . import sync, util
from .messaging import protocol


class PhotographComponent(util.Component):
    '''Manage the state of all the photos on the robot'''

    def __init__(self, robot):
        super().__init__(robot)
        self._photo_info = []

    @property
    def photo_info(self):
        if not self._photo_info:
            self.logger.debug("Photo list was empty. Lazy-loading photo list now.")
            result = self.load_photo_info()
            if isinstance(result, sync.Synchronizer):
                result.wait_for_completed()
        return self._photo_info

    @sync.Synchronizer.wrap
    async def load_photo_info(self):
        req = protocol.PhotosInfoRequest()
        result = await self.interface.PhotosInfo(req)
        self._photo_info = result.photo_infos
        return result

    @sync.Synchronizer.wrap
    @sync.Synchronizer.disable_log
    async def get_photo(self, photo_id):
        req = protocol.PhotoRequest(photo_id=photo_id)
        return await self.interface.Photo(req)

    @sync.Synchronizer.wrap
    @sync.Synchronizer.disable_log
    async def get_thumbnail(self, photo_id):
        req = protocol.ThumbnailRequest(photo_id=photo_id)
        return await self.interface.Thumbnail(req)
