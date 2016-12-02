using UnityEngine;
using System.Collections;
using DG.Tweening;
using Cozmo.UI;
using Anki.UI;

namespace Cozmo {
  namespace UI {
    public class BaseModal : MonoBehaviour {
      private enum ViewState {
        Initialized,
        IsOpening,
        Open,
        IsClosing,
        Closed
      }

      // Static events
      public delegate void BaseViewHandler(BaseModal view);

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

      public event SimpleBaseViewHandler ViewClosedByUser;
      public event SimpleBaseViewHandler ViewClosedByUserAnimationFinished;

      [SerializeField]
      private string _DASEventViewName = "";

      private ViewState _CurrentViewState = ViewState.Initialized;

      protected bool IsClosed {
        get { return _CurrentViewState == ViewState.IsClosing || _CurrentViewState == ViewState.Closed; }
      }

      public string DASEventViewName {
        get { return _DASEventViewName; }
        set { _DASEventViewName = value; }
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
      protected Cozmo.UI.CozmoButton _OptionalCloseDialogButton;

      [SerializeField]
      private int _LayerPriority = 0;

      public int LayerPriority {
        get { return _LayerPriority; }
      }

      public bool DimBackground = false;

      private Sequence _TransitionAnimation;

      // The UI event that should play when this view opens
      [SerializeField]
      private Anki.Cozmo.Audio.AudioEventParameter _OpenAudioEvent = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;

      public Anki.Cozmo.Audio.AudioEventParameter OpenAudioEvent {
        get { return _OpenAudioEvent; }
        set { _OpenAudioEvent = value; }
      }

      void OnDestroy() {
        if (_CurrentViewState != ViewState.Closed) {
          if (_CurrentViewState != ViewState.IsClosing) {
            RaiseViewClosed(this);
          }

          BaseViewCleanUpInternal();
        }
      }

      private bool _UserClosedView = false;

      #region Overriden Methods

      public void Initialize(bool? overrideCloseOnTapOutside = null) {
        bool shouldCloseOnTapOutside = overrideCloseOnTapOutside.HasValue ?
          overrideCloseOnTapOutside.Value : _CloseDialogOnTapOutside;
        if (shouldCloseOnTapOutside) {
          CreateFullScreenCloseCollider();
        }

        SetupCloseButton();

        CheckDASEventName();

        // TODO: In the future, separate the initialize and open dialog steps 
        // for ease of use
        _CurrentViewState = ViewState.Initialized;

        // TODO: This should be raised after open animation finished
        // and raise initialize event here instead?
        RaiseViewOpened(this);

        PlayOpenSound();

        PlayOpenAnimations();
      }

      protected virtual void CleanUp() {

      }

      protected virtual void ConstructOpenAnimation(Sequence openAnimation) {
      }

      // TODO: Make virtual function play from a set of default animations based on a serialized enum?
      // plus the pivot point and direction (top, bottom, left, right)
      // TODO: Make protected functions that return default tweeners you can add to the sequence?
      protected virtual void ConstructCloseAnimation(Sequence closeAnimation) {
      }

      #endregion

      private void CreateFullScreenCloseCollider() {
        GameObject fullScreenButton = UIManager.CreateUIElement(UIManager.Instance.TouchCatcherPrefab,
                                        this.transform);

        // Place the button underneath all the UI in this dialog
        fullScreenButton.transform.SetAsFirstSibling();
        Cozmo.UI.TouchCatcher fullScreenCollider = fullScreenButton.GetComponent<Cozmo.UI.TouchCatcher>();
        fullScreenCollider.OnTouch += (HandleUserClose);
        fullScreenCollider.Initialize("close_view_by_touch_outside_button", DASEventViewName);
      }

      private void SetupCloseButton() {
        if (_OptionalCloseDialogButton != null) {
          _OptionalCloseDialogButton.Initialize(HandleUserClose, "default_close_view_button", DASEventViewName);
        }
      }

      private void CheckDASEventName() {
        if (string.IsNullOrEmpty(_DASEventViewName)) {
          DAS.Error(this, string.Format("View is missing a _DASEventViewName! Please check the prefab. name={0}", gameObject.name));
          _DASEventViewName = gameObject.name;
        }
      }

      protected virtual void HandleUserClose() {
        _UserClosedView = true;
        CloseView();
      }

      protected virtual void Update() {
#if (UNITY_ANDROID && !UNITY_EDITOR)
        // Android Back button.
        if (_OptionalCloseDialogButton != null) {
          if (Input.GetKeyDown(KeyCode.Escape)) {
            HandleUserClose();
          }
        }
#endif
      }

      public void CloseView() {
        // if we are already closing the view don't make multiple calls
        // to the callback and don't try to play the close animation again
        if (_CurrentViewState != ViewState.IsClosing
            && _CurrentViewState != ViewState.Closed) {
          PlayCloseAnimations();
          RaiseViewClosed(this);
        }
      }

      public void CloseViewImmediately() {
        if (_CurrentViewState != ViewState.Closed) {
          if (_CurrentViewState != ViewState.IsClosing) {
            RaiseViewClosed(this);
          }

          BaseViewCleanUpInternal();
          Destroy(gameObject);
        }
      }

      // Play a sound when the view is shown (on calling Initialize)
      private void PlayOpenSound() {
        if (!OpenAudioEvent.IsInvalid()) {
          Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(OpenAudioEvent);
        }
      }

      private void PlayOpenAnimations() {
        if (_CurrentViewState == ViewState.Initialized) {
          _CurrentViewState = ViewState.IsOpening;
          UIManager.DisableTouchEvents();

          // Play some animations
          if (_TransitionAnimation != null) {
            _TransitionAnimation.Kill();
          }
          _TransitionAnimation = DOTween.Sequence();
          ConstructOpenAnimation(_TransitionAnimation);
          _TransitionAnimation.AppendCallback(OnOpenAnimationsFinished);
        }
      }

      private void OnOpenAnimationsFinished() {
        _CurrentViewState = ViewState.Open;
        UIManager.EnableTouchEvents();

        // Raise event
        RaiseViewOpenAnimationFinished(this);
      }

      private void PlayCloseAnimations() {
        _CurrentViewState = ViewState.IsClosing;
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
        if (_CurrentViewState != ViewState.Closed) {
          BaseViewCleanUpInternal();
          Destroy(gameObject);
        }
      }

      private void BaseViewCleanUpInternal() {
        _CurrentViewState = ViewState.Closed;
        if (UIManager.Instance != null) {
          UIManager.EnableTouchEvents();
        }
        if (_TransitionAnimation != null) {
          _TransitionAnimation.Kill();
        }
        CleanUp();
        RaiseViewCloseAnimationFinished(this);
      }

      private static void RaiseViewOpened(BaseModal view) {
        if (BaseViewOpened != null) {
          BaseViewOpened(view);
        }
        if (view.ViewOpened != null) {
          view.ViewOpened();
        }
      }

      private static void RaiseViewOpenAnimationFinished(BaseModal view) {
        if (BaseViewOpenAnimationFinished != null) {
          BaseViewOpenAnimationFinished(view);
        }
        if (view.ViewOpenAnimationFinished != null) {
          view.ViewOpenAnimationFinished();
        }
      }

      private static void RaiseViewClosed(BaseModal view) {
        if (BaseViewClosed != null) {
          BaseViewClosed(view);
        }
        if (view.ViewClosed != null) {
          view.ViewClosed();
        }
        if (view._UserClosedView && view.ViewClosedByUser != null) {
          view.ViewClosedByUser();
        }
      }

      private static void RaiseViewCloseAnimationFinished(BaseModal view) {
        if (BaseViewCloseAnimationFinished != null) {
          BaseViewCloseAnimationFinished(view);
        }
        if (view.ViewCloseAnimationFinished != null) {
          view.ViewCloseAnimationFinished();
        }
        if (view._UserClosedView && view.ViewClosedByUserAnimationFinished != null) {
          view.ViewClosedByUserAnimationFinished();
        }
      }
    }
  }
}
