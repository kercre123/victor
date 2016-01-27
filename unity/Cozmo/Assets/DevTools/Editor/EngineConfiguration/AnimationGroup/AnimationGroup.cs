using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Newtonsoft.Json;

namespace AnimationGroups {
  public class AnimationGroup {

    public List<AnimationGroupEntry> Animations = new List<AnimationGroupEntry>();

    [System.Serializable]
    public class AnimationGroupEntry {

      public string Name;

      public List<EmotionScorer> MoodScorer = new List<EmotionScorer>();
    }
  }
}

