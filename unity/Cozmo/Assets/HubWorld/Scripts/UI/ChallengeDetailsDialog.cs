using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections;

public class ChallengeDetailsDialog : BaseView {

  public delegate void ChallengeDetailsClickedHandler(string challengeId);

  public event ChallengeDetailsClickedHandler ChallengeStarted;

  [SerializeField]
  public AnkiTextLabel _TitleTextLabel;

  [SerializeField]
  public Button _StartChallengeButton;

  private string _ChallengeId;
  private Transform _ChallengeButtonTransform;

  public void Initialize(ChallengeData challengeData, Transform challengeButtonTransform) {
    _TitleTextLabel.text = Localization.Get(challengeData.ChallengeTitleKey);
    _StartChallengeButton.onClick.AddListener(HandleStartButtonClicked);
    _ChallengeButtonTransform = challengeButtonTransform;
    _ChallengeId = challengeData.ChallengeID;

    // TODO: Have a horizontal scroll rect. 
  }

  private void HandleStartButtonClicked() {
    if (ChallengeStarted != null) {
      ChallengeStarted(_ChallengeId);
    }
  }

  protected override void CleanUp() {
    _StartChallengeButton.onClick.RemoveAllListeners();
  }

  protected override void ConstructOpenAnimation(DG.Tweening.Sequence openAnimation) {
    // TODO: Play camera animation
    // TODO: Slide the dialog out and back
  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
    // TODO Reset the camera
  }
}
