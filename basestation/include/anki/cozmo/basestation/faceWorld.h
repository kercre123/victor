/**
 * File: faceWorld.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 2014
 *
 * Description: Implements a container for mirroring on the main thread, the known faces 
 *              from the vision system (which generally runs on another thread).
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef __Anki_Cozmo_FaceWorld_H__
#define __Anki_Cozmo_FaceWorld_H__

#include "anki/vision/basestation/trackedFace.h"

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "clad/types/actionTypes.h"

#include <map>
#include <vector>

namespace Anki {
namespace Cozmo {
  
  // Forward declarations:
  class Robot;
  
  class FaceWorld
  {
  public:
    static const s32 MinTimesToSeeFace = 4;
    
    FaceWorld(Robot& robot);
    
    Result Update(const std::list<Vision::TrackedFace>& observedFaces);
    Result AddOrUpdateFace(const Vision::TrackedFace& face);
  
    Result ChangeFaceID(const Vision::UpdatedFaceID& update);
    
    // Returns nullptr if not found
    const Vision::TrackedFace* GetFace(Vision::FaceID_t faceID) const;
    
    // Returns number of known faces
    std::set<Vision::FaceID_t> GetKnownFaceIDs(bool includeTrackingOnlyFaces = true) const;
    
    // Returns known face IDs observed since seenSinceTime_ms (inclusive)
    std::set<Vision::FaceID_t> GetKnownFaceIDsObservedSince(TimeStamp_t seenSinceTime_ms,
                                                            bool includeTrackingOnlyFaces = true) const;

    // Returns true if any faces are known
    bool HasKnownFaces(TimeStamp_t seenSinceTime_ms = 0, bool includeTrackingOnlyFaces = true) const;

    // If the robot has observed a face, sets p to the pose of the last observed face and returns the
    // timestamp when that face was last seen. Otherwise, returns 0    
    TimeStamp_t GetLastObservedFace(Pose3d& p) const;

    // like GetLastObservedFace, but returns a pose with respect to the current robot pose. If they have
    // different origins (e.g. the robot was picked up and hasn't seen a face since), this code will assume
    // that the origins are the same (even though they aren't).
    TimeStamp_t GetLastObservedFaceWithRespectToRobot(Pose3d& p) const;

    // Returns true if any action has turned towards this face
    bool HasTurnedTowardsFace(Vision::FaceID_t faceID) const;

    // Tell FaceWorld that the robot has turned towards this face (or not, if val=false)
    void SetTurnedTowardsFace(Vision::FaceID_t faceID, bool val = true);
    
    // Removes all known faces and resets the last observed face timer to 0, so
    // GetLastObservedFace() will return 0.
    void ClearAllFaces();
    
    // Specify a faceID to start an enrollment of a specific ID, i.e. with the intention
    // of naming that person.
    // Use UnknownFaceID to enable (or return to) ongoing "enrollment" of session-only / unnamed faces.
    void Enroll(Vision::FaceID_t faceID);
    
    bool IsFaceEnrollmentComplete() const { return _lastEnrollmentCompleted; }
    void SetFaceEnrollmentComplete(bool complete) { _lastEnrollmentCompleted = complete; }
    
    // template for all events we subscribe to
    template<typename T>
    void HandleMessage(const T& msg);
    
  private:
    
    Robot& _robot;
    
    struct KnownFace {
      Vision::TrackedFace      face;
      VizManager::Handle_t     vizHandle;
      s32                      numTimesObserved = 0;
      s32                      numTimesObservedFacingCamera = 0;
      bool                     hasTurnedTowards = false;

      KnownFace(const Vision::TrackedFace& faceIn);
      bool IsNamed() const { return !face.GetName().empty(); }
      bool HasStableID() const;
    };
    
    using FaceContainer = std::map<Vision::FaceID_t, KnownFace>;
    using KnownFaceIter = FaceContainer::iterator;
    FaceContainer _knownFaces;
    
    Vision::FaceID_t _idCtr = 0;
    
    Pose3d      _lastObservedFacePose;
    TimeStamp_t _lastObservedFaceTimeStamp = 0;
    
    bool _lastEnrollmentCompleted = false;
    
    // Removes the face and advances the iterator. Notifies any listeners that
    // the face was removed if broadcast==true.
    void RemoveFace(KnownFaceIter& faceIter, bool broadcast = true);
    
    void RemoveFaceByID(Vision::FaceID_t faceID);

    void SetupEventHandlers(IExternalInterface& externalInterface);
    
    void DrawFace(KnownFace& knownFace);
    
    std::vector<Signal::SmartHandle> _eventHandles;
    
  }; // class FaceWorld
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_FaceWorld_H__
