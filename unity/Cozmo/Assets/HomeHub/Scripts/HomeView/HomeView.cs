using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using Anki.Assets;
using Cozmo.UI;
using DataPersistence;
using System;
using DG.Tweening;

namespace Cozmo.HomeHub {
  public class HomeView : BaseView {

    public enum HomeTab {
      Cozmo,
      Play,
      Profile
    }

    public System.Action<StatContainer, StatContainer, Transform[]> DailyGoalsSet;

    public Action<string> MinigameConfirmed;
    public Action<string> OnTabChanged;

    [SerializeField]
    private HomeViewTab _CozmoTabPrefab;

    [SerializeField]
    private HomeViewTab _PlayTabPrefab;

    [SerializeField]
    private HomeViewTab _ProfileTabPrefab;

    private HomeViewTab _CurrentTab;

    [SerializeField]
    private CozmoButton _CozmoTabButton;
    [SerializeField]
    private Sprite _CozmoTabUpSpritePlay;
    [SerializeField]
    private Sprite _CozmoTabUpSpriteProfile;

    [SerializeField]
    private CozmoButton _CozmoTabDownButton;

    [SerializeField]
    private GameObject _AnyUpgradeAffordableIndicator;

    [SerializeField]
    private CozmoButton _PlayTabButton;
    [SerializeField]
    private Sprite _PlayTabUpSpriteCozmo;
    [SerializeField]
    private Sprite _PlayTabUpSpriteProfile;

    [SerializeField]
    private CozmoButton _PlayTabDownButton;

    [SerializeField]
    private CozmoButton _ProfileTabButton;
    [SerializeField]
    private Sprite _ProfileTabUpSpritePlay;
    [SerializeField]
    private Sprite _ProfileTabUpSpriteCozmo;

    [SerializeField]
    private CozmoButton _ProfileTabDownButton;

    [SerializeField]
    private RectTransform _ScrollRectContent;

    [SerializeField]
    private Cozmo.UI.ProgressBar _RequirementPointsProgressBar;
    [SerializeField]
    private Image _EmotionChipTag;
    [SerializeField]
    private Sprite _EmotionChipSprite_Empty;
    [SerializeField]
    private Sprite _EmotionChipSprite_Mid;
    [SerializeField]
    private Sprite _EmotionChipSprite_Full;

    [SerializeField]
    private Text _CurrentRequirementPointsLabel;

    [SerializeField]
    private ScrollRect _ScrollRect;

    [SerializeField]
    private GameObjectDataLink _LootViewPrefabData;

    private LootView _LootViewInstance = null;

    public bool RewardSequenceActive = false;

    [SerializeField]
    private GameObject _EnergyDooberPrefab;
    [SerializeField]
    private RectTransform _EnergyDooberStart;
    [SerializeField]
    private Transform _EnergyDooberEnd;

    [SerializeField]
    private AnkiTextLabel _DailyGoalsCompletionText;

    [SerializeField]
    private AnkiTextLabel _DailyGaolsCompletionTextDown;

    [SerializeField]
    private ParticleSystem _EnergyBarEmitter;

    [SerializeField]
    private CanvasGroup _TabContentContainer;

    [SerializeField]
    private float _TabContentAnimationXOriginOffset;

    [SerializeField]
    private Ease _TabContentOpenEase = Ease.OutBack;

    [SerializeField]
    private RectTransform _TabButtonContainer;

    [SerializeField]
    private float _TabButtonAnimationXOriginOffset;

    [SerializeField]
    private RectTransform _TopBarContainer;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _HexLabel;

    [SerializeField]
    private float _TopBarAnimationYOriginOffset;

    [SerializeField]
    private Ease _TopBarOpenEase = Ease.OutBack;

    [SerializeField]
    private Ease _TopBarCloseEase = Ease.InBack;

    [SerializeField]
    private Cozmo.UI.CozmoButton _HelpButton;

    [SerializeField]
    private BaseView _HelpViewPrefab;
    private BaseView _HelpViewInstance;

    [SerializeField]
    private Transform _OnboardingTransform;

    private AlertView _RequestDialog = null;

    private HomeHub _HomeHubInstance;

    public HomeHub HomeHubInstance {
      get { return _HomeHubInstance; }
      private set { _HomeHubInstance = value; }
    }

