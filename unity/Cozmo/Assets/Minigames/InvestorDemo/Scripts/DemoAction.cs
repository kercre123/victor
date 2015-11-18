using UnityEngine;
using System.Collections;

namespace InvestorDemo {
  public class DemoAction {
    public DemoAction(string animationName, Anki.Cozmo.BehaviorType behavior) {
      AnimationName = animationName;
      Behavior = behavior;
    }

    public string AnimationName { get; set; }

    public Anki.Cozmo.BehaviorType Behavior { get; set; }

  }
}