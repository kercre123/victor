#include "anki/common/basestation/strongTypedef.h"

namespace Anki {

  ObjectType::StorageType ObjectType::UniqueTypeCounter = 0;
  //std::set<int> ObjectType::ValidTypes;
  
  std::set<int>& ObjectType::GetValidTypes()
  {
    static std::set<int> ValidTypes;
    return ValidTypes;
  }
  
  ObjectID::StorageType ObjectID::UniqueIDCounter = 0;
  
} // namespace Anki