using UnityEngine;
using Cozmo.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  namespace MinigameWidgets {
    public class SharedMinigameView : BaseView {

      public delegate void SharedMinigameViewHandler();

      public event SharedMinigameViewHandler QuitMiniGameConfirmed;
      public event SharedMinigameViewHandler QuitMiniGameViewOpened;
      public event SharedMinigameViewHandler QuitMiniGameViewClosed;

      [SerializeField]
      private GameObject _QuitGameButtonPrefab;

      private QuitMinigameButton _QuitButtonInstance;

      [SerializeField]
      private CozmoStatusWidget _CozmoStatusPrefab;

      private CozmoStatusWidget _CozmoStatusInstance;

      [SerializeField]
      private ChallengeProgressWidget _ChallengeProgressWidgetPrefab;

      private ChallengeProgressWidget _ChallengeProgressWidgetInstance;

      [SerializeField]
      private ChallengeTitleWidget _TitleWidgetPrefab;

      private ChallengeTitleWidget _TitleWidgetInstance;

      [SerializeField]
      private ContinueGameShelfWidget _ContinueButtonShelfPrefab;
      private ContinueGameShelfWidget _ContinueButtonShelfInstance;

      [SerializeField]
      private RectTransform _HowToSlideContainer;

      private CanvasGroup _CurrentSlide;
      private string _CurrentSlideName;
      private Sequence _SlideInTween;
      private CanvasGroup _TransitionOutSlide;
      private Sequence _SlideOutTween;

      private List<IMinigameWidget> _ActiveWidgets = new List<IMinigameWidget>();

      public CanvasGroup CurrentSlide { get { return _CurrentSlide; } }

      public bool _OpenAnimationStarted = false;

      protected override void CleanUp() {
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          widget.DestroyWidgetImmediately();
        }
        _ActiveWidgets.Clear();

        // If there are any slides or tweens running, destroy them right away
        if (_SlideInTween != null) {
          _SlideInTween.Kill();
        }
        if (_SlideOutTween != null) {
          _SlideOutTween.Kill();
        }
        if (_CurrentSlide != null) {
          Destroy(_CurrentSlide.gameObject);
        }
        if (_TransitionOutSlide != null) {
          Destroy(_TransitionOutSlide.gameObject);
        }
      }

      protected override void ConstructOpenAnimation(Sequence openAnimation) {
        Sequence open;
        foreach (IMinigameWidget widget in _ActiveWidgets) {
          open = widget.OpenAnimationSequence();
          if (open != null) {
            openAnimation.Join(open);
          }
        }
        _OpenAnimationStarted = true;
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

      private void AddWidget(IMinigameWidget widgetToAdd) {
        _ActiveWidgets.Add(widgetToAdd);
        if (_OpenAnimationStarted) {
          Sequence openAnimation = widgetToAdd.OpenAnimationSequence();
          openAnimation.Play();
        }
      }

      #region Quit Button

      public void CreateQuitButton() {
        if (_QuitButtonInstance != null) {
          return;
        }

        GameObject newButton = UIManager.CreateUIElement(_QuitGameButtonPrefab, this.transform);

        _QuitButtonInstance = newButton.GetComponent<QuitMinigameButton>();

        _QuitButtonInstance.QuitViewOpened += HandleQuitViewOpened;
        _QuitButtonInstance.QuitViewClosed += HandleQuitViewClosed;
        _QuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;

        AddWidget(_QuitButtonInstance);
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

      #region StaminaBar

      public void SetMaxCozmoAttempts(int maxAttempts) {
        if (_CozmoStatusInstance != null) {
          _CozmoStatusInstance.SetMaxAttempts(maxAttempts);
        }
        else {
          CreateCozmoStatusWidget(maxAttempts);
          // TODO: Play animation, if dialog had already been opened?
        }
      }

      public void SetCozmoAttemptsLeft(int attemptsLeft) {
        if (_CozmoStatusInstance != null) {
          _CozmoStatusInstance.SetAttemptsLeft(attemptsLeft);
        }
        else {
          CreateCozmoStatusWidget(attemptsLeft);
          // TODO: Play animation, if dialog had already been opened?
        }
      }

      private void CreateCozmoStatusWidget(int attemptsAllowed) {
        if (_CozmoStatusInstance != null) {
          return;
        }

        GameObject statusWidgetObj = UIManager.CreateUIElement(_CozmoStatusPrefab.gameObject, this.transform);
        _CozmoStatusInstance = statusWidgetObj.GetComponent<CozmoStatusWidget>();
        _CozmoStatusInstance.SetMaxAttempts(attemptsAllowed);
        _CozmoStatusInstance.SetAttemptsLeft(attemptsAllowed);
        AddWidget(_CozmoStatusInstance);
      }

      #endregion

      #region Challenge Progress Widget

      public string ProgressBarLabelText {
        get {
          return _ChallengeProgressWidgetInstance != null ? _ChallengeProgressWidgetInstance.ProgressBarLabelText : null;
        }
        set {
          if (_ChallengeProgressWidgetInstance == null) {
            CreateProgressWidget(value);
          }
          else {
            _ChallengeProgressWidgetInstance.ProgressBarLabelText = value;
          }
        }
      }

      public int NumSegments {
        get {
          return _ChallengeProgressWidgetInstance != null ? _ChallengeProgressWidgetInstance.NumSegments : 1;
        }
        set {
          if (_ChallengeProgressWidgetInstance == null) {
            CreateProgressWidget(null);
          }
          _ChallengeProgressWidgetInstance.NumSegments = value;
        }
      }

      public void SetProgress(float newProgress) {
        if (_ChallengeProgressWidgetInstance == null) {
          CreateProgressWidget(null);
        }
        _ChallengeProgressWidgetInstance.SetProgress(newProgress);
      }

      private void CreateProgressWidget(string progressLabelText = null) {
        if (_ChallengeProgressWidgetInstance != null) {
          return;
        }

        GameObject widgetObj = UIManager.CreateUIElement(_ChallengeProgressWidgetPrefab.gameObject, this.transform);
        _ChallengeProgressWidgetInstance = widgetObj.GetComponent<ChallengeProgressWidget>();

        if (!string.IsNullOrEmpty(progressLabelText)) {
          _ChallengeProgressWidgetInstance.ProgressBarLabelText = progressLabelText;
        }
        _ChallengeProgressWidgetInstance.ResetProgress();

        AddWidget(_ChallengeProgressWidgetInstance);
      }

      #endregion

      #region Challenge Title Widget

      public string TitleText {
        set {
          if (_TitleWidgetInstance != null) {
            _TitleWidgetInstance.TitleLabelText = value;
          }
          else {
            CreateTitleWidget(value, null);
          }
        }
      }

      public UnityEngine.Sprite TitleIcon {
        set {
          if (_TitleWidgetInstance != null) {
            _TitleWidgetInstance.TitleIcon = value;
          }
          else {
            CreateTitleWidget(null, value);
          }
        }
      }

      private void CreateTitleWidget(string titleText, Sprite titleSprite) {
        if (_TitleWidgetInstance != null) {
          return;
        }

        GameObject widgetObj = UIManager.CreateUIElement(_TitleWidgetPrefab.gameObject, this.transform);
        _TitleWidgetInstance = widgetObj.GetComponent<ChallengeTitleWidget>();

        if (!string.IsNullOrEmpty(titleText)) {
          _TitleWidgetInstance.TitleLabelText = titleText;
        }

        if (titleSprite != null) {
          _TitleWidgetInstance.TitleIcon = titleSprite;
        }

        AddWidget(_TitleWidgetInstance);
      }

      #endregion

      #region How To Play Slides

      public GameObject ShowGameStateSlide(GameObject slidePrefab) {
        return ShowGameStateSlide(slidePrefab.name, slidePrefab);
      }

      public GameObject ShowGameStateSlide(GameStateSlide slideData) {
        return ShowGameStateSlide(slideData.slideName, slideData.slidePrefab.gameObject);
      }

      private GameObject ShowGameStateSlide(string slideName, GameObject slidePrefab) {
        if (slideName == _CurrentSlideName) {
          return _CurrentSlide.gameObject;
        }

        // If a slide already exists, play a transition out tween on it
        HideGameStateSlide();

        _CurrentSlideName = slideName;

        // Create the new slide underneath the container
        GameObject newSlideObj = UIManager.CreateUIElement(slidePrefab, _HowToSlideContainer);
        _CurrentSlide = newSlideObj.GetComponent<CanvasGroup>();
        if (_CurrentSlide == null) {
          _CurrentSlide = newSlideObj.AddComponent<CanvasGroup>();
          _CurrentSlide.interactable = true;
          _CurrentSlide.blocksRaycasts = true;
        }
        _CurrentSlide.alpha = 0;

        // Play a transition in tween on it
        _SlideInTween = DOTween.Sequence();
        _SlideInTween.Append(_CurrentSlide.transform.DOLocalMoveX(
          100, 0.25f).From().SetEase(Ease.OutQuad).SetRelative());
        _SlideInTween.Join(_CurrentSlide.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
        _SlideInTween.Play();

        return newSlideObj;
      }

      public void HideGameStateSlide() {
        if (_CurrentSlide != null) {
          // Set the instance to transition out slot
          _TransitionOutSlide = _CurrentSlide;
          _CurrentSlide = null;
          _CurrentSlideName = null;

          _SlideOutTween = DOTween.Sequence();
          _SlideOutTween.Append(_TransitionOutSlide.transform.DOLocalMoveX(
            -100, 0.25f).SetEase(Ease.OutQuad).SetRelative());
          _SlideOutTween.Join(_TransitionOutSlide.DOFade(0, 0.25f));

          // At the end of the tween destroy the out slide
          _SlideOutTween.OnComplete(() => {
            Destroy(_TransitionOutSlide.gameObject);
          });
          _SlideOutTween.Play();
        }
      }

      #endregion

      #region ContinueGameShelfWidget

      public void ShowContinueButtonShelf() {
        if (_ContinueButtonShelfInstance != null) {
          return;
        }

        GameObject widgetObj = UIManager.CreateUIElement(_ContinueButtonShelfPrefab.gameObject, this.transform);
        _ContinueButtonShelfInstance = widgetObj.GetComponent<ContinueGameShelfWidget>();

        AddWidget(_ContinueButtonShelfInstance);
      }

      public void HideContinueButtonShelf() {
        if (_ContinueButtonShelfInstance == null) {
          return;
        }

        ContinueGameShelfWidget oldShelf = _ContinueButtonShelfInstance;
        _ActiveWidgets.Remove(_ContinueButtonShelfInstance);
        _ContinueButtonShelfInstance = null;

        Sequence close = oldShelf.CloseAnimationSequence();
        close.AppendCallback(() => {
          oldShelf.DestroyWidgetImmediately();
        });
        close.Play();
      }

      public void SetContinueButtonShelfText(string text) {
        _ContinueButtonShelfInstance.SetShelfText(text);
      }

      public void SetContinueButtonText(string text) {
        _ContinueButtonShelfInstance.SetButtonText(text);
      }

      public void SetContinueButtonListener(ContinueGameShelfWidget.ContinueButtonClickHandler buttonClickHandler) {
        _ContinueButtonShelfInstance.SetButtonListener(buttonClickHandler);
      }

      public void EnableContinueButton(bool enable) {
        _ContinueButtonShelfInstance.SetButtonInteractivity(enable);
      }

      #endregion
    }
  }
}