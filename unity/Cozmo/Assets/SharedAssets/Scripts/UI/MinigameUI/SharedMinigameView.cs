using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using DG.Tweening;
using System.Collections.Generic;

namespace Cozmo {
  namespace MinigameWidgets {
    public class SharedMinigameView : BaseView {

      public enum ContentLayer {
        Bottom,
        Middle,
        Overlay
      }

      public delegate void SharedMinigameViewHandler();

      public event SharedMinigameViewHandler QuitMiniGameConfirmed;

      [SerializeField]
      private RectTransform _BottomWidgetContainer;

      [SerializeField]
      private RectTransform _MiddleWidgetContainer;

      [SerializeField]
      private RectTransform _OverlayWidgetContainer;

      #region Backgrounds

      [SerializeField]
      private Color _NormalBackgroundCaratColor;

      [SerializeField]
      private Color _LockedBackgroundCaratColor;

      [SerializeField]
      private Image _LockedBackgroundImage;

      private bool _IsShowingLocked = false;
      private Sequence _LockedBackgroundTween;

      [SerializeField]
      private Image _BackgroundGradient;

      [SerializeField]
      private Image _MiddleBackgroundImage;

      private bool _IsShowingMiddle = false;
      private Sequence _MiddleBackgroundTween;

      [SerializeField]
      private Image _OverlayBackgroundImage;

      private bool _IsShowingOverlay = false;
      private Sequence _OverlayBackgroundTween;

      #endregion

      #region Top widgets

      [SerializeField]
      private QuitMinigameButton _QuitGameButtonPrefab;

      private QuitMinigameButton _QuitButtonInstance;

      [SerializeField]
      private BackButton _BackButtonPrefab;
      private BackButton _BackButtonInstance;

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
      private HowToPlayButton _InstructionsVideoButtonPrefab;
      private HowToPlayButton _InstructionsVideoButtonInstance;

      [SerializeField]
      private ShelfWidget _ShelfWidgetPrefab;
      private ShelfWidget _ShelfWidgetInstance;

      public ShelfWidget ShelfWidget {
        get {
          ShowShelf();
          return _ShelfWidgetInstance;
        }
      }

      [SerializeField]
      private ShelfWidget _TallShelfWidgetPrefab;

      [SerializeField]
      private SpinnerWidget _SpinnerWidgetPrefab;
      private SpinnerWidget _SpinnerWidgetInstance;

      [SerializeField]
      private RectTransform _BannerContainer;

      [SerializeField]
      private Banner _BannerWidgetPrefab;
      private Banner _BannerWidgetInstance;

      #endregion

      #region Difficulty Select

      [SerializeField]
      private DifficultySelectButtonPanel _DifficultySelectButtonPanelPrefab;
      private DifficultySelectButtonPanel _DifficultySelectButtonPanelInstance;

      #endregion

      #region Slide System

      [SerializeField]
      private RectTransform _FullScreenGameSlideContainer;

      [SerializeField]
      private RectTransform _WideGameSlideContainer;

      [SerializeField]
      private RectTransform _NarrowGameSlideContainer;

      [SerializeField]
      private LayoutElement _InfoTitleLayoutElement;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _InfoTitleTextLabel;

      [SerializeField]
      private LayoutElement _InfoTextSlideLayoutElement;

      [SerializeField]
      private RectTransform _InfoTextGameSlideContainer;

      private float _SlideXOffset = 100f;

      #endregion

      #region Default Slides

      [SerializeField]
      private Anki.UI.AnkiTextLabel _InfoTextSlidePrefab;

      [SerializeField]
      private AnimationSlide _AnimationSlidePrefab;

      [SerializeField]
      private GameObject _ShowCozmoCubesSlidePrefab;

      [SerializeField]
      private ChallengeEndedDialog _ChallengeEndViewPrefab;

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

      public Sprite CozmoPortrait {
        get { return _CozmoPortraitSprite; }
      }

      [SerializeField]
      private Sprite _PlayerPortraitSprite;

      public Sprite PlayerPortrait {
        get { return _PlayerPortraitSprite; }
      }

      #endregion

      [SerializeField]
      private WinnerStateSlide _WinnerStateSlidePrefab;

      [SerializeField]
      private ShowCozmoVideo _ShowCozmoVideoPrefab;
      private ShowCozmoVideo _ShowCozmoVideoInstance;

