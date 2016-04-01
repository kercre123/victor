using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Newtonsoft.Json;

/// <summary>
/// Full List of Pairs for CladEvents to AnimationGroups.
/// </summary>
namespace Anki.Cozmo {
  public class AnimationEventMap {
    // TODO: Make it default initialize pairs for all CladEvents

    public List<CladAnimPair> Pairs = new List<CladAnimPair>();

    public void NewMap() {
      Pairs.Clear();
      for (int i = 0; i < (int)CladGameEvent.COUNT; i++) {
        Pairs.Add(new CladAnimPair((CladGameEvent)i));
      }
    }

    [System.Serializable]
    public class CladAnimPair {
      public CladAnimPair() {
        CladEvent = CladGameEvent.COUNT;
        AnimName = string.Empty;
      }

      public CladAnimPair(CladGameEvent cEvent, string animName = "") {
        CladEvent = cEvent;
        AnimName = animName;
      }

      public CladGameEvent CladEvent;
      public string AnimName;
    }
  }
}

