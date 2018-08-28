# Copyright (c) 2018 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''
Animation related classes, functions, events and values.
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ["AnimationComponent"]

import asyncio

from . import exceptions, sync, util
from .messaging import protocol


class AnimationComponent(util.Component):
    '''Manage the state of all the animations in the robot'''

    def __init__(self, robot):
        super().__init__(robot)
        self._anim_dict = {}

    @property
    def anim_list(self):
        if not self._anim_dict:
            self.logger.warning("Anim list was empty. Lazy-loading anim list now.")
            result = self.load_animation_list()
            if isinstance(result, sync.Synchronizer):
                result.wait_for_completed()
        return list(self._anim_dict.keys())

    async def ensure_loaded(self):
        '''
        This is an optimization for the case where a user doesn't
        need the animation_list. That way connections aren't delayed
        by the load_animation_list call.

        If this is invoked inside another async function then we
        explicitly await the result.
        '''
        if not self._anim_dict:
            self.logger.warning("Anim list was empty. Lazy-loading anim list now.")
            result = self.load_animation_list()
            if asyncio.iscoroutine(result):
                await result

    @sync.Synchronizer.wrap
    @sync.Synchronizer.disable_log
    async def load_animation_list(self):
        req = protocol.ListAnimationsRequest()
        result = await self.interface.ListAnimations(req)
        self.logger.debug(f"status: {result.status}, number_of_animations:{len(result.animation_names)}")
        self._anim_dict = {a.name: a for a in result.animation_names}
        return result

    @sync.Synchronizer.wrap
    async def play_animation(self, anim, loop_count=1, ignore_body_track=True, ignore_head_track=True, ignore_lift_track=True):
        '''Starts an animation playing on a robot.

        Vector must be off of the charger to play an animation.

        Warning: Specific animations may be renamed or removed in future updates of the app.
            If you want your program to work more reliably across all versions
            we recommend using :meth:`play_animation_trigger` instead. TODO: implement play_animation_trigger

        Args:
            anim (str or anki_vector.protocol.Animation): The animation to play.
            loop_count (int): Number of times to play the animation.
        '''
        animation = anim
        if not isinstance(anim, protocol.Animation):
            await self.ensure_loaded()
            if anim not in self.anim_list:
                raise exceptions.VectorException(f"Unknown animation: {anim}")
            animation = self._anim_dict[anim]
        req = protocol.PlayAnimationRequest(animation=animation,
                                            loops=loop_count,
                                            ignore_body_track=ignore_body_track,
                                            ignore_head_track=ignore_head_track,
                                            ignore_lift_track=ignore_lift_track)
        return await self.interface.PlayAnimation(req)
