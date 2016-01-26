/**
 * File: navMemoryMap.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Map of the space navigated by the robot with some memory features (like decay = forget).
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "navMemoryMap.h"

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMemoryMap::NavMemoryMap()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::Draw() const
{
  _navMesh.Draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::AddClearQuad(const Quad2f& quad)
{
  _navMesh.AddClearQuad(quad);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::AddObstacleQuad(const Quad2f& quad, NavMemoryMapTypes::EObstacleType obstacleType)
{
  _navMesh.AddObstacle(quad, obstacleType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::AddCliffQuad(const Quad2f& quad)
{
  _navMesh.AddCliff(quad);
}

} // namespace Cozmo
} // namespace Anki
