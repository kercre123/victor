using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo {
  namespace UI {
    // TODO : Write an editor script to replace "Element X" with the name of the currently selected event.
    // Probably just kill config entirely and replace with json driven tool like the Animator
    public class GenericRewardsConfig : ScriptableObject {
      public List<RewardedAction> RewardedActions = new List<RewardedAction>();
    }

    [System.Serializable]
    public class RewardedAction {
      public GameEvent GameEvent;
      public int Amount;
    }
  }
}