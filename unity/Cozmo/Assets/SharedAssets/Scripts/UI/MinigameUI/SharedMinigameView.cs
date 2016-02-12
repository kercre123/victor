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

      #region Simple widgets

      [SerializeField]
      private QuitMinigameButton _QuitGameButtonPrefab;

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

      [SerializeField]
      private ContinueGameShelfWidget _ContinueButtonCenterPrefab;

      private bool _IsContinueButtonShelfCentered;
      private ContinueGameShelfWidget _ContinueButtonShelfInstance;

      [SerializeField]
      private HowToPlayButton _HowToPlayButtonPrefab;
      private HowToPlayButton _HowToPlayButtonInstance;

      [SerializeField]
      private DifficultySelectSlide _DifficultySelectViewPrefab;
      private DifficultySelectSlide _DifficultySelectViewInstance;

      #endregion

      #region Slide System

      [SerializeField]
      private RectTransform _WideGameSlideContainer;

      [SerializeField]
      private RectTransform _NarrowGameSlideContainer;

      [SerializeField]
      private UnityEngine.UI.LayoutElement _InfoTitleLayoutElement;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _InfoTitleTextLabel;

      [SerializeField]
      private RectTransform _InfoTextGameSlideContainer;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _InfoTextSlidePrefab;

      #endregion

      #region Score Widget variables

      [SerializeField]
      private ScoreWidget _ScoreWidgetPrefab;

      private ScoreWidget _CozmoScoreWidgetInstance;
      private ScoreWidget _PlayerScoreWidgetInstance;

      [SerializeField]
      private float _ScoreEnterAnimationXOffset = 600f;

      [SerializeField]
      private RectTransform _CozmoScoreContainer;

      [SerializeField]
      private RectTransform _PlayerScoreContainer;

      [SerializeField]
      private Sprite _CozmoPortraitSprite;

      [SerializeField]
      private Sprite _PlayerPortraitSprite;

      #endregion

      private CanvasGroup _CurrentSlide;
      private string _CurrentSlideName;
      private Sequence _SlideInTween;
      private CanvasGroup _TransitionOutSlide;
      private Sequence _SlideOutTween;

      private List<MinigameWidget> _ActiveWidgets = new List<MinigameWidget>();

      public CanvasGroup CurrentSlide { get { return _CurrentSlide; } }

      public bool _OpenAnimationStarted = false;

      private GameObject _HowToPlayContentPrefab;

      private string _HowToPlayContentLocKey;

      public void Initialize(GameObject howToPlayContentPrefab, string howToPlayContentLocKey) {
        _HowToPlayContentPrefab = howToPlayContentPrefab;
        _HowToPlayContentLocKey = howToPlayContentLocKey;
      }

      #region Base View

      protected override void CleanUp() {
        foreach (MinigameWidget widget in _ActiveWidgets) {
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
        foreach (MinigameWidget widget in _ActiveWidgets) {
          open = widget.OpenAnimationSequence();
          if (open != null) {
            openAnimation.Join(open);
          }
        }
        _OpenAnimationStarted = true;
      }

      protected override void ConstructCloseAnimation(Sequence closeAnimation) {
        Sequence close;
        foreach (MinigameWidget widget in _ActiveWidgets) {
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

      private void CreateWidgetIfNull<T>(ref T widgetInstance, MonoBehaviour widgetPrefab) where T : MinigameWidget {
        if (widgetInstance != null) {
          return;
        }

        GameObject newWidget = UIManager.CreateUIElement(widgetPrefab.gameObject, this.transform);
        widgetInstance = newWidget.GetComponent<T>();
        AddWidget(widgetInstance);
      }

      private void AddWidget(MinigameWidget widgetToAdd) {
        _ActiveWidgets.Add(widgetToAdd);
        if (_OpenAnimationStarted) {
          Sequence openAnimation = widgetToAdd.OpenAnimationSequence();
          openAnimation.Play();
        }
      }

      private void HideWidget(MinigameWidget widgetToHide) {
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

      #region Challenge Title Widget

      public ChallengeTitleWidget TitleWidget {
        get {
          CreateWidgetIfNull<ChallengeTitleWidget>(ref _TitleWidgetInstance, _TitleWidgetPrefab);
          return _TitleWidgetInstance;
        }
      }

      #endregion

      #region StaminaBar

      public CozmoStatusWidget AttemptBar {
        get {
          CreateWidgetIfNull<CozmoStatusWidget>(ref _CozmoStatusInstance, _CozmoStatusPrefab);
          return _CozmoStatusInstance;
        }
      }

      #endregion

      #region Challenge Progress Widget

      public ChallengeProgressWidget ProgressBar {
        get {
          CreateWidgetIfNull<ChallengeProgressWidget>(ref _ChallengeProgressWidgetInstance, _ChallengeProgressWidgetPrefab);
          return _ChallengeProgressWidgetInstance;
        }
      }

      #endregion

      #region Score Widgets

      public ScoreWidget CozmoScoreboard {
        get {
          ShowCozmoScoreWidget();
          return _CozmoScoreWidgetInstance;
        }
      }

      private void ShowCozmoScoreWidget() {
        if (_CozmoScoreWidgetInstance == null) {
          _CozmoScoreWidgetInstance = CreateScoreWidget(_CozmoScoreContainer, _ScoreEnterAnimationXOffset,
            _CozmoPortraitSprite);            
        }
      }

      public ScoreWidget PlayerScoreboard {
        get {
          ShowPlayerScoreWidget();
          return _PlayerScoreWidgetInstance;
        }
      }

      private void ShowPlayerScoreWidget() {
        if (_PlayerScoreWidgetInstance == null) {
          _PlayerScoreWidgetInstance = CreateScoreWidget(_PlayerScoreContainer, -_ScoreEnterAnimationXOffset,
            _PlayerPortraitSprite);            
        }
      }

      private ScoreWidget CreateScoreWidget(RectTransform widgetParent, float animationOffset,
                                            Sprite portrait) {
        GameObject widgetObj = UIManager.CreateUIElement(_ScoreWidgetPrefab.gameObject, widgetParent);
        ScoreWidget instance = widgetObj.GetComponent<ScoreWidget>();
        instance.AnimationXOffset = animationOffset;
        instance.Portrait = portrait;
        instance.IsWinner = false;
        instance.Dim = false;

        AddWidget(instance);
        return instance;
      }

      #endregion

      #region Quit Button

      public void ShowQuitButton() {
        CreateWidgetIfNull<QuitMinigameButton>(ref _QuitButtonInstance, _QuitGameButtonPrefab);
        _QuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;
      }

      public void ShowBackButton() {
        CreateWidgetIfNull<QuickQuitMinigameButton>(ref _QuickQuitButtonInstance, _QuickQuitGameButtonPrefab);
        _QuickQuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;
      }

      public void HideBackButton() {
        HideWidget(_QuickQuitButtonInstance);
        _QuickQuitButtonInstance = null;
      }

      private void HandleQuitConfirmed() {
        CloseHowToPlayView();
        if (QuitMiniGameConfirmed != null) {
          QuitMiniGameConfirmed();
        }
      }

      #endregion

      #region How To Play Button

      public void ShowHowToPlayButton() {
        CreateWidgetIfNull<HowToPlayButton>(ref _HowToPlayButtonInstance, _HowToPlayButtonPrefab);
        _HowToPlayButtonInstance.Initialize(_HowToPlayContentLocKey, _HowToPlayContentPrefab);
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

      #region ContinueButtonShelfWidget

      public void ShowContinueButtonOnShelf(ContinueGameShelfWidget.ContinueButtonClickHandler buttonClickHandler,
                                            string buttonText, string shelfText, Color shelfColor) {
        if (_IsContinueButtonShelfCentered) {
          if (_ContinueButtonShelfInstance != null) {
            HideContinueButtonShelf();
          }
        }
        CreateWidgetIfNull<ContinueGameShelfWidget>(ref _ContinueButtonShelfInstance, _ContinueButtonShelfPrefab);
        _IsContinueButtonShelfCentered = false;
        _ContinueButtonShelfInstance.SetButtonListener(buttonClickHandler);
        _ContinueButtonShelfInstance.SetButtonText(buttonText);
        _ContinueButtonShelfInstance.SetShelfText(shelfText, shelfColor);
      }

      public void ShowContinueButtonCentered(ContinueGameShelfWidget.ContinueButtonClickHandler buttonClickHandler,
                                             string buttonText) {
        if (!_IsContinueButtonShelfCentered) {
          if (_ContinueButtonShelfInstance != null) {
            HideContinueButtonShelf();
          }
        }
        CreateWidgetIfNull<ContinueGameShelfWidget>(ref _ContinueButtonShelfInstance, _ContinueButtonCenterPrefab);
        _IsContinueButtonShelfCentered = true;
        _ContinueButtonShelfInstance.SetButtonListener(buttonClickHandler);
        _ContinueButtonShelfInstance.SetButtonText(buttonText);
      }

      public void HideContinueButtonShelf() {
        HideWidget(_ContinueButtonShelfInstance);
        _ContinueButtonShelfInstance = null;
      }

      public void EnableContinueButton(bool enable) {
        if (_ContinueButtonShelfInstance != null) {
          _ContinueButtonShelfInstance.SetButtonInteractivity(enable);
        }
      }

      public void SetContinueButtonShelfText(string text, Color color) {
        if (_ContinueButtonShelfInstance != null) {
          _ContinueButtonShelfInstance.SetShelfText(text, color);
        }
      }

      #endregion

      #region Info Title Text

      public string InfoTitleText {
        get { return _InfoTitleTextLabel.text; }
        set { 
          _InfoTitleLayoutElement.gameObject.SetActive(!string.IsNullOrEmpty(value));
          _InfoTitleTextLabel.text = value; 
        }
      }

      #endregion

      #region Game State Slides

      public ShowCozmoCubeSlide ShowCozmoCubesSlide(int numCubesRequired) {
        GameObject slideObject = ShowWideGameStateSlide(UIPrefabHolder.Instance.InitialCubesSlide, "ShowCubesSlide");
        ShowCozmoCubeSlide cubeSlide = slideObject.GetComponent<ShowCozmoCubeSlide>();
        cubeSlide.Initialize(numCubesRequired);
        return cubeSlide;
      }

      public GameObject ShowWideGameStateSlide(GameObject prefab, string slideKey) {
        InfoTitleText = null;
        HideGameStateSlide();
        return ShowGameStateSlide(slideKey, prefab, _WideGameSlideContainer);
      }

      public GameObject ShowNarrowGameStateSlide(GameObject prefab, string slideKey) {
        InfoTitleText = null;
        HideGameStateSlide();
        return ShowGameStateSlide(slideKey, prefab, _NarrowGameSlideContainer);
      }

      public void ShowInfoTextSlideWithKey(string localizationKey) {
        ShowInfoTextSlide(Localization.Get(localizationKey));
      }

      public void ShowInfoTextSlide(string textToDisplay) {
        GameObject slide = ShowGameStateSlide("Info Slide: " + textToDisplay, _InfoTextSlidePrefab.gameObject, 
                             _InfoTextGameSlideContainer);
        Anki.UI.AnkiTextLabel textLabel = slide.GetComponent<Anki.UI.AnkiTextLabel>();
        textLabel.text = textToDisplay;
      }

      public void HideGameStateSlide() {
        if (_CurrentSlide != null) {

          if (_TransitionOutSlide != null) {
            Destroy(_TransitionOutSlide.gameObject);
          }

          // Set the instance to transition out slot
          _TransitionOutSlide = _CurrentSlide;
          _CurrentSlide = null;
          _CurrentSlideName = null;

          if (_SlideOutTween != null) {
            _SlideOutTween.Kill();
          }
          _SlideOutTween = DOTween.Sequence();
          _SlideOutTween.Append(_TransitionOutSlide.transform.DOLocalMoveX(
            100, 0.25f).SetEase(Ease.OutQuad).SetRelative());
          _SlideOutTween.Join(_TransitionOutSlide.DOFade(0, 0.25f));

          // At the end of the tween destroy the out slide
          _SlideOutTween.OnComplete(() => {
            if (_TransitionOutSlide != null) {
              Destroy(_TransitionOutSlide.gameObject);
            }
          });
          _SlideOutTween.Play();
        }
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

        if (_SlideInTween != null) {
          _SlideInTween.Kill();
        }

        // Play a transition in tween on it
        _SlideInTween = DOTween.Sequence();
        _SlideInTween.Append(_CurrentSlide.transform.DOLocalMoveX(
          -100, 0.25f).From().SetEase(Ease.OutQuad).SetRelative());
        _SlideInTween.Join(_CurrentSlide.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
        _SlideInTween.Play();

        return newSlideObj;
      }

      #endregion

      #region Difficulty Select Widget

      public void ShowDifficultySelectView(List<DifficultySelectOptionData> options, int highestDifficultyAvailable) {
        if (_DifficultySelectViewInstance != null) {
          return;
        }
        GameObject difficultySlide = ShowWideGameStateSlide(_DifficultySelectViewPrefab.gameObject, "DifficultySelectSlide");
        _DifficultySelectViewInstance = difficultySlide.GetComponent<DifficultySelectSlide>();
        _DifficultySelectViewInstance.Initialize(options, highestDifficultyAvailable);
      }

      public void HideDifficultySelectView() {
        HideGameStateSlide();
        _DifficultySelectViewInstance = null;
      }

      public DifficultySelectOptionData GetSelectedDifficulty() {
        if (_DifficultySelectViewInstance != null) {
          return _DifficultySelectViewInstance.SelectedDifficulty;
        }
        return null;
      }

      #endregion
    }
  }
}