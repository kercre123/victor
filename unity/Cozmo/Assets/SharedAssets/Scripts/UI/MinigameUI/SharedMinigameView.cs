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
      private UnityEngine.UI.Image _LockedBackgroundImage;

      private bool _IsShowingLocked = false;

      private Sequence _LockedBackgroundTween;

      [SerializeField]
      private UnityEngine.UI.Image _BackgroundImage;

      private Color _BaseColor;

      private Sequence _BackgroundColorTween;

      #region Top widgets

      [SerializeField]
      private QuitMinigameButton _QuitGameButtonPrefab;

      private QuitMinigameButton _QuitButtonInstance;

      [SerializeField]
      private QuickQuitMinigameButton _QuickQuitGameButtonPrefab;

      private QuickQuitMinigameButton _QuickQuitButtonInstance;

      [SerializeField]
      private ChallengeTitleWidget _TitleWidgetPrefab;

      private ChallengeTitleWidget _TitleWidgetInstance;

      #endregion

      #region Bottom widgets

      [SerializeField]
      private CozmoStatusWidget _CozmoStatusPrefab;

      private CozmoStatusWidget _CozmoStatusInstance;

      [SerializeField]
      private ChallengeProgressWidget _ChallengeProgressWidgetPrefab;

      private ChallengeProgressWidget _ChallengeProgressWidgetInstance;

      [SerializeField]
      private ContinueGameButtonWidget _ContinueButtonOffsetPrefab;

      [SerializeField]
      private ContinueGameButtonWidget _ContinueButtonCenterPrefab;

      private bool _IsContinueButtonCentered;
      private ContinueGameButtonWidget _ContinueButtonInstance;

      [SerializeField]
      private HowToPlayButton _HowToPlayButtonPrefab;
      private HowToPlayButton _HowToPlayButtonInstance;

      [SerializeField]
      private ShelfWidget _ShelfWidgetPrefab;
      private ShelfWidget _ShelfWidgetInstance;

      public ShelfWidget ShelfWidget {
        get {
          CreateWidgetIfNull<ShelfWidget>(ref _ShelfWidgetInstance, _ShelfWidgetPrefab);
          return _ShelfWidgetInstance;
        }
      }

      #endregion

      #region Difficulty Select

      [SerializeField]
      private DifficultySelectButtonPanel _DifficultySelectButtonPanelPrefab;
      private DifficultySelectButtonPanel _DifficultySelectButtonPanelInstance;

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

      #endregion

      #region Default Slides

      [SerializeField]
      private Anki.UI.AnkiTextLabel _InfoTextSlidePrefab;

      [SerializeField]
      private AnimationSlide _AnimationSlidePrefab;

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
          if (widget != null && widget.gameObject != null) {
            widget.DestroyWidgetImmediately();
          }
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

        if (_LockedBackgroundTween != null) {
          _LockedBackgroundTween.Kill();
        }
        if (_BackgroundColorTween != null) {
          _BackgroundColorTween.Kill();
        }
      }

      protected override void ConstructOpenAnimation(Sequence openAnimation) {
        Sequence open;
        foreach (MinigameWidget widget in _ActiveWidgets) {
          open = widget.CreateOpenAnimSequence();
          if (open != null) {
            openAnimation.Join(open);
          }
        }
        _OpenAnimationStarted = true;
      }

      protected override void ConstructCloseAnimation(Sequence closeAnimation) {
        Sequence close;
        foreach (MinigameWidget widget in _ActiveWidgets) {
          close = widget.CreateCloseAnimSequence();
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
          Sequence openAnimation = widgetToAdd.CreateOpenAnimSequence();
          openAnimation.Play();
        }
      }

      private void HideWidget(MinigameWidget widgetToHide) {
        if (widgetToHide == null) {
          return;
        }

        _ActiveWidgets.Remove(widgetToHide);

        Sequence close = widgetToHide.CreateCloseAnimSequence();
        close.AppendCallback(() => {
          if (widgetToHide != null) {
            widgetToHide.DestroyWidgetImmediately();
          }
        });
        close.Play();
      }

      private void UpdateButtonDasViewControllerNames(string slideName) {
        string currentViewName = ComposeDasViewName(slideName);
        if (_QuitButtonInstance != null) {
          _QuitButtonInstance.DASEventViewController = currentViewName;
        }

        if (_QuickQuitButtonInstance != null) {
          _QuickQuitButtonInstance.DASEventViewController = currentViewName;
        }

        if (_HowToPlayButtonInstance != null) {
          _HowToPlayButtonInstance.DASEventViewController = currentViewName;
        }

        if (_ContinueButtonInstance != null) {
          _ContinueButtonInstance.DASEventViewController = currentViewName;
        }
      }

      private string ComposeDasViewName(string slideName) {
        return string.Format("{0}_{1}", DASEventViewName, 
          string.IsNullOrEmpty(slideName) ? "no_slide" : slideName);
      }

      #region Background Color

      public void InitializeColor(Color baseColor) {
        _BaseColor = baseColor;
        SetBackgroundColor(baseColor);
      }

      public void SetBackgroundColor(Color newColor) {
        if (_BackgroundImage.color != newColor) {
          if (_BackgroundColorTween != null) {
            _BackgroundColorTween.Kill();
          }
          _BackgroundColorTween = DOTween.Sequence();
          _BackgroundColorTween.Append(_BackgroundImage.DOColor(newColor, 0.2f));
        }
      }

      public void ResetBackgroundColor() {
        SetBackgroundColor(_BaseColor);
      }

      public void ShowLockedBackground() {
        if (!_IsShowingLocked) {
          _IsShowingLocked = true;
          if (_LockedBackgroundTween != null) {
            _LockedBackgroundTween.Kill();
          }
          _LockedBackgroundTween = DOTween.Sequence();
          _LockedBackgroundTween.Append(_LockedBackgroundImage.DOFade(1, 0.2f));
        }
      }

      public void HideLockedBackground() {
        if (_IsShowingLocked) {
          _IsShowingLocked = false;
          if (_LockedBackgroundTween != null) {
            _LockedBackgroundTween.Kill();
          }
          _LockedBackgroundTween = DOTween.Sequence();
          _LockedBackgroundTween.Append(_LockedBackgroundImage.DOFade(0, 0.2f));
        }
      }

      #endregion

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
          if (_CozmoScoreWidgetInstance == null) {
            _CozmoScoreWidgetInstance = CreateScoreWidget(_CozmoScoreContainer, _ScoreEnterAnimationXOffset,
              _CozmoPortraitSprite);            
          }
          return _CozmoScoreWidgetInstance;
        }
      }

      public void HideCozmoScoreboard() {
        HideWidget(_CozmoScoreWidgetInstance);
        _CozmoScoreWidgetInstance = null;
      }

      public ScoreWidget PlayerScoreboard {
        get {
          if (_PlayerScoreWidgetInstance == null) {
            _PlayerScoreWidgetInstance = CreateScoreWidget(_PlayerScoreContainer, -_ScoreEnterAnimationXOffset,
              _PlayerPortraitSprite);            
          }
          return _PlayerScoreWidgetInstance;
        }
      }

      public void HidePlayerScoreboard() {
        HideWidget(_PlayerScoreWidgetInstance);
        _PlayerScoreWidgetInstance = null;
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
        _QuitButtonInstance.DASEventViewController = ComposeDasViewName(_CurrentSlideName);
        _QuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;
      }

      public void ShowBackButton() {
        CreateWidgetIfNull<QuickQuitMinigameButton>(ref _QuickQuitButtonInstance, _QuickQuitGameButtonPrefab);
        _QuickQuitButtonInstance.DASEventViewController = ComposeDasViewName(_CurrentSlideName);
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
        _HowToPlayButtonInstance.Initialize(_HowToPlayContentLocKey, _HowToPlayContentPrefab, ComposeDasViewName(_CurrentSlideName));
      }

      public void HideHowToPlayButton() {
        if (_HowToPlayButtonInstance != null) {
          HideWidget(_HowToPlayButtonInstance);
          _HowToPlayButtonInstance = null;
        }
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

      public void ShowContinueButtonOffset(ContinueGameButtonWidget.ContinueButtonClickHandler buttonClickHandler,
                                           string buttonText, string shelfText, Color shelfColor, string dasButtonName) {
        if (_IsContinueButtonCentered) {
          if (_ContinueButtonInstance != null) {
            HideContinueButton();
          }
        }
        CreateWidgetIfNull<ContinueGameButtonWidget>(ref _ContinueButtonInstance, _ContinueButtonOffsetPrefab);
        _IsContinueButtonCentered = false;
        string dasViewControllerName = ComposeDasViewName(_CurrentSlideName);
        _ContinueButtonInstance.Initialize(buttonClickHandler, buttonText, shelfText, shelfColor, dasButtonName, dasViewControllerName);
        EnableContinueButton(true);
      }

      public void ShowContinueButtonCentered(ContinueGameButtonWidget.ContinueButtonClickHandler buttonClickHandler,
                                             string buttonText, string dasButtonName) {
        if (!_IsContinueButtonCentered) {
          if (_ContinueButtonInstance != null) {
            HideContinueButton();
          }
        }
        CreateWidgetIfNull<ContinueGameButtonWidget>(ref _ContinueButtonInstance, _ContinueButtonCenterPrefab);
        _IsContinueButtonCentered = true;
        string dasViewControllerName = ComposeDasViewName(_CurrentSlideName);
        _ContinueButtonInstance.Initialize(buttonClickHandler, buttonText, string.Empty, Color.clear, dasButtonName, dasViewControllerName);
        EnableContinueButton(true);
      }

      public void HideContinueButton() {
        HideWidget(_ContinueButtonInstance);
        _ContinueButtonInstance = null;
      }

      public void EnableContinueButton(bool enable) {
        if (_ContinueButtonInstance != null) {
          _ContinueButtonInstance.SetButtonInteractivity(enable);
        }
      }

      public void SetContinueButtonSupplementText(string text, Color color) {
        if (_ContinueButtonInstance != null) {
          _ContinueButtonInstance.SetShelfText(text, color);
        }
      }

      #endregion

      #region Shelf Widget

      public void ShowShelf() {
        CreateWidgetIfNull<ShelfWidget>(ref _ShelfWidgetInstance, _ShelfWidgetPrefab);
      }

      public void HideShelf() {
        HideWidget(_ShelfWidgetInstance);
        _ShelfWidgetInstance = null;
      }

      #endregion

      #region Info Title Text

      #endregion

      #region Game State Slides

      public ShowCozmoCubeSlide ShowCozmoCubesSlide(int numCubesRequired, TweenCallback endInTweenCallback = null) {
        GameObject slideObject = ShowWideGameStateSlide(MinigameUIPrefabHolder.Instance.InitialCubesSlide, "setup_cubes_slide", endInTweenCallback);
        ShowCozmoCubeSlide cubeSlide = slideObject.GetComponent<ShowCozmoCubeSlide>();
        cubeSlide.Initialize(numCubesRequired, Cozmo.CubePalette.InViewColor);
        return cubeSlide;
      }

      public void ShowWideAnimationSlide(string descLocKey, string slideDasName, GameObject animationPrefab, TweenCallback endInTweenCallback) {
        GameObject slide = ShowWideGameStateSlide(_AnimationSlidePrefab.gameObject, slideDasName, endInTweenCallback);
        AnimationSlide animationSlide = slide.GetComponent<AnimationSlide>();
        animationSlide.Initialize(animationPrefab, descLocKey);
      }

      public void ShowWideSlideWithText(string descLocKey, TweenCallback endInTweenCallback) {
        GameObject slide = ShowWideGameStateSlide(_InfoTextSlidePrefab.gameObject, "wide_info_slide_" + descLocKey, endInTweenCallback);
        Anki.UI.AnkiTextLabel textLabel = slide.GetComponent<Anki.UI.AnkiTextLabel>();
        textLabel.text = Localization.Get(descLocKey);
      }

      public GameObject ShowWideGameStateSlide(GameObject prefab, string slideDasName, TweenCallback endInTweenCallback = null) {
        if (slideDasName == _CurrentSlideName) {
          return _CurrentSlide.gameObject;
        }
        InfoTitleText = null;
        HideGameStateSlide();
        HidePlayerScoreboard();
        HideCozmoScoreboard();
        return ShowGameStateSlide(slideDasName, prefab, _WideGameSlideContainer, endInTweenCallback);
      }

      public GameObject ShowNarrowGameStateSlide(GameObject prefab, string slideDasName, TweenCallback endInTweenCallback = null) {
        if (slideDasName == _CurrentSlideName) {
          return _CurrentSlide.gameObject;
        }
        InfoTitleText = null;
        HideGameStateSlide();
        return ShowGameStateSlide(slideDasName, prefab, _NarrowGameSlideContainer, endInTweenCallback);
      }

      public string InfoTitleText {
        get { return _InfoTitleTextLabel.text; }
        set { 
          _InfoTitleLayoutElement.gameObject.SetActive(!string.IsNullOrEmpty(value));
          _InfoTitleTextLabel.text = value; 
        }
      }

      public void ShowInfoTextSlideWithKey(string localizationKey, TweenCallback endInTweenCallback = null) {
        GameObject slide = ShowGameStateSlide("info_slide_" + localizationKey, _InfoTextSlidePrefab.gameObject, 
                             _InfoTextGameSlideContainer, endInTweenCallback);
        Anki.UI.AnkiTextLabel textLabel = slide.GetComponent<Anki.UI.AnkiTextLabel>();
        textLabel.text = Localization.Get(localizationKey);
      }

      public void HideGameStateSlide() {
        if (_CurrentSlide != null) {
          // Clean up the current out slide
          if (_TransitionOutSlide != null) {
            Destroy(_TransitionOutSlide.gameObject);
          }

          // Send a das event
          DAS.Event("ui.slide.exit", _CurrentSlideName);

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
                                            RectTransform slideContainer, TweenCallback endInTweenCallback = null) {
        if (slideName == _CurrentSlideName) {
          return _CurrentSlide.gameObject;
        }

        // If a slide already exists, play a transition out tween on it
        HideGameStateSlide();

        _CurrentSlideName = slideName;

        // Update das
        DAS.Event("ui.slide.enter", _CurrentSlideName);
        UpdateButtonDasViewControllerNames(_CurrentSlideName);

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
        if (endInTweenCallback != null) {
          _SlideInTween.AppendCallback(endInTweenCallback);
        }
        _SlideInTween.Play();

        return newSlideObj;
      }

      #endregion

      #region Difficulty Select Widget

      public DifficultySelectButtonPanel ShowDifficultySelectButtons(List<DifficultySelectOptionData> options, int highestDifficultyAvailable,
                                                                     TweenCallback endInTweenCallback) {
        if (_DifficultySelectButtonPanelInstance == null) {
          GameObject difficultySlide = ShelfWidget.AddContent(_DifficultySelectButtonPanelPrefab, endInTweenCallback);
          _DifficultySelectButtonPanelInstance = difficultySlide.GetComponent<DifficultySelectButtonPanel>();
          _DifficultySelectButtonPanelInstance.Initialize(options, highestDifficultyAvailable);
        }
        return _DifficultySelectButtonPanelInstance;
      }

      public void HideDifficultySelectButtonPanel() {
        ShelfWidget.HideContent();
        _DifficultySelectButtonPanelInstance = null;
      }

      public DifficultySelectOptionData GetSelectedDifficulty() {
        if (_DifficultySelectButtonPanelInstance != null) {
          return _DifficultySelectButtonPanelInstance.SelectedDifficulty;
        }
        return null;
      }

      #endregion
    }
  }
}