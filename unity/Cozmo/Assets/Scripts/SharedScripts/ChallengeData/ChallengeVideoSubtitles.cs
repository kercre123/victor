using UnityEngine;
using System.Collections.Generic;

public class ChallengeVideoSubtitles : ScriptableObject {
  [SerializeField]
  public List<VideoLocalizationPair> VideoSubtitles = new List<VideoLocalizationPair>();
}
