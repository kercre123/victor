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

      public float Evaluate(Robot robot) {
        float result = 0f;
        for (int i = 0; i < MoodScorer.Count; i++) {

          float emotionVal = robot.EmotionValues[(int)MoodScorer[i].EmotionType];
          result += MoodScorer[i].Curve.Evaluate(emotionVal);
        }
        return result;
      }
    }

    /// <summary>
    /// A temporary list to avoid allocating a new list each time.
    /// </summary>
    private readonly List<string> _TempList = new List<string>();

    public string GetAnimation(Robot robot) {
      float max = float.MinValue;
      _TempList.Clear();

      for (int i = 0; i < Animations.Count; i++) {
        float val = Animations[i].Evaluate(robot);
        // if the two results are approximately the same
        if (Mathf.Abs(val - max) < 0.1f) {
          // add it to our result list
          _TempList.Add(Animations[i].Name);
          // set max to the average of all values in the temp list
          max = Mathf.Lerp(val, max, 1f / _TempList.Count);
        }
        // if this is the new max
        else if (val > max) {
          // clear our result list and add our single entry
          _TempList.Clear();
          _TempList.Add(Animations[i].Name);
          max = val;
        }
      }
      if (_TempList.Count > 0) {
        return _TempList[UnityEngine.Random.Range(0, _TempList.Count)];
      }

      return null;
    }
  }
}

