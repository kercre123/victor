using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo {
  namespace UI {
    [System.Serializable]
    public class DailyGoal {
      public GameEvent GameEvent;
      public LocalizedString Title;
      public LocalizedString Description;
      public int Progress;
      public int Target;
      public int PointsRewarded;
    }
  }
}