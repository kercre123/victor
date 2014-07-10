/**
 * File: dockableObject.h
 *
 * Author: Andrew Stein
 * Date:   7/9/2014
 *
 * Description: Defines a "Dockable" Object, which is a subclass of an 
 *              ObservableObject that can also be docked with.
 *              It extends the (coretech) ObservableObject to have a notion of
 *              docking for Cozmo's purposes, by offering, for example, the 
 *              ability to request pre-dock poses.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_BASESTATION_DOCKABLE_OBJECT_H
#define ANKI_COZMO_BASESTATION_DOCKABLE_OBJECT_H

#include "anki/vision/basestation/observableObject.h"

namespace Anki {
  namespace Cozmo {
    
    class DockableObject : public Vision::ObservableObject
    {
    public:
      
      DockableObject(ObjectType_t type);
      
      // Get possible poses to start docking/tracking procedure. These will be
      // a point a given distance away from each vertical face that has the
      // specified code, in the direction orthogonal to that face.  The points
      // will be w.r.t. same parent as the object, with the Z coordinate at the
      // height of the center of the object. The poses will be paired with
      // references to the corresponding marker. Optionally, only poses/markers
      // with the specified code can be returned.
      using PoseMarkerPair_t = std::pair<Pose3d,const Vision::KnownMarker&>;
      virtual void GetPreDockPoses(const float distance_mm,
                                   std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                                   const Vision::Marker::Code withCode = Vision::Marker::ANY_CODE) const = 0;
      
      // Keep track of whether this object (has been docked with and) is being
      // carried
      bool IsBeingCarried() const;
      void SetIsBeingCarried(const bool tf);

    protected:
      
      bool _isBeingCarried;
      
      bool GetPreDockPose(const Point3f& canonicalPoint,
                          const float distance_mm,
                          Pose3d& preDockPose) const;
      
    }; // class DockableObject
    
    
    
    inline bool DockableObject::IsBeingCarried() const {
      return _isBeingCarried;
    }
    
    inline void DockableObject::SetIsBeingCarried(const bool tf) {
      _isBeingCarried = tf;
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_DOCKABLE_OBJECT_H