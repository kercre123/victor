#ifndef __ANIMATION_H
#define __ANIMATION_H

void animation_init(void);
void animation_tick(void);
void animation_frames(int channel, const KeyFrame* frames);
void animation_index(const FrameMap* map);

#endif
