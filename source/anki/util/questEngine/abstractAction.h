/**
 * File: abstractAction.h
 *
 * Author: aubrey
 * Created: 05/06/15
 *
 * Description: Base class for actions triggered by quest engine events
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_QuestEngine_AbstractAction_H__
#define __Util_QuestEngine_AbstractAction_H__


namespace Anki {
namespace Util {
namespace QuestEngine {

class QuestEngine;
  
class AbstractAction
{
public:
  
  AbstractAction() {};
  virtual ~AbstractAction() {}
      
  virtual void PerformAction(QuestEngine& questEngine) = 0;
      
};
 
} // namespace QuestEngine
} // namespace Util
} // namespace Anki

#endif // __Util_QuestEngine_AbstractAction_H__
