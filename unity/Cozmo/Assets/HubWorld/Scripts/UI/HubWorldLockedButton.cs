using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class HubWorldLockedButton : HubWorldButton {

  [SerializeField]
  private Image _LockedChallengeImage;

  [SerializeField]
  private Sprite[] _LockedChallengeIcons;

  public override void Initialize(ChallengeData challengeData) {

    base.Initialize(challengeData);

    if (_LockedChallengeImage != null) {
      int index = Random.Range(0, _LockedChallengeIcons.Length - 1);
      _LockedChallengeImage.sprite = _LockedChallengeIcons[index];
    }
  }
}
