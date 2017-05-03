using UnityEngine;
using System.Collections;

/// <summary>
/// This class is for prefab or asset data that is only needed once the challenge starts.
/// </summary>
public class ChallengePrefabData : ScriptableObject {

  // the mini game prefab to load for this challenge
  [SerializeField]
  private GameObject _ChallengePrefab;

  public GameObject ChallengePrefab {
    get { return _ChallengePrefab; }
  }

  // icon to show to represent this challenge
  [SerializeField]
  private Sprite _ChallengeIconPlainStyle;

  public Sprite ChallengeIconPlainStyle {
    get {
      return _ChallengeIconPlainStyle;
    }
  }
}
