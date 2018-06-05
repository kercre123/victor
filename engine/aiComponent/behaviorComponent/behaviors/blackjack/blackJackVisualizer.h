/**
 * File: BlackJackVisualizer.h
 *
 * Author: Sam Russell
 * Created: 2018-05-09
 *
 * Description: Class to encapsulate the View functionality of the BlackJack behavior. Displays
 *              appropriate card art and animations to Victor's display during BlackJack.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackVisualizer__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackVisualizer__

namespace Anki{
namespace Cozmo{

class BlackJackVisualizer{
public:
  BlackJackVisualizer();

  void VisualizeDealing();
  void VisualizeSpread();
  void VisualizeFlop();
  void VisualizeClear();

  void UpdateVisualization();

private:

};

} //namespace Cozmo
} //namespace Anki

#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_BlackJackVisualizer__