using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections;
using DG.Tweening;
using Cozmo.UI;

public class ChallengeDetailsDialog : BaseView {

  public delegate void ChallengeDetailsClickedHandler(string challengeId);

  public event ChallengeDetailsClickedHandler ChallengeStarted;

  [SerializeField]
  private AnkiTextLabel _TitleTextLabel;

  [SerializeField]
  private AnkiTextLabel _DescriptionTextLabel;

  [SerializeField]
  private AnkiTextLabel _PlayersAndCubesLabel;

  [SerializeField]
  private Image _ChallengeIcon;

  [SerializeField]
  private AnkiButton _StartChallengeButton;

  [SerializeField]
  private Vector3 _CenteredIconViewportPos;

  [SerializeField]
  private RectTransform _DialogBackground;

  private string _ChallengeId;

  public void Initialize(ChallengeData challengeData, Transform challengeButtonTransform) {
    _TitleTextLabel.text = Localization.Get(challengeData.ChallengeTitleLocKey);
    _DescriptionTextLabel.text = Localization.Get(challengeData.ChallengeDescriptionLocKey);
    _PlayersAndCubesLabel.text = string.Format(Localization.GetCultureInfo(),
      Localization.Get(LocalizationKeys.kChallengeDetailsLabelPlayersAndCubesNeeded),
      challengeData.MinigameConfig.NumPlayersRequired(),
      challengeData.MinigameConfig.NumCubesRequired());
    _ChallengeIcon.sprite = challengeData.ChallengeIcon;
    _StartChallengeButton.onClick.AddListener(HandleStartButtonClicked);
    _ChallengeId = challengeData.ChallengeID;
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
    // Slide the dialog out and back
    DG.Tweening.Tweener dialogTween = _DialogBackground.DOLocalMoveX(2500, 0.5f).From().SetEase(Ease.OutBack).SetDelay(0.2f);
    openAnimation.Join(dialogTween);
  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
    // Slide the dialog out
    DG.Tweening.Tweener dialogTween = _DialogBackground.DOLocalMoveX(2500, 0.5f).SetEase(Ease.InBack).SetDelay(0.2f);
    closeAnimation.Join(dialogTween);
  }
}
