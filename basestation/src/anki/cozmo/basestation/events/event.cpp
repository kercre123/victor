#include "event.h"

namespace Anki {
namespace Cozmo {

  
Result Event::AddData(std::string name, EventDataType type, const void * valuePointer)
{
  switch (type) {
    case EventDataType::BOOLEAN:
    {
      _eventData[name].boolean = *(static_cast<const bool*>(valuePointer));
      break;
    }
    case EventDataType::U_8:
    {
      _eventData[name].u8 = *(static_cast<const u8*>(valuePointer));
      break;
    }
    case EventDataType::S_8:
    {
      _eventData[name].s8 = *(static_cast<const s8*>(valuePointer));
      break;
    }
    case EventDataType::U_16:
    {
      _eventData[name].u16 = *(static_cast<const u16*>(valuePointer));
      break;
    }
    case EventDataType::S_16:
    {
      _eventData[name].s16 = *(static_cast<const s16*>(valuePointer));
      break;
    }
    case EventDataType::U_32:
    {
      _eventData[name].u32 = *(static_cast<const u32*>(valuePointer));
      break;
    }
    case EventDataType::S_32:
    {
      _eventData[name].s32 = *(static_cast<const s32*>(valuePointer));
      break;
    }
    case EventDataType::U_64:
    {
      _eventData[name].u64 = *(static_cast<const u64*>(valuePointer));
      break;
    }
    case EventDataType::S_64:
    {
      _eventData[name].s64 = *(static_cast<const s64*>(valuePointer));
      break;
    }
    case EventDataType::F_32:
    {
      _eventData[name].f32 = *(static_cast<const f32*>(valuePointer));
      break;
    }
    case EventDataType::F_64:
    {
      _eventData[name].f64 = *(static_cast<const f64*>(valuePointer));
      break;
    }
    default:
    {
      assert(false);
      return RESULT_FAIL;
    }
  }
  
  _eventData[name].type = type;
  
  return RESULT_OK;
}


} // namespace Cozmo
} // namespace Anki