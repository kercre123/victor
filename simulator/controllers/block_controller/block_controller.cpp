/*
 * File:          block_controller.cpp
 * Date:
 * Description:   Webots controller for active block
 * Author:        
 * Modifications: 
 */

#include <stdio.h>
#include "activeBlock.h"
#include "anki/common/types.h"

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  using namespace Anki::Cozmo;
  
  if (ActiveBlock::Init() == Anki::RESULT_FAIL) {
    printf("ERROR (block_controller): Failed to init block");
    return -1;
  }
  
  while(ActiveBlock::Update() == Anki::RESULT_OK)
  {
  }
  
  ActiveBlock::DeInit();
  
  return 0;
}