      private CanvasGroup _CurrentSlide;
      private string _CurrentSlideName;
      private Sequence _SlideInTween;
      private CanvasGroup _TransitionOutSlide;
      private Sequence _SlideOutTween;
      private Dictionary<int, Sequence> _HideWidgetSequences = new Dictionary<int, Sequence>();
      private Dictionary<int, Sequence> _AddWidgetSequences = new Dictionary<int, Sequence>();

      private List<MinigameWidget> _ActiveWidgets = new List<MinigameWidget>();

      public CanvasGroup CurrentSlide { get { return _CurrentSlide; } }

      private bool _OpenAnimationStarted = false;

      private bool _IsMinigame = true;

      private ChallengeData _ChallengeData;

      public void Initialize(ChallengeData data) {
        HideNarrowInfoTextSlide();
        _IsMinigame = data.IsMinigame;
        _ChallengeData = data;
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
          _CurrentSlide = null;
        }
        if (_TransitionOutSlide != null) {
          Destroy(_TransitionOutSlide.gameObject);
          _TransitionOutSlide = null;
        }

        if (_LockedBackgroundTween != null) {
          _LockedBackgroundTween.Kill();
        }
        if (_MiddleBackgroundTween != null) {
          _MiddleBackgroundTween.Kill();
        }
        if (_OverlayBackgroundTween != null) {
          _OverlayBackgroundTween.Kill();
        }

        foreach (KeyValuePair<int, Sequence> kvp in _HideWidgetSequences) {
          kvp.Value.Kill();
        }
        _HideWidgetSequences.Clear();

        foreach (KeyValuePair<int, Sequence> kvp in _AddWidgetSequences) {
          kvp.Value.Kill();
        }
        _AddWidgetSequences.Clear();
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
        float fadeOutSeconds = UIDefaultTransitionSettings.Instance.FadeOutTransitionDurationSeconds;
        Ease fadeOutEasing = UIDefaultTransitionSettings.Instance.FadeOutEasing;

        Sequence close;
        foreach (MinigameWidget widget in _ActiveWidgets) {
          close = widget.CreateCloseAnimSequence();
          if (close != null) {
            closeAnimation.Join(close);
          }
        }
        closeAnimation = JoinFadeTween(closeAnimation, null, _BackgroundGradient, 0f, fadeOutSeconds, fadeOutEasing);
        closeAnimation = JoinFadeTween(closeAnimation, _LockedBackgroundTween, _LockedBackgroundImage, 0f, fadeOutSeconds, fadeOutEasing);
        closeAnimation = JoinFadeTween(closeAnimation, _MiddleBackgroundTween, _MiddleBackgroundImage, 0f, fadeOutSeconds, fadeOutEasing);
        closeAnimation = JoinFadeTween(closeAnimation, _OverlayBackgroundTween, _OverlayBackgroundImage, 0f, fadeOutSeconds, fadeOutEasing);

      }

      private Sequence JoinFadeTween(Sequence sequenceToUse, Tween tween, Image targetImage, float targetAlpha,
                                float duration, Ease easing) {
        if (tween != null) {
          tween.Kill();
        }
        sequenceToUse.Join(targetImage.DOFade(targetAlpha, duration).SetEase(easing));
        return sequenceToUse;
      }

      #endregion

      private void Awake() {
        // Hide the info title by default. This also centers game state slides
        // by default.
        _InfoTitleLayoutElement.gameObject.SetActive(false);
      }

      private bool CreateWidgetIfNull<T>(ref T widgetInstance, MonoBehaviour widgetPrefab, ContentLayer layer = ContentLayer.Overlay) where T : MinigameWidget {
        if (widgetInstance != null) {
          return false;
        }
        if (this == null) {
          DAS.Warn("SharedMinigameView.CreateWidgetIfNull", "Attempted to create a widget with a NULL SharedMinigameView");
          return false;
        }

        Transform parentTransform = this.transform;
        switch (layer) {
        case ContentLayer.Bottom:
          parentTransform = _BottomWidgetContainer;
          break;
        case ContentLayer.Middle:
          parentTransform = _MiddleWidgetContainer;
          break;
        case ContentLayer.Overlay:
          parentTransform = _OverlayWidgetContainer;
          break;
        default:
          break;
        }

        GameObject newWidget = UIManager.CreateUIElement(widgetPrefab.gameObject, parentTransform);
        widgetInstance = newWidget.GetComponent<T>();
        AddWidget(widgetInstance);

        return true;
      }

