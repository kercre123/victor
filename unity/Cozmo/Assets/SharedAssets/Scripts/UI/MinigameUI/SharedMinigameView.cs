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
      private DifficultySelectView _DifficultySelectViewPrefab;
      private DifficultySelectView _DifficultySelectViewInstance;

      #endregion

      #region Slide System

      [SerializeField]
      private RectTransform _FullScreenSlideContainer;

      [SerializeField]
      private RectTransform _GameCustomInfoSlideContainer;

      [SerializeField]
      private UnityEngine.UI.LayoutElement _InfoTitleLayoutElement;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _InfoTitleTextLabel;

      [SerializeField]
      private RectTransform _InfoTextSlideContainer;

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

      private List<IMinigameWidget> _ActiveWidgets = new List<IMinigameWidget>();

      public CanvasGroup CurrentSlide { get { return _CurrentSlide; } }

      public bool _OpenAnimationStarted = false;

      private GameObject _HowToPlayContentPrefab;

      private string _HowToPlayContentLocKey;

      private GameStateSlide[] _GameStateSlides;

      public void Initialize(GameObject howToPlayContentPrefab, string howToPlayContentLocKey,
                             GameStateSlide[] gameStateSlides) {
        _HowToPlayContentPrefab = howToPlayContentPrefab;
        _HowToPlayContentLocKey = howToPlayContentLocKey;
        _GameStateSlides = gameStateSlides;
      }

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

      private void CreateWidgetIfNull<T>(ref T widgetInstance, MonoBehaviour widgetPrefab) where T : IMinigameWidget {
        if (widgetInstance != null) {
          return;
        }

        GameObject newWidget = UIManager.CreateUIElement(widgetPrefab.gameObject, this.transform);
        widgetInstance = newWidget.GetComponent<T>();
        AddWidget(widgetInstance);
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

      #region StaminaBar

      public void SetAttemptsMax(int maxAttempts) {
        CreateWidgetIfNull<CozmoStatusWidget>(ref _CozmoStatusInstance, _CozmoStatusPrefab);
        _CozmoStatusInstance.SetMaxAttempts(maxAttempts);
      }

      public void SetAttemptsLeft(int attemptsLeft) {
        CreateWidgetIfNull<CozmoStatusWidget>(ref _CozmoStatusInstance, _CozmoStatusPrefab);
        _CozmoStatusInstance.SetAttemptsLeft(attemptsLeft);
      }

      #endregion

      #region Challenge Progress Widget

      public void SetProgressBarText(string textToDisplay) {
        CreateWidgetIfNull<ChallengeProgressWidget>(ref _ChallengeProgressWidgetInstance, _ChallengeProgressWidgetPrefab);
        _ChallengeProgressWidgetInstance.ProgressBarLabelText = textToDisplay;
      }

      public void SetProgressBarNumSegments(int numSegments) {
        CreateWidgetIfNull<ChallengeProgressWidget>(ref _ChallengeProgressWidgetInstance, _ChallengeProgressWidgetPrefab);
        _ChallengeProgressWidgetInstance.NumSegments = numSegments;
      }

      public void SetProgress(float newProgress) {
        CreateWidgetIfNull<ChallengeProgressWidget>(ref _ChallengeProgressWidgetInstance, _ChallengeProgressWidgetPrefab);
        _ChallengeProgressWidgetInstance.SetProgress(newProgress);
      }

      #endregion

      #region Score Widgets

      public int CozmoScore {
        set {
          ShowCozmoScoreWidget();
          _CozmoScoreWidgetInstance.Score = value;
        }
      }

      public int CozmoMaxRounds {
        set {
          ShowCozmoScoreWidget();
          _CozmoScoreWidgetInstance.MaxRounds = value;
        }
      }

      public int CozmoRoundsWon {
        set {
          ShowCozmoScoreWidget();
          _CozmoScoreWidgetInstance.RoundsWon = value;
        }
      }

      public bool CozmoDim {
        set {
          ShowCozmoScoreWidget();
          _CozmoScoreWidgetInstance.Dim = value;
        }
      }

      public void ShowCozmoWinnerBanner() {
        ShowCozmoScoreWidget();
        _CozmoScoreWidgetInstance.IsWinner = true;
      }

      public void ShowCozmoScoreWidget() {
        if (_CozmoScoreWidgetInstance == null) {
          _CozmoScoreWidgetInstance = CreateScoreWidget(_CozmoScoreContainer, _ScoreEnterAnimationXOffset,
            _CozmoPortraitSprite);            
        }
      }

      public int PlayerScore {
        set {
          ShowPlayerScoreWidget();
          _PlayerScoreWidgetInstance.Score = value;
        }
      }

      public int PlayerMaxRounds {
        set {
          ShowPlayerScoreWidget();
          _PlayerScoreWidgetInstance.MaxRounds = value;
        }
      }

      public int PlayerRoundsWon {
        set {
          ShowPlayerScoreWidget();
          _PlayerScoreWidgetInstance.RoundsWon = value;
        }
      }

      public bool PlayerDim {
        set {
          ShowPlayerScoreWidget();
          _PlayerScoreWidgetInstance.Dim = value;
        }
      }

      public void ShowPlayerWinnerBanner() {
        ShowPlayerScoreWidget();
        _PlayerScoreWidgetInstance.IsWinner = true;
      }

      public void ShowPlayerScoreWidget() {
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

        AddWidget(instance);
        return instance;
      }

      public void HideScoreWidgets() {
        HideWidget(_CozmoScoreWidgetInstance);
        _CozmoScoreWidgetInstance = null;
        HideWidget(_PlayerScoreWidgetInstance);
        _PlayerScoreWidgetInstance = null;
      }

      #endregion

      #region Challenge Title Widget

      public void CreateTitleWidget(string titleText, Sprite titleSprite) {
        CreateWidgetIfNull<ChallengeTitleWidget>(ref _TitleWidgetInstance, _TitleWidgetPrefab);
        _TitleWidgetInstance.Initialize(titleText, titleSprite);
      }

      #endregion

      #region Quit Button

      public void CreateQuitButton() {
        CreateWidgetIfNull<QuitMinigameButton>(ref _QuitButtonInstance, _QuitGameButtonPrefab);
        _QuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;
      }

      public void CreateBackButton() {
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

      public void CreateHowToPlayButton() {
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
          HideExistingContinueButtonShelf();
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
          HideExistingContinueButtonShelf();
        }
        CreateWidgetIfNull<ContinueGameShelfWidget>(ref _ContinueButtonShelfInstance, _ContinueButtonCenterPrefab);
        _IsContinueButtonShelfCentered = true;
        _ContinueButtonShelfInstance.SetButtonListener(buttonClickHandler);
        _ContinueButtonShelfInstance.SetButtonText(buttonText);
      }

      private void HideExistingContinueButtonShelf() {
        if (_ContinueButtonShelfInstance != null) {
          HideContinueButtonShelf();
        }
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
        GameStateSlide cubesSlide = UIPrefabHolder.Instance.InitialCubesSlide;
        GameObject slideObject = ShowFullScreenSlide(cubesSlide.slidePrefab.gameObject, cubesSlide.slideName);
        ShowCozmoCubeSlide cubeSlide = slideObject.GetComponent<ShowCozmoCubeSlide>();
        cubeSlide.Initialize(numCubesRequired);
        return cubeSlide;
      }

      // TODO: Delete after removing usage by CodeBreaker, Minesweeper
      public GameObject ShowFullScreenSlideByName(string slideName) {
        // If found, show that slide.
        GameObject slideObject = null;
        GameStateSlide foundSlideData = GetSlideByName(slideName);
        if (foundSlideData != null) {
          slideObject = ShowFullScreenSlide(foundSlideData.slidePrefab.gameObject, foundSlideData.slideName);
        }
        else {
          DAS.Error(this, "Could not find slide with name '" + slideName + "' in slide prefabs! Check this object's" +
          " list of slides! gameObject.name=" + gameObject.name);
        }
        return slideObject;
      }

      public GameObject ShowFullScreenSlide(GameObject prefab, string slideKey) {
        HideScoreWidgets();
        InfoTitleText = string.Empty;
        return ShowGameStateSlide(slideKey, prefab, _FullScreenSlideContainer);
      }

      public GameObject ShowCustomGameStateSlide(GameObject prefab, string slideKey) {
        InfoTitleText = null;
        HideGameStateSlide();
        return ShowGameStateSlide(slideKey, prefab, _GameCustomInfoSlideContainer);
      }

      public void ShowInfoTextSlideWithKey(string localizationKey) {
        ShowInfoTextSlide(Localization.Get(localizationKey));
      }

      public void ShowInfoTextSlide(string textToDisplay) {
        GameObject slide = ShowGameStateSlide("Info Slide: " + textToDisplay, _InfoTextSlidePrefab.gameObject, 
                             _InfoTextSlideContainer);
        Anki.UI.AnkiTextLabel textLabel = slide.GetComponent<Anki.UI.AnkiTextLabel>();
        textLabel.text = textToDisplay;
      }

      public void HideGameStateSlide() {
        if (_CurrentSlide != null) {
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
            if (_TransitionOutSlide.gameObject != null) {
              Destroy(_TransitionOutSlide.gameObject);
            }
          });
          _SlideOutTween.Play();
        }
      }

      // TODO: Delete after removing usage by CodeBreaker, Minesweeper
      private GameStateSlide GetSlideByName(string slideName) {
        // Search through the array for a slide of the same name
        GameStateSlide foundSlideData = null;
        foreach (var slide in _GameStateSlides) {
          if (slide != null && slide.slideName == slideName) {
            if (slide.slidePrefab != null) {
              foundSlideData = slide;
            }
            else {
              DAS.Error(this, "Null prefab for slide with name '" + slideName + "'! Check this object's" +
              " list of slides! gameObject.name=" + gameObject.name);
            }
            break;
          }
          else if (slide == null) {
            DAS.Warn(this, "Null slide found in slide prefabs! Check this object's" +
            " list of slides! gameObject.name=" + gameObject.name);
          }
        }
        return foundSlideData;
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

      public void OpenDifficultySelectView(List<DifficultySelectOptionData> options, int highestDifficultyAvailable) {
        CreateWidgetIfNull<DifficultySelectView>(ref _DifficultySelectViewInstance, _DifficultySelectViewPrefab);
        _DifficultySelectViewInstance.Initialize(options, highestDifficultyAvailable);
      }

      public void CloseDifficultySelectView() {
        HideWidget(_DifficultySelectViewInstance);
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