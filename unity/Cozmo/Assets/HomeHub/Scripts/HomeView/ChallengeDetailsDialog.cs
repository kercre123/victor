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
  private IconProxy _ChallengeIcon;

  [SerializeField]
  private Cozmo.UI.CozmoButton _StartChallengeButton;

  [SerializeField]
  private Vector3 _CenteredIconViewportPos;

  private string _ChallengeId;

  [SerializeField]
  private CanvasGroup _AlphaController;

  public void Initialize(ChallengeData challengeData, Transform challengeButtonTransform) {
    _TitleTextLabel.text = Localization.Get(challengeData.ChallengeTitleLocKey);
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

    if (_PlayersAndCubesLabel != null) {
      _PlayersAndCubesLabel.text = Localization.GetWithArgs(locKey, players, cubes);
    }
    _ChallengeIcon.SetIcon(challengeData.ChallengeIcon);
    _StartChallengeButton.Initialize(HandleStartButtonClicked, string.Format("{0}_start_button", challengeData.ChallengeID), DASEventViewName);
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

  protected override void ConstructOpenAnimation(Sequence openAnimation) {
    openAnimation.Append(transform.DOLocalMoveY(
      50, 0.15f).From().SetEase(Ease.OutQuad).SetRelative());
    if (_AlphaController != null) {
      _AlphaController.alpha = 0;
      openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
    }
  }

  protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    closeAnimation.Append(transform.DOLocalMoveY(
      -50, 0.15f).SetEase(Ease.OutQuad).SetRelative());
    if (_AlphaController != null) {
      closeAnimation.Join(_AlphaController.DOFade(0, 0.25f));
    }
  }
}
