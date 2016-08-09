using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class QuitMinigameButton : MinigameWidget {

      private const float kAnimXOffset = 0f;
      private const float kAnimYOffset = 200.0f;
      private const float kAnimDur = 0.25f;
      // If this is from a minigame or activity, defaults to true
      private bool _IsMinigame = true;

      public delegate void QuitButtonHandler();

      public event QuitButtonHandler QuitGameConfirmed;

      [SerializeField]
      private CozmoButton _QuitButtonInstance;

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

      public override void DestroyWidgetImmediately() {
        _QuitButtonInstance.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      public override Sequence CreateOpenAnimSequence() {
        return CreateOpenAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      public override Sequence CreateCloseAnimSequence() {
        return CreateCloseAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      private void HandleQuitButtonTap() {
        // Open confirmation dialog instead
        AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab_NoText);
        // Hook up callbacks
        alertView.SetCloseButtonEnabled(false);
        alertView.SetPrimaryButton(LocalizationKeys.kButtonQuit, HandleQuitConfirmed);
        alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel, HandleQuitCancelled);
        if (_IsMinigame) {
          alertView.TitleLocKey = LocalizationKeys.kMinigameQuitViewTitle;
        }
        else {
          alertView.TitleLocKey = LocalizationKeys.kMinigameQuitViewTitleActivity;
        }
        // Listen for dialog close
        alertView.ViewCloseAnimationFinished += HandleQuitViewClosed;
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