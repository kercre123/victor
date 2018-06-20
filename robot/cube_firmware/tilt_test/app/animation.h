#ifndef __ANIMATION_H
#define __ANIMATION_H

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void animation_init(void);
void animation_tick(void);
void animation_frames(const FrameCommand* frames);
void animation_index(const MapCommand* map);

#ifdef __cplusplus
}
#endif
  
#endif
