using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections;
using DG.Tweening;

public class ChallengeDetailsDialog : BaseView {

  public delegate void ChallengeDetailsClickedHandler(string challengeId);

  public event ChallengeDetailsClickedHandler ChallengeStarted;

  [SerializeField]
  private AnkiTextLabel _TitleTextLabel;

  [SerializeField]
  private Button _StartChallengeButton;

  [SerializeField]
  private Vector3 _CenteredIconViewportPos;

  [SerializeField]
  private RectTransform _DialogBackground;

  private string _ChallengeId;
  private Transform _ChallengeButtonTransform;

  public void Initialize(ChallengeData challengeData, Transform challengeButtonTransform) {
    _TitleTextLabel.text = Localization.Get(challengeData.ChallengeTitleLocKey);
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

    // Play camera animation
    DG.Tweening.Tweener cameraTween = HubWorldCamera.Instance.CenterCameraOnTarget(
                                        _ChallengeButtonTransform.position, 
                                        _CenteredIconViewportPos);
    openAnimation.Append(cameraTween);

    // TODO: Slide the dialog out and back
    DG.Tweening.Tweener dialogTween = _DialogBackground.DOLocalMoveX(1600, 0.5f).From().SetEase(Ease.OutBack).SetDelay(0.2f);
    openAnimation.Join(dialogTween);
  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
    // Reset the camera
    DG.Tweening.Tweener cameraTween = HubWorldCamera.Instance.ReturnCameraToDefault();
    closeAnimation.Append(cameraTween);

    DG.Tweening.Tweener dialogTween = _DialogBackground.DOLocalMoveX(1600, 0.5f).SetEase(Ease.InBack).SetDelay(0.2f);
    closeAnimation.Join(dialogTween);
  }
}
