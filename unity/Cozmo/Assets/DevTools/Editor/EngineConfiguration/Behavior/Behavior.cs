using UnityEngine;
using System.Collections;
using Newtonsoft.Json;
using System.Collections.Generic;

namespace Anki.Cozmo {
  public class Behavior {

    [JsonProperty("behaviorType")]
    public BehaviorType BehaviorType;

    // Only valid for PlayAnim behaviors
    [JsonProperty("animName")]
    public string AnimName;

    [JsonProperty("name")]
    public string Name;

    [JsonProperty("emotionScorers")]
    public List<EmotionScorer> EmotionScorers = new List<EmotionScorer>();

    [JsonProperty("repetitionPenalty")]
    public GraphEvaluator2d RepetitionPenalty = new GraphEvaluator2d();

    [JsonProperty("behaviorGroups")]
    public List<BehaviorGroup> BehaviorGroups = new List<BehaviorGroup>();
  }
}
