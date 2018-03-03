#pragma once

#include "cozmoAnim/doom/m_fixed.h"


// Bounding box coordinate storage.
enum
{
    BOXTOP,
    BOXBOTTOM,
    BOXLEFT,
    BOXRIGHT
};	// bbox coordinates

// Bounding box functions.
void M_ClearBox (int*	box);

void
M_AddToBox
( int*	box,
  int	x,
  int	y );
