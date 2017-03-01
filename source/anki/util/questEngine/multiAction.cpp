/**
 * File: multiAction.cpp
 *
 * Author: aubrey
 * Created: 06/23/15
 *
 * Description: Action that wraps multiple actions together
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/questEngine/multiAction.h"
#include "util/questEngine/questNotice.h"
#include "util/questEngine/questEngine.h"

namespace Anki {
namespace Util {
namespace QuestEngine {
      
MultiAction::MultiAction(const std::vector<AbstractAction*>& actions)
: _actions(actions)
{
}

MultiAction::~MultiAction()
{
  for (auto* it : _actions) {
    delete it;
  }
}
  
void MultiAction::PerformAction(QuestEngine& questEngine)
{
  for (AbstractAction* action : _actions) {
    action->PerformAction(questEngine);
  }
}
      
} // namespace StatEngine
} // namespace Util
} // namespace Anki
