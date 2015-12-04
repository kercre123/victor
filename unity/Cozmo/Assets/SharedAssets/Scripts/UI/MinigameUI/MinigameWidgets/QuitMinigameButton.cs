using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class QuitMinigameButton : MonoBehaviour, IMinigameWidget {

      public delegate void QuitButtonHandler();

      public event QuitButtonHandler QuitViewOpened;
      public event QuitButtonHandler QuitViewClosed;
      public event QuitButtonHandler QuitGameConfirmed;

      [SerializeField]
      private AnkiButton _QuitButtonInstance;

      private bool _ConfimedQuit = false;

      private void Start() {
        _QuitButtonInstance.onClick.AddListener(HandleQuitButtonTap);
      }

      public void DestroyWidgetImmediately() {
        _QuitButtonInstance.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      public void EnableInteractivity() {
        _QuitButtonInstance.interactable = true;
      }

      public void DisableInteractivity() {
        _QuitButtonInstance.interactable = false;
      }

      public Sequence OpenAnimationSequence() {
        return null;
      }

      public Sequence CloseAnimationSequence() {
        return null;
      }

      private void HandleQuitButtonTap() {
        // Open confirmation dialog instead
        AlertView alertView = UIManager.OpenView(UIPrefabHolder.Instance.AlertViewPrefab) as AlertView;
        // Hook up callbacks
        alertView.SetCloseButtonEnabled(true);
        alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleQuitConfirmed);
        alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleQuitCancelled);
        alertView.TitleLocKey = LocalizationKeys.kMinigameQuitViewTitle;
        alertView.DescriptionLocKey = LocalizationKeys.kMinigameQuitViewDescription;
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
        else {
          if (QuitViewClosed != null) {
            QuitViewClosed();
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
    }
  }
}