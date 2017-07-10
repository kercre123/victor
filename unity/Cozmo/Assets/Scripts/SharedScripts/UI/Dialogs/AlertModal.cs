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
      private CozmoText _AlertTitleText;

      [SerializeField]
      private CozmoText _AlertMessageText;

      [SerializeField]
      private Cozmo.UI.CozmoButton _PrimaryButton;

      [SerializeField]
      private Cozmo.UI.CozmoButton _SecondaryButton;

      [SerializeField]
      private LayoutElement _SecondaryButtonLayoutElement;

      [SerializeField]
      private CozmoImage _Icon;

      private string _TitleKey;
      private string _DescriptionKey;

      private void Awake() {
        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.gameObject.SetActive(false);
        }

        if (_OptionalCloseDialogCozmoButton != null) {
          _OptionalCloseDialogCozmoButton.gameObject.SetActive(false);
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
        CancelInvoke("HandleTimeoutComplete");
      }

      // This function is referenced by stringname in Invoke
      private void HandleTimeoutComplete() {
        if (!IsClosed) {
          CloseDialog();
        }
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

        if (initializationData.Timeout_Sec > 0.0f) {
          Invoke("HandleTimeoutComplete", initializationData.Timeout_Sec);
        }
      }

      private void InitializeLocalization(string titleLocKey, string descLocKey, object[] titleLocArgs, object[] descLocArgs) {
        if (_AlertTitleText != null && !string.IsNullOrEmpty(titleLocKey)) {
          _TitleKey = titleLocKey;

          if (titleLocArgs != null) {
            _AlertTitleText.FormattingArgs = titleLocArgs;
          }
          _AlertTitleText.key = _TitleKey;
        }
        if (_AlertMessageText != null && !string.IsNullOrEmpty(descLocKey)) {
          _DescriptionKey = descLocKey;
          _AlertMessageText.key = _DescriptionKey;
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

        if (_OptionalCloseDialogCozmoButton != null) {
          _OptionalCloseDialogCozmoButton.gameObject.SetActive(showCloseButton);
          _OptionalCloseDialogCozmoButton.DASEventViewController = DASEventDialogName;
        }
        else {
          DAS.Warn("AlertModal.InitializeCloseButton.OptionalCloseDialogButtonNull", "Tried to set up a button that doesn't exist in this AlertModal! " + gameObject.name);
        }
      }

      private void InitializePrimaryButton(AlertModalButtonData buttonData, object[] primaryButtonLocArgs) {
        if (buttonData != null) {
          SetupButton(_PrimaryButton, buttonData.DasEventButtonName, buttonData.LabelLocKey,
                      buttonData.ClickCallback, buttonData.ClickSoundEffect, buttonData.themeType);
          if (primaryButtonLocArgs != null) {
            _PrimaryButton.FormattingArgs = primaryButtonLocArgs;
          }
        }
      }

      private void InitializeSecondaryButton(AlertModalButtonData buttonData) {
        if (buttonData != null) {
          SetupButton(_SecondaryButton, buttonData.DasEventButtonName, buttonData.LabelLocKey,
                      buttonData.ClickCallback, buttonData.ClickSoundEffect, buttonData.themeType);

          if (_SecondaryButtonLayoutElement != null) {
            _SecondaryButtonLayoutElement.gameObject.SetActive(true);
          }
        }
      }

      private void InitializeIcon(Sprite icon) {
        if (_Icon != null && icon != null) {
          _Icon.gameObject.SetActive(true);
          _Icon.sprite = icon;
        }
      }

      public void DisableAllButtons() {
        _PrimaryButton.Interactable = false;
        _SecondaryButton.Interactable = false;
      }

      private void SetupButton(Cozmo.UI.CozmoButton button, string dasEventButtonName, string titleKey, Action action,
                               Anki.Cozmo.Audio.AudioEventParameter audioParam = default(Anki.Cozmo.Audio.AudioEventParameter),
                               AlertModalButtonData.ThemeType themeType = AlertModalButtonData.ThemeType.Default) {
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
          SetupButtonTheme(button, themeType);
        }
        else {
          DAS.Warn("AlertModal.SetupButton.TargetButtonNull", "Tried to set up a button that doesn't exist in this AlertModal! " + gameObject.name);
        }
      }

      private void SetupButtonTheme(CozmoButton button, AlertModalButtonData.ThemeType themeType) {
        if (themeType != AlertModalButtonData.ThemeType.Default) {
          CozmoImage mainGraphic = button.ButtonGraphics[0].targetImage;
          switch (themeType) {
          case AlertModalButtonData.ThemeType.Negative:
            button.LinkedComponentId = ThemeKeys.Cozmo.Button.kCozmoButtonPrimaryRed;
            mainGraphic.LinkedComponentId = ThemeKeys.Cozmo.Image.kCozmoButtonPrimaryRedBackground;
            break;
          case AlertModalButtonData.ThemeType.Neutral:
            button.LinkedComponentId = ThemeKeys.Cozmo.Button.kCozmoButtonPrimaryGrey;
            mainGraphic.LinkedComponentId = ThemeKeys.Cozmo.Image.kCozmoButtonPrimaryGreyBackground;
            break;
          case AlertModalButtonData.ThemeType.Positive:
            button.LinkedComponentId = ThemeKeys.Cozmo.Button.kCozmoButtonPrimaryBlue;
            mainGraphic.LinkedComponentId = ThemeKeys.Cozmo.Image.kCozmoButtonPrimaryBlueBackground;
            break;
          case AlertModalButtonData.ThemeType.Reward:
            button.LinkedComponentId = ThemeKeys.Cozmo.Button.kCozmoButtonPrimaryGold;
            mainGraphic.LinkedComponentId = ThemeKeys.Cozmo.Image.kCozmoButtonPrimaryGoldBackground;
            break;
          default:
            DAS.Error(this, "No theme data setup for " + themeType);
            return;
          }
          button.UpdateSkinnableElements();
          mainGraphic.UpdateSkinnableElements();
          //Enabled sprite is not set in json but loaded from prefab, 
          //have to force it to update or the button won't be right.
          button.ButtonGraphics[0].enabledSprite = mainGraphic.sprite;
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
      // < 0 for no timeout
      public readonly float Timeout_Sec;

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
                            object[] primaryButtonLocArgs = null,
                            float timeoutSec = -1.0f) {
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
        this.Timeout_Sec = timeoutSec;
      }
    }

    public class AlertModalButtonData {
      public enum ThemeType {
        Default,
        Negative,
        Neutral,
        Positive,
        Reward
      }
      public readonly ThemeType themeType;
      public readonly string DasEventButtonName;
      public readonly string LabelLocKey;
      public readonly Action ClickCallback;
      public readonly AudioEventParameter ClickSoundEffect;

      public AlertModalButtonData(string dasEventButtonName,
                                  string labelLocKey,
                                  Action clickCallback = null,
                                  AudioEventParameter clickSoundEffect = default(AudioEventParameter),
                                 ThemeType themeType = ThemeType.Default) {
        this.DasEventButtonName = dasEventButtonName;
        this.LabelLocKey = labelLocKey;
        this.ClickCallback = clickCallback;
        this.themeType = themeType;
        this.ClickSoundEffect = clickSoundEffect;
      }
    }
  }
}