    public Transform TabContentContainer {
      get { return _TabContentContainer.transform; }
    }
    public bool HomeViewCurrentlyOccupied {
      get {
        return (_RequestDialog != null || _LootViewInstance != null || _HomeHubInstance.IsChallengeDetailsActive || RewardSequenceActive);
      }
    }
    public Transform TabButtonContainer {
      get { return _TabButtonContainer.transform; }
    }
    public Transform TopBarContainer {
      get { return _TopBarContainer.transform; }
    }

    public delegate void ButtonClickedHandler(string challengeClicked, Transform buttonTransform);

    public event ButtonClickedHandler OnUnlockedChallengeClicked;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStates;

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById, HomeHub homeHubInstance) {
      _HomeHubInstance = homeHubInstance;

      DASEventViewName = "home_view";

      _ChallengeStates = challengeStatesById;
      HandlePlayTabButton();
      UpdatePuzzlePieceCount();

      _CozmoTabButton.Initialize(HandleCozmoTabButton, "switch_to_cozmo_tab_button", DASEventViewName);
      _PlayTabButton.Initialize(HandlePlayTabButton, "switch_to_play_tab_button", DASEventViewName);
      _ProfileTabButton.Initialize(HandleProfileTabButton, "switch_to_profile_tab_button", DASEventViewName);
      _HelpButton.Initialize(HandleHelpButton, "help_button", DASEventViewName);

      _CozmoTabDownButton.Initialize(HandleCozmoTabButton, "switch_to_cozmo_tab_button", DASEventViewName);
      _PlayTabDownButton.Initialize(HandlePlayTabButton, "switch_to_play_tab_button", DASEventViewName);
      _ProfileTabDownButton.Initialize(HandleProfileTabButton, "switch_to_profile_tab_button", DASEventViewName);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);

