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
  private AnkiTextLabel _TitleTextLabelBack;

  [SerializeField]
  private AnkiTextLabel _DescriptionTextLabel;

  [SerializeField]
  private AnkiTextLabel _PlayersAndCubesLabel;

  [SerializeField]
  private IconProxy _ChallengeIcon;

  [SerializeField]
  private AnkiButton _StartChallengeButton;

  [SerializeField]
  private Vector3 _CenteredIconViewportPos;

  [SerializeField]
  private RectTransform _DialogBackground;

  private string _ChallengeId;

  public void Initialize(ChallengeData challengeData, Transform challengeButtonTransform) {
    _TitleTextLabel.text = Localization.Get(challengeData.ChallengeTitleLocKey);
    _TitleTextLabelBack.text = Localization.Get(challengeData.ChallengeTitleLocKey);
    _DescriptionTextLabel.text = Localization.Get(challengeData.ChallengeDescriptionLocKey);

    var locKey = LocalizationKeys.kChallengeDetailsLabelPlayersAndCubesNeeded;
    int players = challengeData.MinigameConfig.NumPlayersRequired();
    int cubes = challengeData.MinigameConfig.NumCubesRequired();
    if (players == 1 && cubes == 1) {
      locKey = LocalizationKeys.kChallengeDetailsLabelPlayerAndCubeNeeded;
    }
    else if (players == 1 && cubes != 1) {
      locKey = LocalizationKeys.kChallengeDetailsLabelPlayerAndCubesNeeded;
    }
    else if (players != 1 && cubes == 1) {
      locKey = LocalizationKeys.kChallengeDetailsLabelPlayersAndCubeNeeded;
    }
      
    _PlayersAndCubesLabel.text = Localization.GetWithArgs(locKey, players, cubes);
    _ChallengeIcon.SetIcon(challengeData.ChallengeIcon);
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
    DG.Tweening.Tweener dialogTween = _DialogBackground.DOLocalMoveX(2500, 0.5f).From().SetEase(Ease.Linear).SetDelay(0.2f);
    openAnimation.Join(dialogTween);
  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
    // Slide the dialog out
    DG.Tweening.Tweener dialogTween = _DialogBackground.DOLocalMoveX(2500, 0.5f).SetEase(Ease.Linear);
    closeAnimation.Join(dialogTween);
  }
}