      private void AddWidget(MinigameWidget widgetToAdd) {
        _ActiveWidgets.Add(widgetToAdd);

        if (_OpenAnimationStarted) {

          int widgetInstanceId = widgetToAdd.gameObject.GetInstanceID();

          if (!_AddWidgetSequences.ContainsKey(widgetInstanceId)) {

            // if the hide widget seqeunce for this id is still playing then
            // we need to kill that first.
            if (_HideWidgetSequences.ContainsKey(widgetInstanceId)) {
              _HideWidgetSequences[widgetInstanceId].Kill();
              _HideWidgetSequences.Remove(widgetInstanceId);
            }

            Sequence openAnimation = widgetToAdd.CreateOpenAnimSequence();
            openAnimation.AppendCallback(() => {
              _AddWidgetSequences.Remove(widgetInstanceId);
            });
            openAnimation.Play();
            _AddWidgetSequences.Add(widgetInstanceId, openAnimation);
          }
        }

      }

      private void HideWidget(MinigameWidget widgetToHide) {
        if (widgetToHide == null) {
          return;
        }

        _ActiveWidgets.Remove(widgetToHide);

        int widgetInstanceId = widgetToHide.gameObject.GetInstanceID();

        if (!_HideWidgetSequences.ContainsKey(widgetInstanceId)) {

          // if the add widget seqeunce for this id is still playing then
          // we need to kill that first.
          if (_AddWidgetSequences.ContainsKey(widgetInstanceId)) {
            _AddWidgetSequences[widgetInstanceId].Kill();
            _AddWidgetSequences.Remove(widgetInstanceId);
          }

          Sequence close = widgetToHide.CreateCloseAnimSequence();
          close.AppendCallback(() => {
            _HideWidgetSequences.Remove(widgetInstanceId);
            if (widgetToHide != null) {
              widgetToHide.DestroyWidgetImmediately();
            }
          });
          close.Play();
          _HideWidgetSequences.Add(widgetInstanceId, close);

        }

      }

      private void UpdateButtonDasViewControllerNames(string slideName) {
        string currentViewName = ComposeDasViewName(slideName);
        if (_QuitButtonInstance != null) {
          _QuitButtonInstance.DASEventViewController = currentViewName;
        }

        if (_BackButtonInstance != null) {
          _BackButtonInstance.DASEventViewController = currentViewName;
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
        _BackgroundGradient.color = baseColor;

        Color transparentBaseColor = new Color(baseColor.r, baseColor.g, baseColor.b, 0);

        _OverlayBackgroundImage.color = transparentBaseColor;
        _IsShowingOverlay = false;

        Color lockedColor = _LockedBackgroundImage.color;
        lockedColor.a = 0;
        _LockedBackgroundImage.color = lockedColor;
        _IsShowingLocked = false;

        Color setupColor = UIColorPalette.GameSetupColor;
        setupColor.a = 0;
        _MiddleBackgroundImage.color = setupColor;
        _IsShowingMiddle = false;

        UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe, Color.white);
      }

      public void ShowLockedBackground() {
        if (_ShelfWidgetInstance != null) {
          _ShelfWidgetInstance.CaratColor = _LockedBackgroundCaratColor;
        }
        ShowBackground(ref _IsShowingLocked, ref _LockedBackgroundTween, _LockedBackgroundImage);
      }

      public void HideLockedBackground() {
        if (_ShelfWidgetInstance != null) {
          _ShelfWidgetInstance.CaratColor = _NormalBackgroundCaratColor;
        }
        HideBackground(ref _IsShowingLocked, ref _LockedBackgroundTween, _LockedBackgroundImage);
      }

      // Make Title Text use SetupTextColor rather than base TextColor with MiddleBackground
      public void ShowMiddleBackground() {
        if (_TitleWidgetInstance != null) {
          _TitleWidgetInstance.TitleTextColor = UIColorPalette.SetupTextColor;
        }
        ShowBackground(ref _IsShowingMiddle, ref _MiddleBackgroundTween, _MiddleBackgroundImage);
      }

      // Revert Title Text to MinigameTitle Color when hiding MiddleBackground
      public void HideMiddleBackground() {
        if (_TitleWidgetInstance != null) {
          _TitleWidgetInstance.TitleTextColor = UIColorPalette.MinigameTextColor;
        }
        HideBackground(ref _IsShowingMiddle, ref _MiddleBackgroundTween, _MiddleBackgroundImage);
      }

      public void ShowOverlayBackground() {
        ShowBackground(ref _IsShowingOverlay, ref _OverlayBackgroundTween, _OverlayBackgroundImage);
      }

