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

      [SerializeField]
      private string _DASEventViewName = "";

      public string DASEventViewName {
        get { return _DASEventViewName; } 
        protected set { _DASEventViewName = value; }
      }

      [SerializeField]
      private DASUtil.ViewType _DASEventViewType = DASUtil.ViewType.View;

      public DASUtil.ViewType DASEventViewType {
        get { return _DASEventViewType; } 
      }

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

      void OnDestroy() {
        if (_TransitionAnimation != null) {
          _TransitionAnimation.Kill();
        }
      }

      #region Overriden Methods

      public void Initialize(bool? overrideCloseOnTapOutside = null) {
        bool shouldCloseOnTapOutside = overrideCloseOnTapOutside.HasValue ? 
          overrideCloseOnTapOutside.Value : _CloseDialogOnTapOutside;
        if (shouldCloseOnTapOutside) {
          CreateFullScreenCloseCollider();
        }

        SetupCloseButton();

        CheckDASEventName();

        RaiseViewOpened(this);

        PlayOpenAnimations();
      }

      protected abstract void CleanUp();

      protected virtual void ConstructOpenAnimation(Sequence openAnimation) {
      }

      // TODO: Make virtual function play from a set of default animations based on a serialized enum?
      // plus the pivot point and direction (top, bottom, left, right)
      // TODO: Make protected functions that return default tweeners you can add to the sequence?
      protected virtual void ConstructCloseAnimation(Sequence closeAnimation) {
      }

      #endregion

      private void CreateFullScreenCloseCollider() {
        GameObject fullScreenButton = UIManager.CreateUIElement(UIPrefabHolder.Instance.FullScreenButtonPrefab,
                                        this.transform);

        // Place the button underneath all the UI in this dialog
        fullScreenButton.transform.SetAsFirstSibling();
        Cozmo.UI.TouchCatcher fullScreenCollider = fullScreenButton.GetComponent<Cozmo.UI.TouchCatcher>();
        fullScreenCollider.OnTouch += (HandleCloseColliderClicked);
        fullScreenCollider.DASEventButtonName = "fullscreen_close_button";
        fullScreenCollider.DASEventViewController = DASEventViewName;
      }

      private void SetupCloseButton() {
        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.DASEventButtonName = "close_button";
          _OptionalCloseDialogButton.DASEventViewController = DASEventViewName;
          _OptionalCloseDialogButton.onClick.AddListener(HandleCloseColliderClicked);
        }
      }

      private void CheckDASEventName() {
        if (string.IsNullOrEmpty(_DASEventViewName)) {
          DAS.Error(this, string.Format("View is missing a _DASEventViewName! Please check the prefab. name={0}", gameObject.name));
          _DASEventViewName = gameObject.name;
        }
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
        DAS.Info(this, "OnCloseAnimationsFinished start");
        UIManager.EnableTouchEvents();
        RaiseViewCloseAnimationFinished(this);
        CleanUp();
        GameObject.Destroy(gameObject);
        DAS.Info(this, "OnCloseAnimationsFinished finished");
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
