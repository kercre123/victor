using UnityEngine;
using System.Collections;
using DG.Tweening;
using Cozmo.UI;
using Anki.UI;

namespace Cozmo {
  namespace UI {
    public abstract class BaseView : MonoBehaviour {
  
      // Static events
      public delegate void BaseViewHandler(BaseView view);

      public static event BaseViewHandler BaseViewOpened;
      public static event BaseViewHandler BaseViewOpenAnimationFinished;
      public static event BaseViewHandler BaseViewClosed;
      public static event BaseViewHandler BaseViewCloseAnimationFinished;

      // Instance events
      public delegate void SimpleBaseViewHandler();

      public event SimpleBaseViewHandler ViewOpened;
      public event SimpleBaseViewHandler ViewOpenAnimationFinished;
      public event SimpleBaseViewHandler ViewClosed;
      public event SimpleBaseViewHandler ViewCloseAnimationFinished;

      /// <summary>
      /// If true, creates a full screen button behind all the elements of this
      /// dialog. The button will close this dialog on click.
      /// </summary>
      [SerializeField]
      private bool _CloseDialogOnTapOutside;

      [SerializeField]
      private AnkiButton _OptionalCloseDialogButton;

      public bool DimBackground = false;

      private Sequence _TransitionAnimation;

      public void OnDestroy() {
        if (_TransitionAnimation != null) {
          _TransitionAnimation.Kill();
        }
      }

      #region Overriden Methods

      protected abstract void CleanUp();

      protected virtual void ConstructOpenAnimation(Sequence openAnimation) {
      }

      // TODO: Make virtual function play from a set of default animations based on a serialized enum?
      // plus the pivot point and direction (top, bottom, left, right)
      // TODO: Make protected functions that return default tweeners you can add to the sequence?
      protected virtual void ConstructCloseAnimation(Sequence closeAnimation) {
      }

      #endregion

      public void OpenView() {
        RaiseViewOpened(this);

        if (_CloseDialogOnTapOutside) {
          GameObject fullScreenButton = UIManager.CreateUIElement(UIPrefabHolder.Instance.FullScreenButtonPrefab,
                                          this.transform);

          // Place the button underneath all the UI in this dialog
          fullScreenButton.transform.SetAsFirstSibling();
          UnityEngine.UI.Button fullScreenCollider = fullScreenButton.GetComponent<UnityEngine.UI.Button>();
          fullScreenCollider.onClick.AddListener(HandleCloseColliderClicked);
        }
          
        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.onClick.AddListener(HandleCloseColliderClicked);
        }

        PlayOpenAnimations();
      }

      private void HandleCloseColliderClicked() {
        CloseView();
      }

      public void CloseView() {
        RaiseViewClosed(this);
        PlayCloseAnimations();
      }

      public void CloseViewImmediately() {
        RaiseViewClosed(this);

        // Close dialog without playing animations
        OnCloseAnimationsFinished();
      }

      private void PlayOpenAnimations() {
        UIManager.DisableTouchEvents();

        // Play some animations
        if (_TransitionAnimation != null) {
          _TransitionAnimation.Kill();
        }
        _TransitionAnimation = DOTween.Sequence();
        ConstructOpenAnimation(_TransitionAnimation);
        _TransitionAnimation.AppendCallback(OnOpenAnimationsFinished);
      }

      private void OnOpenAnimationsFinished() {
        UIManager.EnableTouchEvents();

        // Raise event
        RaiseViewOpenAnimationFinished(this);
      }

      private void PlayCloseAnimations() {
        UIManager.DisableTouchEvents();

        // Play some animations
        if (_TransitionAnimation != null) {
          _TransitionAnimation.Kill();
        }
        _TransitionAnimation = DOTween.Sequence();
        ConstructCloseAnimation(_TransitionAnimation);
        _TransitionAnimation.AppendCallback(OnCloseAnimationsFinished);
      }

      private void OnCloseAnimationsFinished() {
        UIManager.EnableTouchEvents();
        RaiseViewCloseAnimationFinished(this);
        CleanUp();
        GameObject.Destroy(gameObject);
      }

      private static void RaiseViewOpened(BaseView view) {
        if (view.ViewOpened != null) {
          view.ViewOpened();
        }
        if (BaseViewOpened != null) {
          BaseViewOpened(view);
        }
      }

      private static void RaiseViewOpenAnimationFinished(BaseView view) {
        if (view.ViewOpenAnimationFinished != null) {
          view.ViewOpenAnimationFinished();
        }
        if (BaseViewOpenAnimationFinished != null) {
          BaseViewOpenAnimationFinished(view);
        }
      }


      private static void RaiseViewClosed(BaseView view) {
        if (view.ViewClosed != null) {
          view.ViewClosed();
        }
        if (BaseViewClosed != null) {
          BaseViewClosed(view);
        }
      }

      private static void RaiseViewCloseAnimationFinished(BaseView view) {
        if (view.ViewCloseAnimationFinished != null) {
          view.ViewCloseAnimationFinished();
        }
        if (BaseViewCloseAnimationFinished != null) {
          BaseViewCloseAnimationFinished(view);
        }
      }
    }
  }
}
