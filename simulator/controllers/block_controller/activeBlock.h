/*
 * File:          activeBlock.h
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#ifndef ACTIVE_BLOCK_H
#define ACTIVE_BLOCK_H

#include "anki/common/types.h"

namespace Anki {
  namespace Cozmo {
    namespace ActiveBlock {
  
      Result Init();
      void DeInit();
      
      Result Update();
  
    }  // namespace ActiveBlock
  }  // namespace Cozmo
}  // namespace Anki


#endif // ACTIVE_BLOCK_H