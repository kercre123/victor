using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Newtonsoft.Json;

/// <summary>
/// Full List of Pairs for CladEvents to AnimationGroups.
/// </summary>
namespace Anki.Cozmo {
  public class CladEventToAnimGroupMap {

    public List<CladAnimPair> Pairs = new List<CladAnimPair>();

    [System.Serializable]
    public class CladAnimPair {
      public CladGameEvent CladEvent;
      public string AnimName;
    }
  }
}

