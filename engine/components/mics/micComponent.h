/***********************************************************************************************************************
 *
 *  MicComponent
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 4/4/2018
 *
 *  Description
 *  + Component to access mic data and to interface with playback and recording
 *
 **********************************************************************************************************************/

#ifndef __Engine_Components_MicComponent_H_
#define __Engine_Components_MicComponent_H_

#include "engine/robotComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "clad/cloud/mic.h"


namespace Anki {
namespace Cozmo {

class MicDirectionHistory;
class VoiceMessageSystem;
class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MicComponent : public IDependencyManagedComponent<RobotComponentID>
{
public:

  MicComponent();
  ~MicComponent();

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent functions

  virtual void InitDependent( Cozmo::Robot* robot, const RobotCompMap& dependentComps ) override;
  virtual void GetInitDependencies( RobotCompIDSet& dependencies ) const override;
  virtual void GetUpdateDependencies( RobotCompIDSet& dependencies ) const override {};


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Component API

  MicDirectionHistory& GetMicDirectionHistory() { return *_micHistory; }
  const MicDirectionHistory& GetMicDirectionHistory() const { return *_micHistory; }

  VoiceMessageSystem& GetVoiceMessageSystem() { return *_messageSystem; }
  const VoiceMessageSystem& GetVoiceMessageSystem() const { return *_messageSystem; }

  void StartWakeWordlessStreaming( CloudMic::StreamType streamType );
  
  void SetShouldStreamAfterWakeWord( bool shouldStream );
  
  void SetTriggerWordDetectionEnabled( bool enabled );
  
  // getters for the above, based on local info, not from the anim process
  bool GetShouldStreamAfterWakeWord() const { return _streamAfterWakeWord; }
  bool GetTriggerWordDetectionEnabled() const { return _triggerDetectionEnabled; }

  // set / get the fullness of the audio processing buffer on the robot (float from 0 to 1)
  void  SetBufferFullness(float val);
  float GetBufferFullness() const { return _fullness; }

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Data
  
  bool _streamAfterWakeWord = true; // default based on micDataProcessor.h
  bool _triggerDetectionEnabled = true; // default based on micDataProcessor.h

  MicDirectionHistory*      _micHistory;
  VoiceMessageSystem*       _messageSystem;
  Robot*                    _robot;
  float                     _fullness;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Components_MicComponent_H_
