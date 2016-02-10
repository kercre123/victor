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

      public void CreateHowToPlayButton() {
        if (_HowToPlayButtonInstance != null) {
          return;
        }

        GameObject newButton = UIManager.CreateUIElement(_HowToPlayButtonPrefab, this.transform);

        _HowToPlayButtonInstance = newButton.GetComponent<HowToPlayButton>();

        if (_HowToPlayContentPrefab != null) {
          _HowToPlayButtonInstance.Initialize(_HowToPlayContentPrefab);
        }
        else {
          _HowToPlayButtonInstance.Initialize(_HowToPlayContentLocKey);
        }
        _HowToPlayButtonInstance.Initialize(_HowToPlayContentPrefab);

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

      #region Difficulty Select Widget

      public void OpenDifficultySelectView(List<DifficultySelectOptionData> options, int highestDifficultyAvailable) {
        if (_DifficultySelectViewInstance != null) {
          _DifficultySelectViewInstance.Initialize(options, highestDifficultyAvailable);
          return;
        }

        GameObject newView = UIManager.CreateUIElement(_DifficultySelectViewPrefab, this.transform);

        _DifficultySelectViewInstance = newView.GetComponent<DifficultySelectView>();

        _DifficultySelectViewInstance.Initialize(options, highestDifficultyAvailable);

        AddWidget(_DifficultySelectViewInstance);
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
        HideInfoTitleText();
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

      #region ContinueGameShelfWidget

      public void ShowContinueButtonShelf(bool centerShelf) {
        if (_ContinueButtonShelfInstance != null) {
          if (_IsContinueButtonShelfCentered == centerShelf) {
            return;
          }
          else {
            HideContinueButtonShelf();
          }
        }

        ContinueGameShelfWidget prefabToUse = centerShelf ? _ContinueButtonCenterPrefab : _ContinueButtonShelfPrefab;
        _IsContinueButtonShelfCentered = centerShelf;
        GameObject widgetObj = UIManager.CreateUIElement(prefabToUse.gameObject, this.transform);
        _ContinueButtonShelfInstance = widgetObj.GetComponent<ContinueGameShelfWidget>();

        AddWidget(_ContinueButtonShelfInstance);
      }

      public void HideContinueButtonShelf() {
        HideWidget(_ContinueButtonShelfInstance);
        _ContinueButtonShelfInstance = null;
      }

      public void SetContinueButtonShelfText(string text, bool isComplete) {
        _ContinueButtonShelfInstance.SetShelfText(text, isComplete);
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
          _InfoTitleLayoutElement.gameObject.SetActive(!string.IsNullOrEmpty(value));
          _InfoTitleTextLabel.text = value; 
        }
      }

      private void HideInfoTitleText() {
        _InfoTitleLayoutElement.gameObject.SetActive(false);
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
    }
  }
}