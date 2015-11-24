using UnityEngine;
using System.Collections;

public class HubWorldUnlockedButton : HubWorldButton {

  [SerializeField]
  private UnityEngine.UI.Image _ChallengeIconImage;

  public override void Initialize(ChallengeData challengeData) {

    base.Initialize(challengeData);

    if (_ChallengeIconImage != null) {
      if (challengeData.ChallengeIcon != null) {
        _ChallengeIconImage.sprite = challengeData.ChallengeIcon;
      }
      else {
        _ChallengeIconImage.gameObject.SetActive(false);
      }
    }
  }
}
