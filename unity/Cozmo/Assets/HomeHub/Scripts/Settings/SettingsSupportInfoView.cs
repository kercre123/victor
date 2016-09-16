using Anki.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.Settings {
  public class SettingsSupportInfoView : BaseView {
    public event System.Action OnOpenRestoreCozmoViewButtonTapped;

    [SerializeField]
    private CanvasGroup _AlphaController;

    [SerializeField]
    private CozmoButton _OpenRestoreCozmoDialogButton;

    [SerializeField]
    private AnkiTextLabel _ClarificationText;

    private void Start() {
      _OpenRestoreCozmoDialogButton.Initialize(HandleOpenRestoreCozmoViewButtonTapped, "open_restore_cozmo_view_button", this.DASEventViewName);
    }

    protected override void CleanUp() {
    }

    public void HideRestoreButton(bool isActive) {
      _OpenRestoreCozmoDialogButton.gameObject.SetActive(isActive);
      _ClarificationText.gameObject.SetActive(isActive);
    }

    private void HandleOpenRestoreCozmoViewButtonTapped() {
      if (OnOpenRestoreCozmoViewButtonTapped != null) {
        OnOpenRestoreCozmoViewButtonTapped();
      }
    }

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      UIDefaultTransitionSettings settings = UIDefaultTransitionSettings.Instance;
      openAnimation.Append(transform.DOLocalMoveY(
        50, 0.15f).From().SetEase(settings.MoveOpenEase).SetRelative());
      if (_AlphaController != null) {
        _AlphaController.alpha = 0;
        openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(settings.FadeInEasing));
      }
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      UIDefaultTransitionSettings settings = UIDefaultTransitionSettings.Instance;
      closeAnimation.Append(transform.DOLocalMoveY(
        -50, 0.15f).SetEase(settings.MoveCloseEase).SetRelative());
      if (_AlphaController != null) {
        closeAnimation.Join(_AlphaController.DOFade(0, 0.25f).SetEase(settings.FadeOutEasing));
      }
    }
  }
}