      public void HideOverlayBackground() {
        HideBackground(ref _IsShowingOverlay, ref _OverlayBackgroundTween, _OverlayBackgroundImage);
      }

      public void ShowBackground(ref bool currentlyShowing, ref Sequence sequence, Image targetImage) {
        if (!currentlyShowing) {
          currentlyShowing = true;
          PlayFadeTween(ref sequence, targetImage, 1,
                        UIDefaultTransitionSettings.Instance.FadeInTransitionDurationSeconds,
                        UIDefaultTransitionSettings.Instance.FadeInEasing);
        }
      }

      public void HideBackground(ref bool currentlyShowing, ref Sequence sequence, Image targetImage) {
        if (currentlyShowing) {
          currentlyShowing = false;
          PlayFadeTween(ref sequence, targetImage, 0,
                        UIDefaultTransitionSettings.Instance.FadeOutTransitionDurationSeconds,
                        UIDefaultTransitionSettings.Instance.FadeOutEasing);
        }
      }

      private void PlayFadeTween(ref Sequence sequenceToUse, Image targetImage, float targetAlpha,
                                float duration, Ease easing) {
        if (sequenceToUse != null) {
          sequenceToUse.Kill();
        }
        sequenceToUse = DOTween.Sequence();
        sequenceToUse.Append(targetImage.DOFade(targetAlpha, duration).SetEase(easing));
      }

      #endregion

      #region Challenge Title Widget

      public ChallengeTitleWidget TitleWidget {
        get {
          CreateWidgetIfNull<ChallengeTitleWidget>(ref _TitleWidgetInstance, _TitleWidgetPrefab);
          return _TitleWidgetInstance;
        }
      }

