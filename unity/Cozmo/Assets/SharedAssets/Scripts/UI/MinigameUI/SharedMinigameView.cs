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

      [SerializeField]
      private GameObject _QuitGameButtonPrefab;

      private QuitMinigameButton _QuitButtonInstance;

      [SerializeField]
      private QuickQuitMinigameButton _QuickQuitGameButtonPrefab;

      private QuickQuitMinigameButton _QuickQuitButtonInstance;

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
      private HowToPlayButton _HowToPlayButtonPrefab;
      private HowToPlayButton _HowToPlayButtonInstance;

      [SerializeField]
      private RectTransform _FullScreenSlideContainer;

      [SerializeField]
      private RectTransform _GameInfoSlideContainer;

      [SerializeField]
      private UnityEngine.UI.LayoutElement _InfoTitleLayoutElement;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _InfoTitleTextLabel;

      private CanvasGroup _CurrentSlide;
      private string _CurrentSlideName;
      private Sequence _SlideInTween;
      private CanvasGroup _TransitionOutSlide;
      private Sequence _SlideOutTween;

      private List<IMinigameWidget> _ActiveWidgets = new List<IMinigameWidget>();

      public CanvasGroup CurrentSlide { get { return _CurrentSlide; } }

      public bool _OpenAnimationStarted = false;

      #region Base View

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

      #endregion

      private void Awake() {
        // Hide the info title by default. This also centers game state slides
        // by default.
        _InfoTitleLayoutElement.gameObject.SetActive(false);
      }

      private void AddWidget(IMinigameWidget widgetToAdd) {
        _ActiveWidgets.Add(widgetToAdd);
        if (_OpenAnimationStarted) {
          Sequence openAnimation = widgetToAdd.OpenAnimationSequence();
          openAnimation.Play();
        }
      }

      private void HideWidget(IMinigameWidget widgetToHide) {
        if (widgetToHide == null) {
          return;
        }

        _ActiveWidgets.Remove(widgetToHide);

        Sequence close = widgetToHide.CloseAnimationSequence();
        close.AppendCallback(() => {
          widgetToHide.DestroyWidgetImmediately();
        });
        close.Play();
      }

      #region Quit Button

      public void CreateQuitButton() {
        if (_QuitButtonInstance != null) {
          return;
        }

        GameObject newButton = UIManager.CreateUIElement(_QuitGameButtonPrefab, this.transform);

        _QuitButtonInstance = newButton.GetComponent<QuitMinigameButton>();
        _QuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;

        AddWidget(_QuitButtonInstance);
      }

      public void CreateQuickQuitButton() {
        if (_QuickQuitButtonInstance != null) {
          return;
        }

        GameObject newButton = UIManager.CreateUIElement(_QuickQuitGameButtonPrefab, this.transform);

        _QuickQuitButtonInstance = newButton.GetComponent<QuickQuitMinigameButton>();
        _QuickQuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;

        AddWidget(_QuickQuitButtonInstance);
      }

      public void HideQuickQuitButton() {
        if (_QuickQuitButtonInstance != null) {
          HideWidget(_QuickQuitButtonInstance);
          _QuickQuitButtonInstance = null;
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
        }
      }

      public void SetCozmoAttemptsLeft(int attemptsLeft) {
        if (_CozmoStatusInstance != null) {
          _CozmoStatusInstance.SetAttemptsLeft(attemptsLeft);
        }
        else {
          CreateCozmoStatusWidget(attemptsLeft);
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

      public void CreateTitleWidget(string titleText, Sprite titleSprite) {
        if (_TitleWidgetInstance != null) {
          return;
        }

        GameObject widgetObj = UIManager.CreateUIElement(_TitleWidgetPrefab.gameObject, this.transform);
        _TitleWidgetInstance = widgetObj.GetComponent<ChallengeTitleWidget>();

        _TitleWidgetInstance.Initialize(titleText, titleSprite);

        AddWidget(_TitleWidgetInstance);
      }

      #endregion

      #region How To Play Widget

      public void CreateHowToPlayButton(GameObject howToPlayContentsPrefab) {

        if (_HowToPlayButtonInstance != null) {
          return;
        }

        GameObject newButton = UIManager.CreateUIElement(_HowToPlayButtonPrefab, this.transform);

        _HowToPlayButtonInstance = newButton.GetComponent<HowToPlayButton>();

        _HowToPlayButtonInstance.Initialize(howToPlayContentsPrefab);

        AddWidget(_HowToPlayButtonInstance);
      }

      public void OpenHowToPlayView() {
        if (_HowToPlayButtonInstance != null) {
          _HowToPlayButtonInstance.OpenHowToPlayView(false, false);
        }
      }

      public void CloseHowToPlayView() {
        if (_HowToPlayButtonInstance != null) {
          _HowToPlayButtonInstance.CloseHowToPlayView();
        }
      }

      #endregion

      #region Game State Slides

      public GameObject ShowFullScreenSlide(GameStateSlide slideData) {
        return ShowGameStateSlide(slideData.slideName, slideData.slidePrefab.gameObject, 
          _FullScreenSlideContainer);
      }

      public GameObject ShowGameStateSlide(GameStateSlide slideData) {
        return ShowGameStateSlide(slideData.slideName, slideData.slidePrefab.gameObject, 
          _GameInfoSlideContainer);
      }

      private GameObject ShowGameStateSlide(string slideName, GameObject slidePrefab,
                                            RectTransform slideContainer) {
        if (slideName == _CurrentSlideName) {
          return _CurrentSlide.gameObject;
        }

        // If a slide already exists, play a transition out tween on it
        HideGameStateSlide();

        _CurrentSlideName = slideName;

        // Create the new slide underneath the container
        GameObject newSlideObj = UIManager.CreateUIElement(slidePrefab, slideContainer);
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
        if (_ContinueButtonShelfInstance != null) {
          HideWidget(_ContinueButtonShelfInstance);
          _ContinueButtonShelfInstance = null;
        }
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

      #region Info Title Text

      public string InfoTitleText {
        get { return _InfoTitleTextLabel.text; }
        set { 
          _InfoTitleLayoutElement.gameObject.SetActive(true);
          _InfoTitleTextLabel.text = value; 
        }
      }

      #endregion
    }
  }
}