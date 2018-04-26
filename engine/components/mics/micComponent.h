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


namespace Anki {
namespace Cozmo {

class MicDirectionHistory;
class VoiceMessageSystem;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MicComponent : public IDependencyManagedComponent<RobotComponentID>
{
public:

  MicComponent();
  ~MicComponent();

  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent functions

  virtual void InitDependent( Cozmo::Robot* robot, const RobotCompMap& dependentComponents ) override;
  virtual void GetInitDependencies( RobotCompIDSet& dependencies ) const override;
  virtual void GetUpdateDependencies( RobotCompIDSet& dependencies ) const override {};


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Component API

  MicDirectionHistory& GetMicDirectionHistory() { return *_micHistory; }
  const MicDirectionHistory& GetMicDirectionHistory() const { return *_micHistory; }

  VoiceMessageSystem& GetVoiceMessageSystem() { return *_messageSystem; }
  const VoiceMessageSystem& GetVoiceMessageSystem() const { return *_messageSystem; }


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Data

  MicDirectionHistory*      _micHistory;
  VoiceMessageSystem*       _messageSystem;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Components_MicComponent_H_
