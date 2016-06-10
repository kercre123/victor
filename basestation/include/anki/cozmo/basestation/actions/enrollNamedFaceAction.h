/**
 * File: enrollNamedFaceAction.h
 *
 * Author: Andrew Stein
 * Date:   4/24/2016
 *
 * Description: Defines an action for going through the steps to enroll a new, named face.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Cozmo_Actions_EnrollNamedFaceAction_H__
#define __Anki_Cozmo_Actions_EnrollNamedFaceAction_H__

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/vision/basestation/faceIdTypes.h"

#include "clad/types/actionTypes.h"
#include "clad/types/faceEnrollmentPoses.h"
#include "clad/types/faceEnrollmentSequences.h"

#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

  class EnrollNamedFaceAction : public IAction
  {
  public:
    EnrollNamedFaceAction(Robot& robot, Vision::FaceID_t faceID, const std::string& name);
    
    virtual ~EnrollNamedFaceAction();
    
    virtual const std::string& GetName() const override { return _actionName; }
    virtual RobotActionType GetType() const override { return RobotActionType::ENROLL_NAMED_FACE; }
        
    virtual u8 GetTracksToLock() const override {
      return (u8)AnimTrackFlag::NO_TRACKS;
    }
    
    virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
    
    // Specify which enrollment sequence to use
    void SetSequenceType(FaceEnrollmentSequence whichSeq) { _whichSeq = whichSeq; }
    
    // Set to true to save the enrolled face to the robot's NVStorage when done (default is true)
    void EnableSaveToRobot(bool enable) { _saveToRobot = enable; }
    
    void SetSayNameWhenDone(bool tf) { _sayNameWhenDone = tf; }
    
    //void SetIdleAnimation(const std::string& name) { _idleAnimName = name; }
    
    template<typename T>
    void HandleMessage(const T& msg);
    
  protected:
    
    virtual ActionResult Init() override;
    virtual ActionResult CheckIfDone() override;

  private:
    
    enum class State : u8 {
      PreActing,
      Enrolling,
      PostActing,
      Finishing
    };
    
    State _state;
    FaceEnrollmentSequence _whichSeq = FaceEnrollmentSequence::Default;

    // Member variables:
    std::string               _actionName = "EnrollNamedFace";
    Vision::FaceID_t          _faceID = Vision::UnknownFaceID;
    std::string               _faceName;
    IActionRunner*            _action = nullptr;
    TimeStamp_t               _lastModeChangeTime_ms;
    TimeStamp_t               _minTimePerEnrollStep_ms = 1000;
    TimeStamp_t               _lastFaceSeen_ms = 0;
    s32                       _numEnrollmentsRequired = 0;
    bool                      _enrollmentCountReached = false;
    bool                      _saveToRobot = true;
    bool                      _idlePushed = false;
    bool                      _sayNameWhenDone = true;
    
    struct EnrollStep {
      Vision::FaceEnrollmentPose pose;
      s32 numEnrollments;
      std::function<Result(void)> startFcn, duringFcn, stopFcn;
    };
    
    std::vector<EnrollStep> _enrollSequence;
    std::vector<EnrollStep>::const_iterator _seqIter;
    
    std::vector<Signal::SmartHandle> _signalHandles;
    
    // Member methods:
    Result InitSequence();
    Result InitCurrentStep();
    void SetAction(IActionRunner* action);
    
  }; // class SayTextAction


} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Actions_EnrollNamedFaceAction_H__ */
