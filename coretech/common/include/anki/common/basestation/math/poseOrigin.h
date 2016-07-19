/**
 * File: poseOrigin.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 7/12/2016
 *
 *
 * Description: Mostly a placeholder for defining a PoseOrigin class if we want later.
 *              For now it just aliases a const Pose3d* in a common include file.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Coretech_Common_PoseOrigin_H__
#define __Anki_Coretech_Common_PoseOrigin_H__

namespace Anki
{
  
  // Forward decl.
  class Pose3d;
  
  using PoseOrigin = const Pose3d*;
  
}

#endif // __Anki_Coretech_Common_PoseOrigin_H__