      public void HideTitleWidget() {
        HideWidget(_TitleWidgetInstance);
        _TitleWidgetInstance = null;
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
                                                          _CozmoPortraitSprite, false);
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
                                                           _PlayerPortraitSprite, true);
          }
          return _PlayerScoreWidgetInstance;
        }
      }

      public void HidePlayerScoreboard() {
        HideWidget(_PlayerScoreWidgetInstance);
        _PlayerScoreWidgetInstance = null;
      }

      private ScoreWidget CreateScoreWidget(RectTransform widgetParent, float animationOffset,
                                            Sprite portrait, bool isPlayer) {
        GameObject widgetObj = UIManager.CreateUIElement(_ScoreWidgetPrefab.gameObject, widgetParent);
        ScoreWidget instance = widgetObj.GetComponent<ScoreWidget>();
        instance.AnimationXOffset = animationOffset;
        instance.Portrait = portrait;
        instance.IsWinner = false;
        instance.IsPlayer = isPlayer;
        instance.Dim = false;

        AddWidget(instance);
        return instance;
      }

      #endregion

      #region Quit Button

      public bool IsQuitAlertViewOpen() {
        if (_QuitButtonInstance != null) {
          return _QuitButtonInstance.IsQuitAlertViewOpen();
        }
        return false;
      }

      public void ShowQuitButton() {
        HideBackButton();
        CreateWidgetIfNull<QuitMinigameButton>(ref _QuitButtonInstance, _QuitGameButtonPrefab, ContentLayer.Middle);
        _QuitButtonInstance.Initialize(_IsMinigame);
        _QuitButtonInstance.DASEventViewController = ComposeDasViewName(_CurrentSlideName);
        _QuitButtonInstance.QuitGameConfirmed += HandleQuitConfirmed;
      }

      public void ShowBackButton(BackButton.BackButtonHandler backTapped) {
        HideQuitButton();
        CreateWidgetIfNull<BackButton>(ref _BackButtonInstance, _BackButtonPrefab, ContentLayer.Middle);
        _BackButtonInstance.DASEventViewController = ComposeDasViewName(_CurrentSlideName);
        _BackButtonInstance.HandleBackTapped = new BackButton.BackButtonHandler(backTapped);
      }

      public void HideBackButton() {
        HideWidget(_BackButtonInstance);
        _BackButtonInstance = null;
      }

      public void HideQuitButton() {
        if (_QuitButtonInstance != null) {
          _QuitButtonInstance.QuitGameConfirmed -= HandleQuitConfirmed;
          HideWidget(_QuitButtonInstance);
          _QuitButtonInstance = null;
        }
        else {
          DAS.Warn(this, "HideQuitButton - _QuitButtonInstance is NULL");
        }
      }

      public void HandleQuitConfirmed() {
        HideGameStateSlide();
        HideInstructionsVideoButton();
        if (QuitMiniGameConfirmed != null) {
          QuitMiniGameConfirmed();
        }
      }

      #endregion

      #region HowToPlay

      public void ShowInstructionsVideoButton() {
        CreateWidgetIfNull<HowToPlayButton>(ref _InstructionsVideoButtonInstance, _InstructionsVideoButtonPrefab);
        _InstructionsVideoButtonInstance.OnHowToPlayButtonClicked += () => {
          PlayVideo(_ChallengeData.InstructionVideoPath, null, true);
        };
      }

      public void HideInstructionsVideoButton() {
        HideWidget(_InstructionsVideoButtonInstance);
        _InstructionsVideoButtonInstance = null;
      }

      #endregion

      #region ContinueButtonShelfWidget

      public void ShowContinueButtonOffset(ContinueGameButtonWidget.ContinueButtonClickHandler buttonClickHandler,
                                           string buttonText, string shelfText, Color shelfTextColor, string dasButtonName) {
        if (_IsContinueButtonCentered) {
          if (_ContinueButtonInstance != null) {
            HideContinueButton();
          }
        }
        CreateWidgetIfNull<ContinueGameButtonWidget>(ref _ContinueButtonInstance, _ContinueButtonOffsetPrefab);
        _IsContinueButtonCentered = false;
        string dasViewControllerName = ComposeDasViewName(_CurrentSlideName);
        _ContinueButtonInstance.Initialize(buttonClickHandler, buttonText, shelfText, shelfTextColor, dasButtonName, dasViewControllerName);
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

      public void ShowTallShelf(bool showTall) {
        if (_ShelfWidgetInstance != null) {
          HideShelf();
        }
        if (showTall) {
          CreateWidgetIfNull<ShelfWidget>(ref _ShelfWidgetInstance, _TallShelfWidgetPrefab);
        }
        else {
          ShowShelf();
        }
      }

      public void HideShelf() {
        HideWidget(_ShelfWidgetInstance);
        _ShelfWidgetInstance = null;
      }

      #endregion

      #region Game State Slides

      public ShowCozmoCubeSlide ShowCozmoCubesSlide(int numCubesRequired, TweenCallback endInTweenCallback = null) {
        GameObject slideObject = ShowWideGameStateSlide(_ShowCozmoCubesSlidePrefab, "setup_cubes_slide", endInTweenCallback);
        ShowCozmoCubeSlide cubeSlide = slideObject.GetComponent<ShowCozmoCubeSlide>();
        cubeSlide.Initialize(numCubesRequired, CubePalette.Instance.InViewColor, CubePalette.Instance.OutOfViewColor);
        return cubeSlide;
      }

      public WinnerStateSlide ShowWinnerStateSlide(bool playerWon, string centerText, string footerText) {
        HideQuitButton();
        GameObject winnerStateEndSlide = ShowNarrowGameStateSlide(
          _WinnerStateSlidePrefab.gameObject, "Winnerstate_end_slide");
        WinnerStateSlide winnerStateSlideScript = winnerStateEndSlide.GetComponent<WinnerStateSlide>();
        winnerStateSlideScript.Init(playerWon ? _PlayerPortraitSprite : _CozmoPortraitSprite, centerText, footerText);
        return winnerStateSlideScript;
      }

      public ChallengeEndedDialog ShowChallengeEndedSlide(ChallengeData data) {
        HideQuitButton();
        GameObject challengeEndSlide = ShowWideGameStateSlide(
          _ChallengeEndViewPrefab.gameObject, "challenge_end_slide");
        ChallengeEndedDialog challengeEndSlideScript = challengeEndSlide.GetComponent<ChallengeEndedDialog>();
        challengeEndSlideScript.SetupDialog(data);
        return challengeEndSlideScript;
      }

      public void ShowWideAnimationSlide(string descLocKey, string slideDasName, GameObject animationPrefab, TweenCallback endInTweenCallback, string headerLocKey = null) {
        GameObject slide = ShowWideGameStateSlide(_AnimationSlidePrefab.gameObject, slideDasName, endInTweenCallback);
        AnimationSlide animationSlide = slide.GetComponent<AnimationSlide>();
        animationSlide.Initialize(animationPrefab, descLocKey, headerLocKey);
      }

      public void ShowWideSlideWithText(string descLocKey, TweenCallback endInTweenCallback) {
        GameObject slide = ShowWideGameStateSlide(_InfoTextSlidePrefab.gameObject, "wide_info_slide_" + descLocKey, endInTweenCallback);
        Anki.UI.AnkiTextLabel textLabel = slide.GetComponent<Anki.UI.AnkiTextLabel>();
        textLabel.text = Localization.Get(descLocKey);
      }

      public GameObject ShowFullScreenGameStateSlide(GameObject prefab, string slideDasName, TweenCallback endInTweenCallback = null) {
        if (slideDasName == _CurrentSlideName) {
          return _CurrentSlide.gameObject;
        }
        InfoTitleText = null;
        HideGameStateSlide();
        HidePlayerScoreboard();
        HideCozmoScoreboard();
        HideShelf();
        HideContinueButton();
        return ShowGameStateSlide(slideDasName, prefab, _FullScreenGameSlideContainer, endInTweenCallback, 0f);
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

      public void ShowNarrowInfoTextSlideWithKey(string localizationKey, TweenCallback endInTweenCallback = null) {
        _InfoTextSlideLayoutElement.gameObject.SetActive(true);
        GameObject slide = ShowGameStateSlide("info_slide_" + localizationKey, _InfoTextSlidePrefab.gameObject,
                             _InfoTextGameSlideContainer, endInTweenCallback);
        Anki.UI.AnkiTextLabel textLabel = slide.GetComponent<Anki.UI.AnkiTextLabel>();
        textLabel.text = Localization.Get(localizationKey);
      }

      public void HideNarrowInfoTextSlide() {
        _InfoTextSlideLayoutElement.gameObject.SetActive(false);
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
            -_SlideXOffset, 0.25f).SetEase(Ease.OutQuad).SetRelative());
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
                                            RectTransform slideContainer, TweenCallback endInTweenCallback = null, float transitionXOffset = 100f) {
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
        _SlideXOffset = transitionXOffset;
        _SlideInTween = DOTween.Sequence();
        _SlideInTween.Append(_CurrentSlide.transform.DOLocalMoveX(
          _SlideXOffset, 0.25f).From().SetEase(Ease.OutQuad).SetRelative());
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
                                                                     TweenCallback endInTweenCallback, int selectedDifficulty) {
        if (_DifficultySelectButtonPanelInstance == null) {
          GameObject difficultySlide = ShelfWidget.SetShelfContent(_DifficultySelectButtonPanelPrefab, endInTweenCallback);
          _DifficultySelectButtonPanelInstance = difficultySlide.GetComponent<DifficultySelectButtonPanel>();
          _DifficultySelectButtonPanelInstance.Initialize(options, highestDifficultyAvailable, selectedDifficulty);
        }
        return _DifficultySelectButtonPanelInstance;
      }

      public void HideDifficultySelectButtonPanel() {
        ShelfWidget.HideContent();
        _DifficultySelectButtonPanelInstance = null;
      }

      #endregion

      public void ShowSpinnerWidget() {
        CreateWidgetIfNull<SpinnerWidget>(ref _SpinnerWidgetInstance, _SpinnerWidgetPrefab);
      }

      public void HideSpinnerWidget() {
        HideWidget(_SpinnerWidgetInstance);
        _SpinnerWidgetInstance = null;
      }

      public void PlayBannerAnimation(string textToDisplay, TweenCallback animationEndCallback = null, float customSlowDurationSeconds = 0f, bool playSound = true) {
        if (_BannerWidgetInstance == null) {
          _BannerWidgetInstance = UIManager.CreateUIElement(_BannerWidgetPrefab.gameObject, _BannerContainer).GetComponent<Banner>();
        }
        _BannerWidgetInstance.PlayBannerAnimation(textToDisplay, animationEndCallback, customSlowDurationSeconds, playSound);
      }

      public void PlayVideo(string videoPath, System.Action onVideoContinue, bool showSkipButton = false) {
        _ShowCozmoVideoInstance = GameObject.Instantiate(_ShowCozmoVideoPrefab.gameObject).GetComponent<ShowCozmoVideo>();
        _ShowCozmoVideoInstance.ShowSkipButton(showSkipButton);
        _ShowCozmoVideoInstance.PlayVideo(videoPath);
        _ShowCozmoVideoInstance.transform.SetParent(transform, false);
        _ShowCozmoVideoInstance.OnContinueButton += onVideoContinue;
        _ShowCozmoVideoInstance.OnContinueButton += DestroyVideo;
      }

      private void DestroyVideo() {
        if (_ShowCozmoVideoInstance != null) {
          GameObject.Destroy(_ShowCozmoVideoInstance.gameObject);
        }
      }
    }
  }
}