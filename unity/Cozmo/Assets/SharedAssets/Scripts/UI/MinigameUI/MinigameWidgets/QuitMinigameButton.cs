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
      private CozmoButtonLegacy _QuitButtonInstance;

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
        string titleKey = _IsMinigame ? LocalizationKeys.kMinigameQuitViewTitle : LocalizationKeys.kMinigameQuitViewTitleActivity;

        var confirmQuitButtonData = new AlertModalButtonData("confirm_button", LocalizationKeys.kButtonQuit, HandleQuitConfirmed,
                                                             Anki.Cozmo.Audio.AudioEventParameter.UIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Click_Back));

        var cancelQuitButtonData = new AlertModalButtonData("cancel_button", LocalizationKeys.kButtonCancel, HandleQuitCancelled);

        var quitGameAlertData = new AlertModalData("quit_minigame_alert",
                                                   titleKey,
                                                   descLocKey: null,
                                                   primaryButtonData: confirmQuitButtonData,
                                                   secondaryButtonData: cancelQuitButtonData,
                                                   dialogCloseAnimationFinishedCallback: HandleQuitViewClosed);

        var quitGamePriorityData = new ModalPriorityData(ModalPriorityLayer.Low, 0,
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

      public void SetButtonGraphic(Sprite sprite) {
        _QuitButtonInstance.SetButtonGraphic(sprite);
      }

      public void SetButtonTint(Color tintColor) {
        _QuitButtonInstance.SetButtonTint(tintColor);
      }
    }
  }
}