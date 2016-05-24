/**
 * File: abstractCondition.h
 *
 * Author: aubrey
 * Created: 05/08/15
 *
 * Description: Base class for conditions triggered by quest engine events
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_QuestEngine_AbstractCondition_H__
#define __Util_QuestEngine_AbstractCondition_H__

#include <ctime>


namespace Anki {
namespace Util {
namespace QuestEngine {

class QuestEngine;

class AbstractCondition
{
public:
  
  explicit AbstractCondition();
  virtual ~AbstractCondition() {}
  
  virtual bool IsSatisfied(QuestEngine& questEngine, std::tm& eventTime) const = 0;

};
  
} // namespace QuestEngine
} // namespace Util
} // namespace Anki

#endif // __Util_QuestEngine_AbstractCondition_H__
