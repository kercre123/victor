using UnityEngine;
using System.Collections;
using DG.Tweening;
using Cozmo.UI;
using Anki.UI;

namespace Cozmo {
  namespace UI {
    public class BaseView : MonoBehaviour {

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

      public event SimpleBaseViewHandler ViewClosedByUser;
      public event SimpleBaseViewHandler ViewClosedByUserAnimationFinished;

      [SerializeField]
      private string _DASEventViewName = "";

      private bool _ClosingAnimationPlaying = false;

      public bool ClosingAnimationPlaying {
        get { return _ClosingAnimationPlaying; }
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

      public bool DimBackground = false;
      public CanvasGroup DimBackgroundPrefabOverride = null;

      private Sequence _TransitionAnimation;

      // The UI event that should play when this view opens
      [SerializeField]
      private Anki.Cozmo.Audio.AudioEventParameter _OpenAudioEvent = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;

      public Anki.Cozmo.Audio.AudioEventParameter OpenAudioEvent {
        get { return _OpenAudioEvent; }
        set { _OpenAudioEvent = value; }
      }

      void OnDestroy() {
        if (_ClosingAnimationPlaying) {
          DAS.Warn("BaseView.OnDestroy", "BaseView being destroyed when close animation has not finished: " + _DASEventViewName);
        }
        if (_TransitionAnimation != null) {
          _TransitionAnimation.Kill();
        }

        // make sure we re-enable touch events in case this gets killed before animation finish callbacks are invoked.
        if (UIManager.Instance != null) {
          UIManager.EnableTouchEvents();
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
        if (_ClosingAnimationPlaying) {
          return;
        }
        RaiseViewClosed(this);
        PlayCloseAnimations();
      }

      public void CloseViewImmediately() {
        // if we are already closing the view then don't call the view closed callback again,
        // but do proceed to stop the animation anyway.
        if (!_ClosingAnimationPlaying) {
          RaiseViewClosed(this);
        }

        // Close dialog without playing animations
        OnCloseAnimationsFinished();
      }

      // Play a sound when the view is shown (on calling Initialize)
      private void PlayOpenSound() {
        if (!OpenAudioEvent.IsInvalid()) {
          Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(OpenAudioEvent);
        }
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
        _ClosingAnimationPlaying = true;

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
        if (UIManager.Instance != null) {
          UIManager.EnableTouchEvents();
        }
        _ClosingAnimationPlaying = false;
        CleanUp();
        RaiseViewCloseAnimationFinished(this);
        Destroy(gameObject);
        DAS.Info(this, "OnCloseAnimationsFinished finished");
      }

      private static void RaiseViewOpened(BaseView view) {
        if (BaseViewOpened != null) {
          BaseViewOpened(view);
        }
        if (view.ViewOpened != null) {
          view.ViewOpened();
        }
      }

      private static void RaiseViewOpenAnimationFinished(BaseView view) {
        if (BaseViewOpenAnimationFinished != null) {
          BaseViewOpenAnimationFinished(view);
        }
        if (view.ViewOpenAnimationFinished != null) {
          view.ViewOpenAnimationFinished();
        }
      }


      private static void RaiseViewClosed(BaseView view) {
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

      private static void RaiseViewCloseAnimationFinished(BaseView view) {
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
