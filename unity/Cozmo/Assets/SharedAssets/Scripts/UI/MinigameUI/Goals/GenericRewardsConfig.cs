using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo {
  namespace UI {
    // TODO : kill config entirely and replace with json driven tool like the Animator
    public class GenericRewardsConfig : ScriptableObject {
      public List<RewardedAction> RewardedActions = new List<RewardedAction>();
    }

    [System.Serializable]
    public class RewardedAction {
      public SerializableGameEvents GameEvent;
      public int Amount;
    }
  }
}