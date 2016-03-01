using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class QuitMinigameButton : MinigameWidget {

      public delegate void QuitButtonHandler();

      public event QuitButtonHandler QuitGameConfirmed;

      [SerializeField]
      private AnkiButton _QuitButtonInstance;

      public string DASEventViewController {
        get { return _QuitButtonInstance.DASEventViewController; } 
        set { _QuitButtonInstance.DASEventViewController = value; }
      }

      private bool _ConfimedQuit = false;

      private void Start() {
        _QuitButtonInstance.DASEventButtonName = "quit_button";
        _QuitButtonInstance.onClick.AddListener(HandleQuitButtonTap);
      }

      public override void DestroyWidgetImmediately() {
        _QuitButtonInstance.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public override Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 200, 
          this.transform.localPosition.y + 200, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public override Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 200, 
          this.transform.localPosition.y + 200, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      private void HandleQuitButtonTap() {
        // Open confirmation dialog instead
        AlertView alertView = UIManager.OpenView(UIPrefabHolder.Instance.AlertViewPrefab) as AlertView;
        // Hook up callbacks
        alertView.SetCloseButtonEnabled(true);
        alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleQuitConfirmed, 
          Anki.Cozmo.Audio.AudioEventParameter.SFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameEnd));
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