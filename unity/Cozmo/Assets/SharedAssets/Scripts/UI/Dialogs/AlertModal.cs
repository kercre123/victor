using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DG.Tweening;

namespace Cozmo {
  namespace UI {
    public class AlertModal : BaseModal {

      [SerializeField]
      private AnkiTextLabel _AlertTitleText;

      [SerializeField]
      private AnkiTextLabel _AlertMessageText;

      [SerializeField]
      private Cozmo.UI.CozmoButton _PrimaryButton;

      [SerializeField]
      private Cozmo.UI.CozmoButton _SecondaryButton;

      [SerializeField]
      private LayoutElement _SecondaryButtonLayoutElement;

      [SerializeField]
      private CanvasGroup _AlphaController;

      [SerializeField]
      private IconProxy _Icon;

      private string _DasEventName;

      public void SetDasEventName(string dasEventName) {
        _DasEventName = dasEventName;
      }

      public void SetIcon(Sprite icon) {
        if (_Icon != null) {
          _Icon.gameObject.SetActive(true);
          _Icon.SetIcon(icon);
        }
      }

      private string _TitleKey;
      private string _DescriptionKey;

      public string TitleLocKey {
        get { return _AlertTitleText != null ? _AlertTitleText.text : null; }
        set {
          if (_TitleKey != value && _AlertTitleText != null) {
            _TitleKey = value;
            _AlertTitleText.text = Localization.Get(value);
            UpdateButtonViewControllerNames();
          }
        }
      }

      public string DescriptionLocKey {
        get { return _AlertMessageText != null ? _AlertMessageText.text : null; }
        set {
          if (_DescriptionKey != value && _AlertMessageText != null) {
            _DescriptionKey = value;
            _AlertMessageText.text = Localization.Get(value);
            UpdateButtonViewControllerNames();
          }
        }
      }

      private void Awake() {
        string alertDasName = GenerateDasName();

        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.gameObject.SetActive(false);
        }

        if (_Icon != null) {
          _Icon.gameObject.SetActive(false);
        }

        // Hide all buttons
        if (_PrimaryButton != null) {
          _PrimaryButton.DASEventViewController = alertDasName;
          _PrimaryButton.gameObject.SetActive(false);
        }

        if (_SecondaryButton != null) {
          _SecondaryButton.DASEventViewController = alertDasName;
          _SecondaryButton.gameObject.SetActive(false);
        }

        if (_SecondaryButtonLayoutElement != null) {
          _SecondaryButtonLayoutElement.gameObject.SetActive(false);
        }

      }

      protected override void CleanUp() {

      }

      private void UpdateButtonViewControllerNames() {
        string alertDasName = GenerateDasName();
        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.DASEventViewController = alertDasName;
        }

        // Hide all buttons
        if (_PrimaryButton != null) {
          _PrimaryButton.DASEventViewController = alertDasName;
        }

        if (_SecondaryButton != null) {
          _SecondaryButton.DASEventViewController = alertDasName;
        }
      }

      private string GenerateDasName() {
        string viewFormatting = "{0}_{1}";
        string alertDasName = null;
        if (_TitleKey != null) {
          alertDasName = string.Format(viewFormatting, DASEventViewName, _TitleKey);
        }
        else if (_DescriptionKey != null) {
          alertDasName = string.Format(viewFormatting, DASEventViewName, _DescriptionKey);
        }
        else {
          alertDasName = DASEventViewName;
        }
        return alertDasName;
      }

      public void SetCloseButtonEnabled(bool enabled) {
        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.gameObject.SetActive(enabled);
        }
        else {
          DAS.Warn("AlertModal.SetCloseButtonEnabled.OptionalCloseDialogButtonNull", "Tried to set up a button that doesn't exist in this AlertModal! " + gameObject.name);
        }
      }

      public void SetPrimaryButton(string titleKey, Action action = null,
                                   Anki.Cozmo.Audio.AudioEventParameter audioParam = default(Anki.Cozmo.Audio.AudioEventParameter)) {
        SetupButton(_PrimaryButton, titleKey, action, audioParam);
      }

      public void SetSecondaryButton(string titleKey, Action action = null,
                                     Anki.Cozmo.Audio.AudioEventParameter audioParam = default(Anki.Cozmo.Audio.AudioEventParameter)) {
        SetupButton(_SecondaryButton, titleKey, action, audioParam);

        if (_SecondaryButtonLayoutElement != null) {
          _SecondaryButtonLayoutElement.gameObject.SetActive(true);
        }
      }

      public void SetTitleArgs(object[] args) {
        _AlertTitleText.FormattingArgs = args;
      }

      public void SetMessageArgs(object[] args) {
        _AlertMessageText.FormattingArgs = args;
      }

      public void SetPrimaryButtonArgs(object[] args) {
        _PrimaryButton.FormattingArgs = args;
      }

      public void DisableAllButtons() {
        _PrimaryButton.Interactable = false;
        _SecondaryButton.Interactable = false;
      }

      private void SetupButton(Cozmo.UI.CozmoButton button, string titleKey, Action action,
                               Anki.Cozmo.Audio.AudioEventParameter audioParam = default(Anki.Cozmo.Audio.AudioEventParameter)) {
        if (button != null) {
          string title = Localization.Get(titleKey);
          button.gameObject.SetActive(true);
          button.Text = title;

          string dasEventName;
          if (string.IsNullOrEmpty(_DasEventName)) {
            dasEventName = title;
          }
          else {
            dasEventName = _DasEventName;
          }

          button.Initialize(() => {
            if (action != null) {
              action();
            }
            CloseView();
          }, string.Format("{0}_button", dasEventName), "alert_view");

          if (!audioParam.IsInvalid()) {
            button.SoundEvent = audioParam;
          }
        }
        else {
          DAS.Warn("AlertModal.SetupButton.TargetButtonNull", "Tried to set up a button that doesn't exist in this AlertModal! " + gameObject.name);
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
}
