using System;
using UnityEngine;
using Newtonsoft.Json;
using Anki.Cozmo;


public class EmotionScorer {

  [JsonProperty("emotionType")]
  public EmotionType EmotionType;

  [JsonProperty("scoreGraph")]
  public GraphEvaluator2d GraphEvaluator { get { return Curve; } set { Curve = value; } }

  [JsonProperty("trackDelta")]
  public bool TrackDelta;

  // Use an animation curve when editing in unity since Unity gives us a nice graph editor for free
  [JsonIgnore]
  public AnimationCurve Curve = new AnimationCurve(new Keyframe(-1, -1, 1, 1), new Keyframe(1,1, 1, 1));

  public EmotionScorer() {
  }
}