      ChestRewardManager.Instance.ChestRequirementsGained += HandleChestRequirementsGained;
      ChestRewardManager.Instance.ChestGained += HandleChestGained;
      _RequirementPointsProgressBar.ProgressUpdateCompleted += HandleProgressUpdated;
      UnlockablesManager.Instance.OnUnlockPopupRequested += HandleUnlockView;
      // If we have energy earned, create the energy doobers and clear pending action rewards
      if (RewardedActionManager.Instance.RewardPending) {
        RewardSequenceActive = true;
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.NoGame);
        }
        int endPoints = ChestRewardManager.Instance.GetCurrentRequirementPoints();
        if (ChestRewardManager.Instance.ChestPending) {
          endPoints = ChestRewardManager.Instance.GetPreviousRequirementPoints();
        }
        endPoints -= Mathf.Min(endPoints, (RewardedActionManager.Instance.TotalPendingEnergy));
        if (endPoints < 0) {
          endPoints = 0;
        }
        UpdateChestProgressBar(endPoints, ChestRewardManager.Instance.GetNextRequirementPoints(), true);
        StartCoroutine(BurstAfterInit());
      }
      else {
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          DailyGoalManager.Instance.SetMinigameNeed();
        }
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints(), true);
      }
      DailyGoalManager.Instance.OnRefreshDailyGoals += UpdatePlayTabText;
      GameEventManager.Instance.OnGameEvent += HandleDailyGoalCompleted;
      UpdatePlayTabText();

      Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded += HandleItemValueChanged;
      playerInventory.ItemRemoved += HandleItemValueChanged;
      CheckIfUnlockablesAffordableAndUpdateBadge();

      // Checks if any stages are required...
      OnboardingManager.Instance.InitHomeHubOnboarding(this, _OnboardingTransform);
    }

    private IEnumerator BurstAfterInit() {
      yield return new WaitForFixedUpdate();
      EnergyDooberBurst(RewardedActionManager.Instance.TotalPendingEnergy);
    }

    private void HandleChestGained() {
      UpdateChestProgressBar(ChestRewardManager.Instance.GetPreviousRequirementPoints(), ChestRewardManager.Instance.GetPreviousRequirementPoints());
    }

    private void HandleUnlockView(Anki.Cozmo.UnlockId unlockID, bool showPopup) {
      // TODO: Make Tabs pass in information to allow for immediate Popups like former App Unlock
      // if showPopup, then pass info to the Tab.
      UnlockableInfo info = UnlockablesManager.Instance.GetUnlockableInfo(unlockID);
      if (info.UnlockableType == UnlockableType.Game) {
        HandlePlayTabButton();
      }
      else {
        HandleCozmoTabButton();
      }
      CheckIfUnlockablesAffordableAndUpdateBadge();
    }

    // Opens loot view and fires and relevant events
    private void OpenLootView() {
      if (HomeViewCurrentlyOccupied && RewardSequenceActive == false) {
        // Avoid dupes but fail gracefully
        DAS.Warn("HomeView.OpenLootView", "HomeViewCurrentlyOccupied with non reward stuff when we tried to open LootView");
        HandleLootViewCloseAnimationFinished();
        return;
      }
      _EmotionChipTag.gameObject.SetActive(false);

      AssetBundleManager.Instance.LoadAssetBundleAsync(_LootViewPrefabData.AssetBundle, (bool success) => {
        _LootViewPrefabData.LoadAssetData((GameObject prefabObject) => {
          LootView alertView = UIManager.OpenView(prefabObject.GetComponent<LootView>());
          alertView.LootBoxRewards = ChestRewardManager.Instance.PendingChestRewards;
          _LootViewInstance = alertView;
          _LootViewInstance.ViewCloseAnimationFinished += (() => {
            HandleLootViewCloseAnimationFinished();
            // Only unload the asset bundle if we actually loaded it before
            AssetBundleManager.Instance.UnloadAssetBundle(_LootViewPrefabData.AssetBundle);
          });
        });
      });
    }

    private void HandleLootViewCloseAnimationFinished() {
      _EmotionChipTag.gameObject.SetActive(true);
      RewardSequenceActive = false;
      _LootViewInstance = null;
      CheckIfUnlockablesAffordableAndUpdateBadge();
      UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints(), true);
    }

    // Doobstorm 2016 - Create Energy Doobers, set up Tween Sequence, and get it started
    public void EnergyDooberBurst(int pointsEarned) {
      GenericRewardsConfig rc = GenericRewardsConfig.Instance;
      int doobCount = Mathf.CeilToInt((float)pointsEarned / 5.0f);
      Sequence dooberSequence = DOTween.Sequence();
      for (int i = 0; i < doobCount; i++) {
        Transform freshDoobz = UIManager.CreateUIElement(_EnergyDooberPrefab, _EnergyDooberStart).transform;
        float xOffset = UnityEngine.Random.Range(rc.ExpParticleMinSpread, rc.ExpParticleMaxSpread);
        float yOffset = UnityEngine.Random.Range(rc.ExpParticleMinSpread, rc.ExpParticleMaxSpread);
        float exitTime = UnityEngine.Random.Range(0.0f, rc.ExpParticleStagger) + rc.ExpParticleBurst + rc.ExpParticleHold;
        Vector3 doobTarget = new Vector3(freshDoobz.position.x - xOffset, freshDoobz.position.y + yOffset, freshDoobz.position.z);
        dooberSequence.Insert(0.0f, freshDoobz.DOMove(doobTarget, rc.ExpParticleBurst).SetEase(Ease.OutBack));
        dooberSequence.Insert(exitTime, freshDoobz.DOMove(_EnergyDooberEnd.position, rc.ExpParticleLeave).SetEase(Ease.InBack).OnComplete(() => (CleanUpDoober(freshDoobz))));
      }
      dooberSequence.AppendCallback(ResolveDooberBurst);
      dooberSequence.Play();
    }

    private void CleanUpDoober(Transform toClean) {
      _EnergyBarEmitter.Emit(GenericRewardsConfig.Instance.BurstPerParticleHit);
      Destroy(toClean.gameObject);
    }

    private void ResolveDooberBurst() {
      RewardedActionManager.Instance.PendingActionRewards.Clear();
      if (ChestRewardManager.Instance.ChestPending == false) {
        RewardSequenceActive = false;
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.All);
        }
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());
      }
      else {
        HandleChestGained();
      }
    }

    private void HandleChestRequirementsGained(int currentPoints, int numPointsNeeded) {
      // Ignore updating if we are in the process of showing a new LootView
      if (ChestRewardManager.Instance.ChestPending == false) {
        UpdateChestProgressBar(currentPoints, numPointsNeeded);
      }
    }

    private void UpdateChestProgressBar(int currentPoints, int numPointsNeeded, bool instant = false) {
      float progress = ((float)currentPoints / (float)numPointsNeeded);
      _RequirementPointsProgressBar.SetProgress(progress, instant);
      _CurrentRequirementPointsLabel.text = currentPoints.ToString();
      if (progress <= 0.0f) {
        _EmotionChipTag.overrideSprite = _EmotionChipSprite_Empty;
      }
      else if (progress >= 1.0f) {
        _EmotionChipTag.overrideSprite = _EmotionChipSprite_Full;
      }
      else {
        _EmotionChipTag.overrideSprite = _EmotionChipSprite_Mid;
      }

    }

    // If we have a Chest Pending, open the loot view once the progress bar finishes filling.
    // If there are Rewards Pending, do this when the Energy Sequence ends.
    private void HandleProgressUpdated() {
      if (ChestRewardManager.Instance.ChestPending && _LootViewInstance == null
          && RewardedActionManager.Instance.RewardPending == false) {
        OpenLootView();
      }
      else {
        // Update Minigame need now that daily goal progress has changed
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          DailyGoalManager.Instance.SetMinigameNeed();
        }
      }
    }

    private void UpdatePuzzlePieceCount() {
      IEnumerable<string> puzzlePieceIds = Cozmo.HexItemList.GetPuzzlePieceIds();
      int totalNumberHexes = 0;
      Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      foreach (string puzzlePieceId in puzzlePieceIds) {
        totalNumberHexes += playerInventory.GetItemAmount(puzzlePieceId);
      }
      _HexLabel.FormattingArgs = new object[] { totalNumberHexes };
    }

    public void SetChallengeStates(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      _ChallengeStates = challengeStatesById;
    }

    private void HandleCozmoTabButton() {
      // Do not allow changing tabs while receiving chests
      if (HomeViewCurrentlyOccupied) {
        return;
      }
      ClearCurrentTab();
      ShowNewCurrentTab(_CozmoTabPrefab);
      UpdateTabGraphics(HomeTab.Cozmo);
    }

    private void HandlePlayTabButton() {
      ClearCurrentTab();
      ShowNewCurrentTab(_PlayTabPrefab);
      UpdateTabGraphics(HomeTab.Play);
    }

    private void HandleProfileTabButton() {
      // Do not allow changing tabs while receiving chests
      if (HomeViewCurrentlyOccupied) {
        return;
      }
      ClearCurrentTab();
      ShowNewCurrentTab(_ProfileTabPrefab);
      UpdateTabGraphics(HomeTab.Profile);
    }

    private void HandleHelpButton() {
      _HelpViewInstance = UIManager.OpenView(_HelpViewPrefab);
    }

    private void ClearCurrentTab() {
      if (_CurrentTab != null) {
        Destroy(_CurrentTab.gameObject);
      }
    }

    private void ShowNewCurrentTab(HomeViewTab homeViewTabPrefab) {
      _ScrollRect.horizontalNormalizedPosition = 0.0f;
      _CurrentTab = Instantiate(homeViewTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);

      if (OnTabChanged != null) {
        OnTabChanged(homeViewTabPrefab.name);
      }
    }

    private void UpdateTabGraphics(HomeTab currentTab) {
      _CozmoTabButton.gameObject.SetActive(currentTab != HomeTab.Cozmo);
      _CozmoTabDownButton.gameObject.SetActive(currentTab == HomeTab.Cozmo);

      _PlayTabButton.gameObject.SetActive(currentTab != HomeTab.Play);
      _PlayTabDownButton.gameObject.SetActive(currentTab == HomeTab.Play);

      _ProfileTabButton.gameObject.SetActive(currentTab != HomeTab.Profile);
      _ProfileTabDownButton.gameObject.SetActive(currentTab == HomeTab.Profile);

      switch (currentTab) {
      case HomeTab.Cozmo:
        _PlayTabButton.GetComponent<Image>().overrideSprite = _PlayTabUpSpriteCozmo;
        _ProfileTabButton.GetComponent<Image>().overrideSprite = _ProfileTabUpSpriteCozmo;
        break;
      case HomeTab.Play:
        _CozmoTabButton.GetComponent<Image>().overrideSprite = _CozmoTabUpSpritePlay;
        _ProfileTabButton.GetComponent<Image>().overrideSprite = _ProfileTabUpSpritePlay;
        break;
      case HomeTab.Profile:
        _CozmoTabButton.GetComponent<Image>().overrideSprite = _CozmoTabUpSpriteProfile;
        _PlayTabButton.GetComponent<Image>().overrideSprite = _PlayTabUpSpriteProfile;
        break;
      }

    }

    public Dictionary<string, ChallengeStatePacket> GetChallengeStates() {
      return _ChallengeStates;
    }

    public void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      // Do not allow changing tabs while receiving chests
      if (HomeViewCurrentlyOccupied) {
        return;
      }
      if (OnUnlockedChallengeClicked != null) {
        OnUnlockedChallengeClicked(challengeClicked, buttonTransform);
      }
    }

    private void HandleDailyGoalCompleted(GameEventWrapper gameEvent) {
      if (gameEvent.GameEventEnum == Anki.Cozmo.GameEvent.OnDailyGoalCompleted) {
        UpdatePlayTabText();
      }
    }

    private void UpdatePlayTabText() {
      if (DataPersistenceManager.Instance.CurrentSession == null) {
        _DailyGoalsCompletionText.text = "";
        _DailyGaolsCompletionTextDown.text = "";
        return;
      }
      int totalGoals = DataPersistenceManager.Instance.CurrentSession.DailyGoals.Count;
      int goalsCompleted = 0;
      for (int i = 0; i < DataPersistenceManager.Instance.CurrentSession.DailyGoals.Count; ++i) {
        if (DataPersistenceManager.Instance.CurrentSession.DailyGoals[i].GoalComplete) {
          goalsCompleted++;
        }
      }
      _DailyGoalsCompletionText.text = goalsCompleted.ToString() + "/" + totalGoals.ToString();
      _DailyGaolsCompletionTextDown.text = goalsCompleted.ToString() + "/" + totalGoals.ToString();
    }

    private void HandleItemValueChanged(string itemId, int delta, int newCount) {
      if (Cozmo.HexItemList.IsPuzzlePiece(itemId)) {
        UpdatePuzzlePieceCount();
      }
      CheckIfUnlockablesAffordableAndUpdateBadge();
    }

    private void CheckIfUnlockablesAffordableAndUpdateBadge() {
      if (ChestRewardManager.Instance.ChestPending) {
        _AnyUpgradeAffordableIndicator.SetActive(false);
        return;
      }
      Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      List<UnlockableInfo> unlockableUnlockData = UnlockablesManager.Instance.GetAvailableAndLockedExplicit();

      bool canAfford = false;
      for (int i = 0; i < unlockableUnlockData.Count; ++i) {
        if (playerInventory.CanRemoveItemAmount(unlockableUnlockData[i].UpgradeCostItemId,
              unlockableUnlockData[i].UpgradeCostAmountNeeded)) {
          canAfford = true;
          break;
        }
      }
      _AnyUpgradeAffordableIndicator.SetActive(canAfford);
    }

    #region Request Game


    private void HandleAskForMinigame(object messageObject) {
      if (HomeViewCurrentlyOccupied) {
        // Avoid dupes
        return;
      }

      ChallengeData data = DailyGoalManager.Instance.CurrentChallengeToRequest;
      // Do not send the minigame message if the challenge is invalid.
      if (data == null) {
        return;
      }

      ContextManager.Instance.AppFlash(playChime: true);
      // Create alert view with Icon
      AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab_Icon, overrideCloseOnTouchOutside: true);
      // Hook up callbacks
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleMiniGameConfirm);
      alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, LearnToCopeWithMiniGameRejection);
      alertView.SetIcon(data.ChallengeIcon);
      alertView.ViewClosed += HandleRequestDialogClose;
      alertView.TitleLocKey = LocalizationKeys.kRequestGameTitle;
      alertView.DescriptionLocKey = LocalizationKeys.kRequestGameDescription;
      alertView.SetTitleArgs(new object[] { Localization.Get(data.ChallengeTitleLocKey) });
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Request_Game);
      _RequestDialog = alertView;
    }

    private void LearnToCopeWithMiniGameRejection() {
      DAS.Info(this, "LearnToCopeWithMiniGameRejection");
      RobotEngineManager.Instance.SendDenyGameStart();
    }

    private void HandleMiniGameConfirm() {
      DAS.Info(this, "HandleMiniGameConfirm");
      if (_RequestDialog != null) {
        _RequestDialog.DisableAllButtons();
        _RequestDialog.ViewClosed -= HandleRequestDialogClose;
      }
      HandleMiniGameYesAnimEnd(true);
    }

    private void HandleMiniGameYesAnimEnd(bool success) {
      DAS.Info(this, "HandleMiniGameYesAnimEnd");
      MinigameConfirmed.Invoke(DailyGoalManager.Instance.CurrentChallengeToRequest.ChallengeID);
    }

    private void HandleExternalRejection(object messageObject) {
      DAS.Info(this, "HandleExternalRejection");
      if (_RequestDialog != null) {
        _RequestDialog.CloseView();
      }
    }

    private void HandleRequestDialogClose() {
      DAS.Info(this, "HandleUnexpectedClose");
      if (_RequestDialog != null) {
        _RequestDialog.ViewClosed -= HandleRequestDialogClose;
        DailyGoalManager.Instance.SetMinigameNeed();
      }
    }

    #endregion

    protected override void CleanUp() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);
      ChestRewardManager.Instance.ChestRequirementsGained -= HandleChestRequirementsGained;
      ChestRewardManager.Instance.ChestGained -= HandleChestGained;
      _RequirementPointsProgressBar.ProgressUpdateCompleted -= HandleProgressUpdated;
      GameEventManager.Instance.OnGameEvent -= HandleDailyGoalCompleted;
      DailyGoalManager.Instance.OnRefreshDailyGoals -= UpdatePlayTabText;
      UnlockablesManager.Instance.OnUnlockPopupRequested -= HandleUnlockView;

      if (_HelpViewInstance != null) {
        UIManager.CloseView(_HelpViewInstance);
      }

      Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded -= HandleItemValueChanged;
      playerInventory.ItemRemoved -= HandleItemValueChanged;
      StopCoroutine(BurstAfterInit());
    }

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      UIDefaultTransitionSettings defaultSettings = UIDefaultTransitionSettings.Instance;
      openAnimation.Append(defaultSettings.CreateOpenMoveTween(_TabButtonContainer,
        _TabButtonAnimationXOriginOffset,
        0));

      openAnimation.Join(defaultSettings.CreateOpenMoveTween(_TopBarContainer,
        0,
        _TopBarAnimationYOriginOffset,
        _TopBarOpenEase)
                         .SetDelay(defaultSettings.CascadeDelay));

      float contentContainerAnimDuration = defaultSettings.MoveOpenDurationSeconds;
      openAnimation.Join(defaultSettings.CreateOpenMoveTween(_TabContentContainer.transform,
        _TabContentAnimationXOriginOffset,
        0,
        _TabContentOpenEase,
        contentContainerAnimDuration)
                         .SetDelay(defaultSettings.CascadeDelay));

      _TabContentContainer.alpha = 0f;
      openAnimation.Join(defaultSettings.CreateFadeInTween(_TabContentContainer, Ease.Unset, contentContainerAnimDuration * 0.3f));

      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe, Color.white);
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      UIDefaultTransitionSettings defaultSettings = UIDefaultTransitionSettings.Instance;
      closeAnimation.Append(defaultSettings.CreateCloseMoveTween(_TabContentContainer.transform,
        _TabContentAnimationXOriginOffset,
        0));
      closeAnimation.Join(defaultSettings.CreateFadeOutTween(_TabContentContainer,
        Ease.Unset,
        UIDefaultTransitionSettings.Instance.MoveCloseDurationSeconds));

      closeAnimation.Join(defaultSettings.CreateCloseMoveTween(_TopBarContainer,
        0,
        _TopBarAnimationYOriginOffset,
        _TopBarCloseEase)
                          .SetDelay(defaultSettings.CascadeDelay));

      closeAnimation.Join(defaultSettings.CreateCloseMoveTween(_TabButtonContainer,
        _TabButtonAnimationXOriginOffset,
        0)
                          .SetDelay(defaultSettings.CascadeDelay));
    }
  }
}
