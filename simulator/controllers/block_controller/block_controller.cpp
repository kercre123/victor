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
  
  ActiveBlock::Init();
  
  while(ActiveBlock::Update() == Anki::RESULT_OK)
  {
  }
  
  ActiveBlock::DeInit();
  
  return 0;
}
