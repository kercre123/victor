using System;
using UnityEngine;
using Newtonsoft.Json;
using Anki.Cozmo;


public class EmotionScorer {

  [JsonProperty("emotionType")]
  public EmotionType EmotionType;

  [JsonProperty("scoreGraph")]
  public GraphEvaluator2d GraphEvaluator;

  [JsonProperty("trackDelta")]
  public bool TrackDelta;

  public EmotionScorer() {
  }
}

