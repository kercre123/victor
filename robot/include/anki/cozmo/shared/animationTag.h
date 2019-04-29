#ifndef __Anki_Cozmo_AnimationTag_H__
#define __Anki_Cozmo_AnimationTag_H__

#include <stdint.h>

namespace Anki {
namespace Vector {
  
using AnimationTag = uint8_t;

static const AnimationTag kNotAnimatingTag  = 0;

enum
{
  kAnimationTag_NotAnimating = 0,
  kAnimationTag_RecognizerResponse,
  kAnimationTag_FirstValidIndex
};

}
}

#endif /* __Anki_Cozmo_AnimationTag_H__ */
