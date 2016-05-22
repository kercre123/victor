using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DG.Tweening;

namespace Cozmo {
  namespace UI {
    public class AlertView : BaseView {

      [SerializeField]
      private AnkiTextLabel _AlertTitleText;

      [SerializeField]
      private AnkiTextLabel _AlertMessageText;

      [SerializeField]
      private Cozmo.UI.CozmoButton _PrimaryButton;

      [SerializeField]
      private Cozmo.UI.CozmoButton _SecondaryButton;

      [SerializeField]
      private Cozmo.UI.CozmoButton _CloseButton;

      [SerializeField]
      private CanvasGroup _AlphaController;

      [SerializeField]
      private IconProxy _Icon;

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
        Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.UI.WindowOpen);

        string viewName = GenerateDasName();

        if (_CloseButton != null) {
          _CloseButton.Initialize(
            () => {
              Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.UI.ClickBack);
              CloseView();
            },
            viewName,
            "close_alert_view_button");
          _CloseButton.gameObject.SetActive(false);
        }

        if (_Icon != null) {
          _Icon.gameObject.SetActive(false);
        }

        // Hide all buttons
        if (_PrimaryButton != null) {
          _PrimaryButton.DASEventViewController = viewName;
          _PrimaryButton.gameObject.SetActive(false);
        }

        if (_SecondaryButton != null) {
          _SecondaryButton.DASEventViewController = viewName;
          _SecondaryButton.gameObject.SetActive(false);
        }
      }

      protected override void CleanUp() {
        Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.UI.WindowClose);
        ResetButton(_PrimaryButton);
        ResetButton(_CloseButton);
        ResetButton(_SecondaryButton);
      }

      private void UpdateButtonViewControllerNames() {
        string viewName = GenerateDasName();
        if (_CloseButton != null) {
          _CloseButton.DASEventViewController = viewName;
        }

        // Hide all buttons
        if (_PrimaryButton != null) {
          _PrimaryButton.DASEventViewController = viewName;
        }

        if (_SecondaryButton != null) {
          _SecondaryButton.DASEventViewController = viewName;
        }
      }

      private string GenerateDasName() {
        string viewFormatting = "{0}_{1}";
        string viewName = null;
        if (_TitleKey != null) {
          viewName = string.Format(viewFormatting, DASEventViewName, _TitleKey);
        }
        else if (_DescriptionKey != null) {
          viewName = string.Format(viewFormatting, DASEventViewName, _DescriptionKey);
        }
        else {
          viewName = DASEventViewName;
        }
        return viewName;
      }

      private void ResetButton(Cozmo.UI.CozmoButton button) {
        if (button != null && button.isActiveAndEnabled) {
          button.onClick.RemoveAllListeners();
        }
      }

      public void SetCloseButtonEnabled(bool enabled) {
        if (_CloseButton != null) {
          _CloseButton.gameObject.SetActive(enabled);
        }
        else {
          DAS.Warn(this, "Tried to set up a button that doesn't exist in this AlertView! " + gameObject.name);
        }
      }

      public void SetPrimaryButton(string titleKey, Action action = null, 
                                   Anki.Cozmo.Audio.AudioEventParameter audioParam = default(Anki.Cozmo.Audio.AudioEventParameter)) {
        SetupButton(_PrimaryButton, titleKey, action, audioParam);
      }

      public void SetSecondaryButton(string titleKey, Action action = null, 
                                     Anki.Cozmo.Audio.AudioEventParameter audioParam = default(Anki.Cozmo.Audio.AudioEventParameter)) {
        SetupButton(_SecondaryButton, titleKey, action, audioParam);
      }

      public void SetTitleArgs(object[] args) {
        _AlertTitleText.FormattingArgs = args;
      }

      public void SetMessageArgs(object[] args) {
        _AlertMessageText.FormattingArgs = args;
      }

      public void DisableAllButtons() {
        ResetButton(_PrimaryButton);
        _PrimaryButton.Interactable = false;
        ResetButton(_SecondaryButton);
        _SecondaryButton.Interactable = false;
        if (_CloseButton != null) {
          ResetButton(_CloseButton);
          _CloseButton.Interactable = false;
        }
      }

      private void SetupButton(Cozmo.UI.CozmoButton button, string titleKey, Action action, 
                               Anki.Cozmo.Audio.AudioEventParameter audioParam = default(Anki.Cozmo.Audio.AudioEventParameter)) {
        if (button != null) {
          string title = Localization.Get(titleKey);
          button.gameObject.SetActive(true);
          button.Text = title;

          button.Initialize(() => {
            if (action != null) {
              action();
            }
            CloseView();
          }, string.Format("{0}_button", title), "alert_view");

          if (!audioParam.IsInvalid()) {
            button.SoundEvent = audioParam;
          }
        }
        else {
          DAS.Warn(this, "Tried to set up a button that doesn't exist in this AlertView! " + gameObject.name);
        }
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
  }
}
