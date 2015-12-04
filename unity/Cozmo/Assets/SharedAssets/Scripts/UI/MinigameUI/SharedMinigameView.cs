using UnityEngine;
using Cozmo.UI;
using DG.Tweening;
using System.Collections.Generic;

namespace Cozmo {
  namespace MinigameWidgets {
    public class SharedMinigameView : BaseView {

      public delegate void SharedMinigameViewHandler();

      public event SharedMinigameViewHandler QuitMiniGameConfirmed;
      public event SharedMinigameViewHandler QuitMiniGameViewOpened;
      public event SharedMinigameViewHandler QuitMiniGameViewClosed;

      [SerializeField]
      private GameObject _DefaultQuitGameButtonPrefab;

      private QuitMinigameButton _QuitButtonInstance;

      private List<IMinigameWidget> _ActiveWidgets = new List<IMinigameWidget>();

      protected override void CleanUp() {
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          widget.DestroyWidgetImmediately();
        }
        _ActiveWidgets.Clear();
      }

      protected override void ConstructOpenAnimation(Sequence openAnimation) {
        Sequence open;
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          open = widget.OpenAnimationSequence();
          if (open != null) {
            openAnimation.Join(open);
          }
        }
      }

      protected override void ConstructCloseAnimation(Sequence closeAnimation) {
        Sequence close;
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          close = widget.CloseAnimationSequence();
          if (close != null) {
            closeAnimation.Join(close);
          }
        }
      }

      public void EnableInteractivity() {
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          widget.EnableInteractivity();
        }
      }

      public void DisableInteractivity() {
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          widget.DisableInteractivity();
        }
      }

      #region Quit Button

      public void CreateQuitButton() {
        GameObject newButton = UIManager.CreateUIElement(_DefaultQuitGameButtonPrefab);

        _QuitButtonInstance = newButton.GetComponent<QuitMinigameButton>();

        _QuitButtonInstance.QuitViewOpened += HandleQuitViewOpened;
        _QuitButtonInstance.QuitViewClosed += HandleQuitViewClosed;
        _QuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;

        _ActiveWidgets.Add(_QuitButtonInstance);
      }

      private void HandleQuitViewOpened() {
        if (QuitMiniGameViewOpened != null) {
          QuitMiniGameViewOpened();
        }
      }

      private void HandleQuitViewClosed() {
        if (QuitMiniGameViewClosed != null) {
          QuitMiniGameViewClosed();
        }
      }

      private void HandleQuitConfirmed() {
        if (QuitMiniGameConfirmed != null) {
          QuitMiniGameConfirmed();
        }
      }

      #endregion
    }
  }
}