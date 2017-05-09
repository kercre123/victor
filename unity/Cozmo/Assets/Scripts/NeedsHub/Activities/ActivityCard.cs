using Cozmo.Challenge;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Needs.Activities.UI {
  public class ActivityCard : MonoBehaviour {

    public delegate void ActivityButtonPressedHandler(string challengeId);
    public event ActivityButtonPressedHandler OnActivityButtonPressed;

    [SerializeField]
    private CozmoButton _StartActivityButton;

    [SerializeField]
    private CozmoText _ActivityTitleLabel;

    [SerializeField]
    private CozmoText _ActivityDescriptionLabel;

    [SerializeField]
    private CozmoImage _ActivityIcon;

    [SerializeField]
    private CozmoImage _ActivityTintedFrame;

    private string _ChallengeId;

    public void Initialize(ChallengeData activityData, string parentDialogName) {
      _ChallengeId = activityData.ChallengeID;

      _StartActivityButton.Initialize(HandleStartActivityButtonPressed, "large_start_activity_button", parentDialogName);

      _ActivityTitleLabel.text = Localization.Get(activityData.ChallengeTitleLocKey);
      _ActivityDescriptionLabel.text = Localization.Get(activityData.ChallengeDescriptionLocKey);

      _ActivityTitleLabel.color = activityData.ActivityData.ActivityTitleColor;
      _ActivityTintedFrame.color = activityData.ActivityData.ActivityFrameColor;

      _ActivityIcon.sprite = activityData.ActivityData.ActivityIcon;
    }

    private void HandleStartActivityButtonPressed() {
      if (OnActivityButtonPressed != null) {
        OnActivityButtonPressed(_ChallengeId);
      }
    }
  }
}