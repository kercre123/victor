using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Newtonsoft.Json;


[System.Serializable]
public class SerializableAnimationTrigger : SerializableEnum<Anki.Cozmo.AnimationTrigger> {
}


/// <summary>
/// Full List of Pairs for CladEvents to AnimationGroups.
/// </summary>
namespace Anki.Cozmo {
  public class AnimationEventMap {

    // Comparer to properly sort AnimationPairings
    public class AnimPairComparer : IComparer<CladAnimPair> {
      public int Compare(CladAnimPair x, CladAnimPair y) {
        if (x == null) {
          if (y == null) {
            // If both null, equals
            return 0;
          }
          else {
            // If x null and y isn't, y is greater
            return -1;
          }
        }
        else {
          // If y null, x is greater
          if (y == null) {
            return 1;
          }
          else {
            // If both aren't null, actually compare CladEvent values
            return ((int)x.CladEvent).CompareTo((int)y.CladEvent);
          }
        }
      }
    }

    public List<CladAnimPair> Pairs = new List<CladAnimPair>();

    public void NewMap() {
      Pairs.Clear();
      for (int i = 0; i < (int)AnimationTrigger.Count; i++) {
        Pairs.Add(new CladAnimPair((AnimationTrigger)i));
      }
    }

    // Updates the Map to include any new GameEvents that may have been added
    // returns true if anything new was added. TODO: Is there a faster way to do this? Do I care?
    public bool MapUpdate(out string newStuff) {
      bool didUpdate = false;
      newStuff = "";
      List<AnimationTrigger> eList = new List<AnimationTrigger>();
      // Add in all clad generated game events
      for (int i = 0; i < (int)AnimationTrigger.Count; i++) {
        eList.Add((AnimationTrigger)i);
      }
      // Remove the ones we already have from the "to add" list
      for (int i = 0; i < Pairs.Count; i++) {
        eList.Remove(Pairs[i].CladEvent);
      }
      // Add in the remaining ones
      for (int i = 0; i < eList.Count; i++) {
        newStuff += string.Format("- {0} -", eList[i]);
        Pairs.Add(new CladAnimPair(eList[i]));
        didUpdate = true;
      }
      if (didUpdate) {
        Pairs.Sort(new AnimPairComparer());
      }
      return didUpdate;
    }

    [System.Serializable]
    public class CladAnimPair {
      public CladAnimPair(AnimationTrigger cEvent, string animName = "") {
        CladEvent = cEvent;
        AnimName = animName;
      }

      public AnimationTrigger CladEvent;
      public string AnimName;
    }
  }
}

