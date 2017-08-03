using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class QuitMinigameButton : MinigameWidget {

      private const float kAnimXOffset = 0f;
      private const float kAnimYOffset = 300.0f;
      private const float kAnimDur = 0.25f;
      // If this is from a minigame or activity, defaults to true
      private bool _IsMinigame = true;

      public delegate void QuitButtonHandler();

      public event QuitButtonHandler QuitGameConfirmed;

      [SerializeField]
      private CozmoButton _QuitButtonInstance;

      private AlertModal _QuitPopupInstance;

      public string DASEventViewController {
        get { return _QuitButtonInstance.DASEventViewController; }
        set { _QuitButtonInstance.DASEventViewController = value; }
      }

      private bool _ConfimedQuit = false;

      private void Awake() {
        _QuitButtonInstance.Initialize(HandleQuitButtonTap, "open_quit_game_confirm_view_button", "shared_minigame_view");
      }

      public void Initialize(bool isMinigame) {
        _IsMinigame = isMinigame;
      }

      public bool IsQuitAlertViewOpen() {
        return _QuitPopupInstance != null;
      }

      public override void DestroyWidgetImmediately() {
        _QuitButtonInstance.onClick.RemoveAllListeners();
        if (_QuitPopupInstance != null) {
          _QuitPopupInstance.CloseDialogImmediately();
        }
        Destroy(gameObject);
      }

      public override Sequence CreateOpenAnimSequence() {
        return CreateOpenAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      public override Sequence CreateCloseAnimSequence() {
        return CreateCloseAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      private void HandleQuitButtonTap() {
        string titleKey;
        if (_OverrideTitleKey != null)
          titleKey = _OverrideTitleKey;
        else {
          titleKey = _IsMinigame ? LocalizationKeys.kMinigameQuitViewTitle : LocalizationKeys.kMinigameQuitViewTitleActivity;
        }

        string confirmKey;
        if (_OverrideComfirmKey != null) {
          confirmKey = _OverrideComfirmKey;
        }
        else {
          confirmKey = LocalizationKeys.kButtonQuit;
        }
        var confirmQuitButtonData = new AlertModalButtonData("confirm_button", confirmKey, HandleQuitConfirmed,
                                                             Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.AudioMetaData.GameEvent.Ui.Click_Back));

        string cancelKey;
        if (_OverrideCancelKey != null) {
          cancelKey = _OverrideCancelKey;
        }
        else {
          cancelKey = LocalizationKeys.kButtonCancel;
        }
        var cancelQuitButtonData = new AlertModalButtonData("cancel_button", cancelKey, HandleQuitCancelled);

        var quitGameAlertData = new AlertModalData("quit_minigame_alert",
                                                   titleKey,
                                                   descLocKey: _OverrideDescriptionKey,
                                                   primaryButtonData: confirmQuitButtonData,
                                                   secondaryButtonData: cancelQuitButtonData,
                                                   dialogCloseAnimationFinishedCallback: HandleQuitViewClosed);

        var quitGamePriorityData = new ModalPriorityData(ModalPriorityLayer.Low, 1,
                                                         LowPriorityModalAction.CancelSelf,
                                                         HighPriorityModalAction.Stack);

        System.Action<AlertModal> quitAlertCreated = (alertModal) => {
          _QuitPopupInstance = alertModal;
        };

        UIManager.OpenAlert(quitGameAlertData, quitGamePriorityData, quitAlertCreated);

        _ConfimedQuit = false;
      }

      private void HandleQuitViewClosed() {
        if (_ConfimedQuit) {
          if (QuitGameConfirmed != null) {
            QuitGameConfirmed();
          }
        }
        _ConfimedQuit = false;
      }

      private void HandleQuitCancelled() {
        _ConfimedQuit = false;
      }

      private void HandleQuitConfirmed() {
        _ConfimedQuit = true;
      }

      private string _OverrideTitleKey = null;
      private string _OverrideDescriptionKey = null;
      private string _OverrideComfirmKey = null;
      private string _OverrideCancelKey = null;

      public void OverrideModalText(string titleKey = null, string descriptionKey = null, string confirmKey = null, string cancelKey = null) {
        if (titleKey != null) {
          _OverrideTitleKey = titleKey;
        }
        if (descriptionKey != null) {
          _OverrideDescriptionKey = descriptionKey;
        }
        if (confirmKey != null) {
          _OverrideComfirmKey = confirmKey;
        }
        if (cancelKey != null) {
          _OverrideCancelKey = cancelKey;
        }
      }

      public void SetButtonGraphic(Sprite sprite) {
        _QuitButtonInstance.SetButtonGraphic(sprite);
      }

      public void SetButtonTint(Color tintColor) {
        var colors = _QuitButtonInstance.colors;
        colors.normalColor = tintColor;
        _QuitButtonInstance.colors = colors;
      }
    }
  }
}