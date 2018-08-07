/**
 * File: IVisionModeSubscriber
 *
 * Author: Sam Russell
 * Date:   1/16/2018
 *
 * Description: Interface class for use by the visionScheduleMediator for observer-pattern style
 *              tracking of subscribers. Classes which use the VSM to subscribe to various vision
 *              modes should inherit from this class. In future, functionality may be added to 
 *              notify subscribers of optimization related decisions made by the VSM such as 
 *              subscription demotion or cancellation) in the event that the Vision system is 
 *              overloaded.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Engine_Components_iVisionModeSubscriber_H__
#define __Engine_Components_iVisionModeSubscriber_H__

namespace Anki{
namespace Vector{

class IVisionModeSubscriber
{
public:
  virtual ~IVisionModeSubscriber() {};
protected:
  // enforce class as abstract
  IVisionModeSubscriber() {};
};

} // namespace Vector
} // namespace Anki

#endif //__Engine_Components_iVisionModeSubscriber_H__