using Anki.UI;
using DG.Tweening;
using UnityEngine;
using System.Collections;

namespace Cozmo.UI {
  public class LongPressConfirmationModal : BaseModal {

    [SerializeField]
    private AnkiTextLabel _TitleTextLabel;

    [SerializeField]
    private AnkiTextLabel _WarningTextLabel;

    [SerializeField]
    private RectTransform _InstructionTextContainer;

    [SerializeField]
    private AnkiTextLabel _InstructionTextLabel;

    [SerializeField]
    private RectTransform _InProgressTextContainer;

    [SerializeField]
    private AnkiTextLabel _InProgressTextLabel;

    [SerializeField]
    private CozmoButton _CancelButton;

    [SerializeField]
    private CozmoHoldButton _ConfirmHoldButton;

    public void Initialize(string dasEventViewName, string titleLocKey, string warningTextLocKey,
                           string cancelTextLocKey, string confirmTextLocKey,
                           System.Action confirmCallback, float secondsToFillBar) {
      this.DASEventDialogName = dasEventViewName;
      _TitleTextLabel.text = Localization.Get(titleLocKey);
      _WarningTextLabel.text = Localization.Get(warningTextLocKey);

      _InProgressTextContainer.gameObject.SetActive(false);
      _InstructionTextContainer.gameObject.SetActive(false);

      _ConfirmHoldButton.Initialize(confirmCallback, "confirm_hold_button", dasEventViewName, secondsToFillBar);
      _ConfirmHoldButton.Text = Localization.Get(confirmTextLocKey);

      _CancelButton.Initialize(() => {
        this.CloseDialog();
      }, "cancel_button", dasEventViewName);
      _CancelButton.Text = Localization.Get(cancelTextLocKey);
    }

    public void ShowInstructionsLabel(string text) {
      _InProgressTextContainer.gameObject.SetActive(false);
      _InstructionTextContainer.gameObject.SetActive(true);
      _InstructionTextLabel.text = text;
    }

    public void ShowInProgressLabel(string text) {
      _InProgressTextContainer.gameObject.SetActive(true);
      _InstructionTextContainer.gameObject.SetActive(false);
      _InProgressTextLabel.text = text;
    }

    public void EnableButtons(bool enabled) {
      _CancelButton.Interactable = enabled;
      _ConfirmHoldButton.Interactable = enabled;
      if (_ConfirmHoldButton.Interactable) {
        _ConfirmHoldButton.ResetButton();
      }
    }

    protected override void CleanUp() {
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