using UnityEngine;
using System;
using UnityEngine.UI;
using Anki.UI;
using DG.Tweening;
using Anki.Cozmo.Audio;

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
      private IconProxy _Icon;

      // TODO remove
      private string _DasEventName;

      private string _TitleKey;
      private string _DescriptionKey;

      private void Awake() {
        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.gameObject.SetActive(false);
        }

        if (_Icon != null) {
          _Icon.gameObject.SetActive(false);
        }

        // Hide all buttons
        if (_PrimaryButton != null) {
          _PrimaryButton.gameObject.SetActive(false);
        }

        if (_SecondaryButton != null) {
          _SecondaryButton.gameObject.SetActive(false);
        }

        if (_SecondaryButtonLayoutElement != null) {
          _SecondaryButtonLayoutElement.gameObject.SetActive(false);
        }
      }

      protected override void CleanUp() {

      }

      public void InitializeAlertData(AlertModalData initializationData) {
        DASEventDialogName = initializationData.DasEventAlertName;

        InitializeLocalization(initializationData.TitleLocKey,
                               initializationData.DescriptionLocKey,
                               initializationData.TitleLocArgs,
                               initializationData.DescLocArgs);
        InitializeCloseButton(initializationData.ShowCloseButton);
        InitializePrimaryButton(initializationData.PrimaryButtonData,
                                initializationData.PrimaryButtonLocArgs);
        InitializeSecondaryButton(initializationData.SecondaryButtonData);
        InitializeIcon(initializationData.AlertIcon);

        if (initializationData.DialogCloseAnimationFinishedCallback != null) {
          this.DialogCloseAnimationFinished += initializationData.DialogCloseAnimationFinishedCallback;
        }
      }

      private void InitializeLocalization(string titleLocKey, string descLocKey, object[] titleLocArgs, object[] descLocArgs) {
        if (_AlertTitleText != null && !string.IsNullOrEmpty(titleLocKey)) {
          _TitleKey = titleLocKey;
          _AlertTitleText.text = Localization.Get(_TitleKey);
          if (titleLocArgs != null) {
            _AlertTitleText.FormattingArgs = titleLocArgs;
          }
        }
        if (_AlertMessageText != null && !string.IsNullOrEmpty(descLocKey)) {
          _DescriptionKey = descLocKey;
          _AlertMessageText.text = Localization.Get(_DescriptionKey);
          if (descLocArgs != null) {
            _AlertMessageText.FormattingArgs = descLocArgs;
          }
        }
      }

      private void InitializeCloseButton(bool showCloseButton) {
        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.gameObject.SetActive(showCloseButton);
          _OptionalCloseDialogButton.DASEventViewController = DASEventDialogName;
        }
        else {
          DAS.Warn("AlertModal.InitializeCloseButton.OptionalCloseDialogButtonNull", "Tried to set up a button that doesn't exist in this AlertModal! " + gameObject.name);
        }
      }

      private void InitializePrimaryButton(AlertModalButtonData buttonData, object[] primaryButtonLocArgs) {
        if (buttonData != null) {
          SetupButton(_PrimaryButton, buttonData.DasEventButtonName, buttonData.LabelLocKey, buttonData.ClickCallback, buttonData.ClickSoundEffect);
          if (primaryButtonLocArgs != null) {
            _PrimaryButton.FormattingArgs = primaryButtonLocArgs;
          }
        }
      }

      private void InitializeSecondaryButton(AlertModalButtonData buttonData) {
        if (buttonData != null) {
          SetupButton(_SecondaryButton, buttonData.DasEventButtonName, buttonData.LabelLocKey, buttonData.ClickCallback, buttonData.ClickSoundEffect);

          if (_SecondaryButtonLayoutElement != null) {
            _SecondaryButtonLayoutElement.gameObject.SetActive(true);
          }
        }
      }

      private void InitializeIcon(Sprite icon) {
        if (_Icon != null && icon != null) {
          _Icon.gameObject.SetActive(true);
          _Icon.SetIcon(icon);
        }
      }

      public void DisableAllButtons() {
        _PrimaryButton.Interactable = false;
        _SecondaryButton.Interactable = false;
      }

      private void SetupButton(Cozmo.UI.CozmoButton button, string dasEventButtonName, string titleKey, Action action,
                               Anki.Cozmo.Audio.AudioEventParameter audioParam = default(Anki.Cozmo.Audio.AudioEventParameter)) {
        if (button != null) {
          string title = Localization.Get(titleKey);
          button.gameObject.SetActive(true);
          button.Text = title;

          button.Initialize(() => {
            if (action != null) {
              action();
            }
            CloseDialog();
          }, dasEventButtonName, DASEventDialogName);

          if (!audioParam.IsInvalid()) {
            button.SoundEvent = audioParam;
          }
          else {
            button.SoundEvent = AudioEventParameter.DefaultClick;
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

    public class AlertModalData {
      public readonly string DasEventAlertName;
      public readonly string TitleLocKey;
      public readonly string DescriptionLocKey;
      public readonly bool ShowCloseButton;
      public readonly AlertModalButtonData PrimaryButtonData;
      public readonly AlertModalButtonData SecondaryButtonData;
      public readonly Sprite AlertIcon;
      public readonly BaseDialog.SimpleBaseDialogHandler DialogCloseAnimationFinishedCallback;
      public readonly object[] TitleLocArgs;
      public readonly object[] DescLocArgs;
      public readonly object[] PrimaryButtonLocArgs;

      public AlertModalData(string dasEventAlertName,
                            string titleLocKey,
                            string descLocKey = null,
                            AlertModalButtonData primaryButtonData = null,
                            AlertModalButtonData secondaryButtonData = null,
                            bool showCloseButton = false,
                            Sprite icon = null,
                            BaseDialog.SimpleBaseDialogHandler dialogCloseAnimationFinishedCallback = null,
                            object[] titleLocArgs = null,
                            object[] descLocArgs = null,
                            object[] primaryButtonLocArgs = null) {
        this.DasEventAlertName = dasEventAlertName;
        this.TitleLocKey = titleLocKey;
        this.DescriptionLocKey = descLocKey;
        this.ShowCloseButton = showCloseButton;
        this.PrimaryButtonData = primaryButtonData;
        this.SecondaryButtonData = secondaryButtonData;
        this.AlertIcon = icon;
        this.DialogCloseAnimationFinishedCallback = dialogCloseAnimationFinishedCallback;
        this.TitleLocArgs = titleLocArgs;
        this.DescLocArgs = descLocArgs;
        this.PrimaryButtonLocArgs = primaryButtonLocArgs;
      }
    }

    public class AlertModalButtonData {
      public readonly string DasEventButtonName;
      public readonly string LabelLocKey;
      public readonly Action ClickCallback;
      public readonly AudioEventParameter ClickSoundEffect;

      public AlertModalButtonData(string dasEventButtonName,
                                  string labelLocKey,
                                  Action clickCallback = null,
                                  AudioEventParameter clickSoundEffect = default(AudioEventParameter)) {
        this.DasEventButtonName = dasEventButtonName;
        this.LabelLocKey = labelLocKey;
        this.ClickCallback = clickCallback;
        this.ClickSoundEffect = clickSoundEffect;
      }
    }
  }
}