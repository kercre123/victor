using UnityEngine;
using System.Collections;
using Newtonsoft.Json;
using System.Collections.Generic;

namespace Anki.Cozmo {
  [JsonConverter(typeof(BehaviorConverter))]
  public class Behavior {

    [JsonProperty("behaviorType")]
    public BehaviorClass BehaviorClass;
    public ReactionTrigger ReactionaryBehaviorType;

    // Any extra parameters that are valid for specific behaviors
    [JsonIgnore]
    public Dictionary<string, object> CustomParams = new Dictionary<string, object>();

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
