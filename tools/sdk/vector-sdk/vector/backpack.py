'''
Backpack related classes, functions, events and values.

Copyright(c) 2018 Anki, Inc.
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ["BackpackComponent"]


from . import lights, sync, util
from .messaging import protocol


class BackpackComponent(util.Component):
    '''Manage Vector's Backpack lights'''
    @sync.Synchronizer.wrap
    async def set_backpack_lights(self, light1, light2, light3, backpack_color_profile=lights.WHITE_BALANCED_BACKPACK_PROFILE):
        '''Set the lights on Vector's backpack.

        The light descriptions below are all from Vector's perspective.

        Args:
            light1 (:class:`vector.lights.Light`): The front backpack light
            light2 (:class:`vector.lights.Light`): The center backpack light
            light3 (:class:`vector.lights.Light`): The rear backpack light
        '''
        params = lights.package_request_params((light1, light2, light3), backpack_color_profile)
        set_backpack_lights_request = protocol.SetBackpackLightsRequest(**params)

        return await self.interface.SetBackpackLights(set_backpack_lights_request)

    def set_all_backpack_lights(self, light, color_profile=lights.WHITE_BALANCED_BACKPACK_PROFILE):
        '''Set the lights on Vector's backpack to the same color.

        Args:
            light (:class:`vector.lights.Light`): The lights for Vector's backpack.
        '''
        light_arr = [light] * 3
        return self.set_backpack_lights(*light_arr, color_profile)
