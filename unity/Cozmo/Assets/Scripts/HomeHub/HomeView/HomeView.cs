using Anki.UI;
using Anki.Assets;
using Anki.Cozmo;
using Cozmo.Challenge;
using Cozmo.UI;
using DataPersistence;
using DG.Tweening;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.HomeHub {
  public class HomeView : BaseView {

    public enum HomeTab {
      Cozmo,
      Play,
      Profile,
      Settings
    }

    private const string _kBurstEnergyAfterInitDisableKey = "HomeView.BurstEnergyAfterInit";

    private const float _kFreeplayIntervalCheck = 60.0f;
    private float _FreeplayIntervalLastTimestamp = -1;
    private float _FreeplayStartedTimestamp = -1;
    private const int kCubesCount = 3;

    public System.Action<StatContainer, StatContainer, Transform[]> DailyGoalsSet;

    public Action<string> MinigameConfirmed;
    public Action<string> OnTabChanged;

    [SerializeField]
    private AlertModal _BadLightAlertPrefab;

    [SerializeField]
    private ViewTab _CozmoTabPrefab;

    [SerializeField]
    private ViewTab _PlayTabPrefab;

    [SerializeField]
    private ViewTab _ProfileTabPrefab;

    [SerializeField]
    private ViewTab _SettingsTabPrefab;

    private ViewTab _CurrentTabInstance;
    private HomeTab _CurrentTab = HomeTab.Play;
    private HomeTab _PreviousTab = HomeTab.Play;

    [SerializeField]
    private GameObject _AnyUpgradeAffordableIndicator;

    [SerializeField]
    private CozmoButtonLegacy[] _CozmoTabButtons;

    [SerializeField]
    private CozmoButtonLegacy[] _PlayTabButtons;

    [SerializeField]
    private CozmoButtonLegacy[] _ProfileTabButtons;

    [SerializeField]
    private GameObject _SettingsSelectedTabs;

    [SerializeField]
    private GameObject _CozmoSelectedTabs;

    [SerializeField]
    private GameObject _PlaySelectedTabs;

    [SerializeField]
    private GameObject _ProfileSelectedTabs;

    [SerializeField]
    private CozmoButtonLegacy _SettingsButton;

    [SerializeField]
    private UnityEngine.UI.Image _SettingsAlertImage;
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

    private bool _LootSequenceActive = false;
    private System.Diagnostics.Stopwatch _Stopwatch = null;
    private string _CurrentChallengeId = null;
    private UnlockId _CurrentChallengeUnlockID = UnlockId.Count;

    private Sequence _RewardSequence;

    [SerializeField]
    private GameObject _EnergyRewardParticlePrefab;
    [SerializeField]
    private Transform _EnergyRewardStart_PlayTab;
    [SerializeField]
    private Transform _EnergyRewardStart_CozmoTab;
    [SerializeField]
    private Transform _EnergyRewardTarget;

    [SerializeField]
    private AnkiTextLegacy[] _DailyGoalsCompletionTexts;

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
    private RectTransform _BottomBarContainer;

    [SerializeField]
    private float _TopBarAnimationYOriginOffset;

    [SerializeField]
    private Ease _TopBarOpenEase = Ease.OutBack;

    [SerializeField]
    private Ease _TopBarCloseEase = Ease.InBack;

    [SerializeField]
    private Cozmo.UI.CozmoButtonLegacy _HelpButton;

    [SerializeField]
    private BaseModal _HelpTipsModalPrefab;
    private BaseModal _HelpTipsModalInstance;

    [SerializeField]
    private BaseModal _CubeHelpModalPrefab;

    private AlertModal _RequestDialog = null;

    private AlertModal _BadLightAlertInstance = null;

    private HomeHub _HomeHubInstance;

    public HomeHub HomeHubInstance {
      get { return _HomeHubInstance; }
      private set { _HomeHubInstance = value; }
    }

    public Transform TabContentContainer {
      get { return _TabContentContainer.transform; }
    }

    public Transform TabButtonContainer {
      get { return _TabButtonContainer.transform; }
    }
    public Transform TopBarContainer {
      get { return _TopBarContainer.transform; }
    }

    public Transform BottomBarContainer {
      get { return _BottomBarContainer.transform; }
    }
    public HomeTab CurrentTab {
      get { return _CurrentTab; }
    }

    public delegate void ButtonClickedHandler(string challengeClicked, Transform buttonTransform);

    public event ButtonClickedHandler OnUnlockedChallengeClicked;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStates;

    private IEnumerator _BurstEnergyAfterInitCoroutine = null;

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById, HomeHub homeHubInstance) {
      OnboardingManager.Instance.InitHomeHubOnboarding(this);
      ChestRewardManager.Instance.TryPopulateChestRewards();
      _FreeplayIntervalLastTimestamp = -1;
      _FreeplayStartedTimestamp = Time.time;
      _HomeHubInstance = homeHubInstance;

      DASEventDialogName = "home_view";

      _ChallengeStates = challengeStatesById;

      InitializeButtons(_CozmoTabButtons, HandleCozmoTabButton, "switch_to_cozmo_tab_button");
      InitializeButtons(_PlayTabButtons, HandlePlayTabButton, "switch_to_play_tab_button");
      InitializeButtons(_ProfileTabButtons, HandleProfileTabButton, "switch_to_profile_tab_button");

      _HelpButton.Initialize(HandleHelpButton, "help_button", DASEventDialogName);
      _SettingsButton.Initialize(HandleSettingsButton, "settings_button", DASEventDialogName);

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.EngineErrorCodeMessage>(HandleEngineErrorCode);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.OnNumBlocksConnectedChanged += HandleBlockConnectivityChanged;
        _SettingsAlertImage.gameObject.SetActive(RobotEngineManager.Instance.CurrentRobot.LightCubes.Count != kCubesCount);
      }

      _RequirementPointsProgressBar.ProgressUpdateCompleted += HandleCheckForLootView;
      DailyGoalManager.Instance.OnRefreshDailyGoals += UpdatePlayTabText;
      RewardedActionManager.Instance.OnFreeplayRewardEvent += HandleFreeplayRewardedAction;
      GameEventManager.Instance.OnGameEvent += HandleGameEvents;
      UpdatePlayTabText();

      Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded += HandleItemValueChanged;
      playerInventory.ItemRemoved += HandleItemValueChanged;
      playerInventory.ItemCountSet += HandleItemValueChanged;
      CheckIfUnlockablesAffordableAndUpdateBadge();

      UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints(), true);
      // If in SDK Mode, immediately open Settings and SDK view instead of PlayTab,
      // otherwise default to opening PlayTab
      if (DataPersistenceManager.Instance.IsSDKEnabled) {
        SwitchToTab(HomeTab.Settings);
      }
      else {
        SwitchToTab(HomeTab.Play);
      }

      // Start listening for Battery Level popups now that HomeView is fully initialized
      PauseManager.Instance.ListeningForBatteryLevel = true;
    }

    private void HandleBlockConnectivityChanged(int blocksConnected) {
      _SettingsAlertImage.gameObject.SetActive(blocksConnected != kCubesCount);
    }

    private void InitializeButtons(CozmoButtonLegacy[] buttons, UnityEngine.Events.UnityAction callback, string dasButtonName) {
      foreach (CozmoButtonLegacy button in buttons) {
        button.Initialize(callback, dasButtonName, DASEventDialogName);
      }
    }

    private void HandleEngineErrorCode(Anki.Cozmo.ExternalInterface.EngineErrorCodeMessage message) {
      if (_BadLightAlertInstance != null && message.errorCode == Anki.Cozmo.EngineErrorCode.ImageQualityGood) {
        _BadLightAlertInstance.CloseDialog();
      }
      else if (message.errorCode == Anki.Cozmo.EngineErrorCode.ImageQualityTooBright ||
          message.errorCode == Anki.Cozmo.EngineErrorCode.ImageQualityTooDark) {
        CreateBadLightAlert();
      }
    }

    private void CreateBadLightAlert() {
      System.Action<BaseModal> badLightAlertCreated = (alertModal) => {
        ContextManager.Instance.AppFlash(playChime: true);

        var badLightAlertData = new AlertModalData("bad_light_alert", null, showCloseButton: true);
        var badLightAlert = (AlertModal)alertModal;
        badLightAlert.InitializeAlertData(badLightAlertData);

        _BadLightAlertInstance = badLightAlert;
      };

      var badLightPriorityData = new ModalPriorityData(ModalPriorityLayer.VeryLow, 0,
                                                       LowPriorityModalAction.CancelSelf,
                                                       HighPriorityModalAction.Queue);
      UIManager.OpenModal(_BadLightAlertPrefab, badLightPriorityData, badLightAlertCreated,
                          overrideCloseOnTouchOutside: true);
    }

    private void UpdateChestProgressBar(int currentPoints, int numPointsNeeded, bool instant = false) {
      currentPoints = Mathf.Min(currentPoints, numPointsNeeded);
      float progress = ((float)currentPoints / (float)numPointsNeeded);

      if (instant) {
        _RequirementPointsProgressBar.SetValueInstant(progress);
      }
      else {
        _RequirementPointsProgressBar.SetTargetAndAnimate(progress);
      }

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

    public void SetChallengeStates(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      _ChallengeStates = challengeStatesById;
    }

    private void HandleCozmoTabButton() {
      // Do not allow changing tabs while receiving chests
      if (!_LootSequenceActive) {
        SwitchToTab(HomeTab.Cozmo);
      }
    }

    private void HandlePlayTabButton() {
      // Do not allow changing tabs while receiving chests
      if (!_LootSequenceActive) {
        SwitchToTab(HomeTab.Play);
      }
    }

    private void HandleProfileTabButton() {
      // Do not allow changing tabs while receiving chests
      if (!_LootSequenceActive) {
        SwitchToTab(HomeTab.Profile);
      }
    }

    private void HandleHelpButton() {
      var helpTipsModalPriorityData = new ModalPriorityData(ModalPriorityLayer.Low, 0,
                                                            LowPriorityModalAction.CancelSelf,
                                                            HighPriorityModalAction.Stack);
      UIManager.OpenModal(_HelpTipsModalPrefab, helpTipsModalPriorityData, (newHelpModal) => {
        _HelpTipsModalInstance = newHelpModal;
      });
    }

    private void HandleSettingsButton() {
      // Don't allow settings button to be clicked when the view is doing other things
      if (_LootSequenceActive) {
        return;
      }
      if (_CurrentTab != HomeTab.Settings) {
        SwitchToTab(HomeTab.Settings);

        // auto scroll to the cubes setting panel if we don't have three cubes connected.
        if (RobotEngineManager.Instance.CurrentRobot != null && RobotEngineManager.Instance.CurrentRobot.LightCubes.Count != kCubesCount) {
          _CurrentTabInstance.ParentLayoutContentSizeFitter.OnResizedParent += () => {
            const int kCubesHelpIndex = 1;
            _ScrollRect.horizontalNormalizedPosition = _CurrentTabInstance.GetNormalizedSnapIndexPosition(kCubesHelpIndex);
          };
        }

      }
      else {
        SwitchToTab(_PreviousTab);
      }
    }

    private void SwitchToTab(HomeTab tab) {
      if (_CurrentTab != tab) {
        _PreviousTab = _CurrentTab;
      }
      _CurrentTab = tab;
      ClearCurrentTab();
      ShowNewCurrentTab(GetHomeViewTabPrefab(tab));
      UpdateTabGraphics(tab);
      // Check for Reward sequence whenever we enter a new tab
      CheckForRewardSequence();
    }

    private ViewTab GetHomeViewTabPrefab(HomeTab tab) {
      ViewTab tabPrefab = null;
      switch (tab) {
      case HomeTab.Cozmo:
        tabPrefab = _CozmoTabPrefab;
        break;
      case HomeTab.Play:
        tabPrefab = _PlayTabPrefab;
        break;
      case HomeTab.Profile:
        tabPrefab = _ProfileTabPrefab;
        break;
      case HomeTab.Settings:
        tabPrefab = _SettingsTabPrefab;
        break;
      }
      return tabPrefab;
    }

    private void ClearCurrentTab() {
      if (_CurrentTabInstance != null) {
        DestroyImmediate(_CurrentTabInstance.gameObject);
      }
    }

    private void ShowNewCurrentTab(ViewTab homeViewTabPrefab) {
      _ScrollRect.horizontalNormalizedPosition = 0.0f;
      _CurrentTabInstance = Instantiate(homeViewTabPrefab.gameObject).GetComponent<ViewTab>();
      _CurrentTabInstance.transform.SetParent(_ScrollRectContent, false);
      _CurrentTabInstance.Initialize(this);

      if (OnTabChanged != null) {
        OnTabChanged(homeViewTabPrefab.name);
      }
    }

    private void UpdateTabGraphics(HomeTab currentTab) {
      _SettingsSelectedTabs.gameObject.SetActive(currentTab == HomeTab.Settings);
      _CozmoSelectedTabs.gameObject.SetActive(currentTab == HomeTab.Cozmo);
      _PlaySelectedTabs.gameObject.SetActive(currentTab == HomeTab.Play);
      _ProfileSelectedTabs.gameObject.SetActive(currentTab == HomeTab.Profile);
      CheckIfUnlockablesAffordableAndUpdateBadge();
    }

    public Dictionary<string, ChallengeStatePacket> GetChallengeStates() {
      return _ChallengeStates;
    }

    public void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      // This is still needed because if you click on a challenge after the rewards is finshed,
      // but before the asset bundle is done loading the game dialog will come up before loot.
      // and loot will be stuck.
      if (_LootSequenceActive) {
        return;
      }
      if (OnUnlockedChallengeClicked != null) {
        OnUnlockedChallengeClicked(challengeClicked, buttonTransform);
      }
    }

    private void HandleGameEvents(GameEventWrapper gameEvent) {
      if (gameEvent.GameEventEnum == Anki.Cozmo.GameEvent.OnDailyGoalCompleted) {
        UpdatePlayTabText();
        if (_CurrentTab == HomeTab.Play) {
          CheckForRewardSequence();
        }
      }
      else if (gameEvent.GameEventEnum == Anki.Cozmo.GameEvent.OnUnlockableEarned) {
        CheckIfUnlockablesAffordableAndUpdateBadge();
      }
    }

    private void UpdatePlayTabText() {
      string goalProgressText = "";
      if (DataPersistenceManager.Instance.CurrentSession != null) {
        int totalGoals = DataPersistenceManager.Instance.CurrentSession.DailyGoals.Count;
        int goalsCompleted = 0;
        for (int i = 0; i < DataPersistenceManager.Instance.CurrentSession.DailyGoals.Count; ++i) {
          if (DataPersistenceManager.Instance.CurrentSession.DailyGoals[i].GoalComplete) {
            goalsCompleted++;
          }
        }

        goalProgressText = Localization.GetWithArgs(LocalizationKeys.kLabelFractionCount,
                                                    Localization.GetNumber(goalsCompleted),
                                                    Localization.GetNumber(totalGoals));
      }

      foreach (AnkiTextLegacy textLabel in _DailyGoalsCompletionTexts) {
        textLabel.text = goalProgressText;
      }
    }

    private void HandleItemValueChanged(string itemId, int delta, int newCount) {
      CheckIfUnlockablesAffordableAndUpdateBadge();
    }

    private void CheckIfUnlockablesAffordableAndUpdateBadge() {
      if (ChestRewardManager.Instance != null && ChestRewardManager.Instance.ChestPending) {
        _AnyUpgradeAffordableIndicator.SetActive(false);
        return;
      }
      if (DataPersistenceManager.Instance != null && UnlockablesManager.Instance != null) {
        Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        List<UnlockableInfo> unlockableUnlockData = UnlockablesManager.Instance.GetAvailableAndLocked();

        bool canAffordCozmoUpgrade = false;
        for (int i = 0; i < unlockableUnlockData.Count; ++i) {
          // Only check for unlockable actions
          if (unlockableUnlockData[i].UnlockableType == UnlockableType.Action) {
            if (playerInventory.CanRemoveItemAmount(unlockableUnlockData[i].UpgradeCostItemId,
                  unlockableUnlockData[i].UpgradeCostAmountNeeded)) {
              canAffordCozmoUpgrade = true;
              break;
            }
          }
        }
        _AnyUpgradeAffordableIndicator.SetActive(canAffordCozmoUpgrade && _CurrentTab != HomeTab.Cozmo);
      }
    }

    #region Freeplay Related GameEvents


    // Every kFreeplayIntervalCheck seconds, fire the FreeplayInterval event for Freeplay time related goals
    protected void Update() {
      if (_FreeplayIntervalLastTimestamp < 0.0f) {
        _FreeplayIntervalLastTimestamp = Time.time;
      }
      if (Time.time - _FreeplayIntervalLastTimestamp > _kFreeplayIntervalCheck) {
        _FreeplayIntervalLastTimestamp = Time.time;
        GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnFreeplayInterval, Time.time - _FreeplayStartedTimestamp));
      }
    }


    private void HandleFreeplayRewardedAction(RewardedActionData reward) {
      // Only update the chest progress bar and do context flash if the home view isn't currently occupied
      if (!_LootSequenceActive) {
        ContextManager.Instance.AppFlash(false);
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints(), false);
      }
    }

    #endregion

    #region Reward Sequence and Lootview

    public void CheckForRewardSequence() {
      // Don't start coroutines if not active; this can happen in CleanUp steps
      // See https://ankiinc.atlassian.net/browse/COZMO-5212
      if (!this.enabled || this.gameObject == null || !this.gameObject.activeInHierarchy || _LootSequenceActive) {
        return;
      }

      DailyGoalManager.Instance.ValidateExistingGoals();
      if (RewardedActionManager.Instance.RewardPending || DailyGoalManager.Instance.GoalsPending) {
        // If Rewards are pending, set sequence to active, shut down input until everything is done
        if (_BurstEnergyAfterInitCoroutine != null) {
          StopCoroutine(_BurstEnergyAfterInitCoroutine);
        }
        _BurstEnergyAfterInitCoroutine = BurstEnergyAfterInit();
        StartCoroutine(_BurstEnergyAfterInitCoroutine);
        UIManager.DisableTouchEvents(_kBurstEnergyAfterInitDisableKey);
      }
      else if (ChestRewardManager.Instance.ChestPending) {
        HandleCheckForLootView();
      }
      else {
        // Otherwise set minigame need and update chest progress bar to whatever it should be at as
        // we enter the view.
        EnableGameRequestsIfAllowed(true);
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints(), true);
      }
    }

    private IEnumerator BurstEnergyAfterInit() {
      yield return new WaitForFixedUpdate();
      if (_RewardSequence != null) {
        _RewardSequence.Kill();
      }
      _RewardSequence = DOTween.Sequence();
      // Only handle goal rewards 
      if (_CurrentTab == HomeTab.Play && DailyGoalManager.Instance.GoalsPending) {
        for (int i = 0; i < DailyGoalManager.Instance.PendingDailyGoals.Count; i++) {
          DailyGoal currGoal = DailyGoalManager.Instance.PendingDailyGoals[i];
          _RewardSequence = EnergyRewardsBurst(currGoal.PointsRewarded, GetGoalSource(currGoal), _RewardSequence);
        }
        DailyGoalManager.Instance.ResolveDailyGoalsEarned();
      }
      Transform source = _EnergyRewardStart_PlayTab;
      if (_CurrentTab == HomeTab.Cozmo) {
        source = _EnergyRewardStart_CozmoTab;
      }
      if (RewardedActionManager.Instance.RewardPending) {
        RewardedActionManager.Instance.PendingActionRewards = RewardedActionManager.Instance.ResolveTagRewardCollisions(RewardedActionManager.Instance.PendingActionRewards);
        _RewardSequence = EnergyRewardsBurst(RewardedActionManager.Instance.TotalPendingEnergy, source, _RewardSequence);
      }
      // Prevent stray reward particles from being forgotten by dotween bug
      _RewardSequence.AppendInterval(GenericRewardsConfig.Instance.ExpParticleStagger);
      _RewardSequence.OnComplete(ResolveRewardParticleBurst);
      _RewardSequence.Play();
    }


    // If we have a Chest Pending, open the loot view once the progress bar finishes filling.
    // If there are Rewards Pending, do this when the Energy Sequence ends.
    private void HandleCheckForLootView() {
      if (ChestRewardManager.Instance.ChestPending && !_LootSequenceActive
          && RewardedActionManager.Instance.RewardPending == false) {
        // phase Loot onboarding is two stages, the first one is just an explination pointing to your meter,
        // then the callback from Onboarding will open the loot view.
        if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Loot)) {
          // Block Loot Sequences to prevent bugs with onboarding loading
          _LootSequenceActive = true;
          OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.Loot);
          UIManager.EnableTouchEvents();
          OnboardingManager.Instance.OnOnboardingStageStarted += HandleOnboardingLootStarted;
        }
        else {
          OpenLootView();
        }
      }
      else {
        // Update Minigame need now that daily goal progress has changed
        EnableGameRequestsIfAllowed(true);
      }
    }

    // Opens loot view and fires and relevant events
    private void OpenLootView() {
      if (_LootSequenceActive) {
        DAS.Warn("HomeView.OpenLootview", "Attempted to Load LootView Twice");
        return;
      }
      _LootSequenceActive = true;
      _EmotionChipTag.gameObject.SetActive(false);
      EnableGameRequestsIfAllowed(false);

      AssetBundleManager.Instance.LoadAssetBundleAsync(_LootViewPrefabData.AssetBundle, (bool success) => {
        if (success) {
          BaseDialog.SimpleBaseDialogHandler lootViewClosed = () => {
            HandleLootViewCloseAnimationFinished();
            // Only unload the asset bundle if we actually loaded it before
            AssetBundleManager.Instance.UnloadAssetBundle(_LootViewPrefabData.AssetBundle);
          };

          Action<BaseModal> lootViewCreated = (newModal) => {
            LootView lootView = (LootView)newModal;
            lootView.LootBoxRewards = ChestRewardManager.Instance.PendingChestRewards;
            lootView.DialogCloseAnimationFinished += (lootViewClosed);
          };

          var lootViewPriorityData = new ModalPriorityData(ModalPriorityLayer.VeryHigh, 0,
                                                           LowPriorityModalAction.Queue,
                                                           HighPriorityModalAction.ForceCloseOthersAndOpen);

          Action<GameObject> lootViewPrefabLoaded = (GameObject prefabObject) => {
            if (prefabObject != null) {
              UIManager.OpenModal(prefabObject.GetComponent<LootView>(), lootViewPriorityData, lootViewCreated);
            }
            else {
              DAS.Error("HomeView.OpenLootView", "Error loading LootViewPrefabData");
            }
          };

          _LootViewPrefabData.LoadAssetData(lootViewPrefabLoaded);
        }
        else {
          DAS.Error("HomeView.OpenLootView", "Failed to load asset bundle " + _LootViewPrefabData.AssetBundle);
        }
      });
    }

    private void HandleOnboardingLootStarted(OnboardingManager.OnboardingPhases phase, int onboardingStage) {
      if (phase == OnboardingManager.OnboardingPhases.Loot && onboardingStage > 0) {
        // Now that we are done with the onboarding part of the loot sequence, make it possible for loot sequences to be
        // started again before firing open loot view.
        _LootSequenceActive = false;
        OpenLootView();
        OnboardingManager.Instance.OnOnboardingStageStarted -= HandleOnboardingLootStarted;
      }
    }

    // If enabling is allowed by the tab, enable, otherwise ignore
    public void EnableGameRequestsIfAllowed(bool isEnabled) {
      if (_CurrentTabInstance != null) {
        _CurrentTabInstance.EnableGameRequestsIfAllowed(isEnabled);
      }
    }

    private void HandleLootViewCloseAnimationFinished() {
      _EmotionChipTag.gameObject.SetActive(true);
      _LootSequenceActive = false;
      EnableGameRequestsIfAllowed(true);
      CheckIfUnlockablesAffordableAndUpdateBadge();
      // Snap to zero, then tween to current progress
      UpdateChestProgressBar(0, ChestRewardManager.Instance.GetNextRequirementPoints(), true);
      UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());
    }

    // Create Energy Reward particles, set up Tween Sequence, and get it started, prevent game requests
    // and other obnoxious logic when we are doing pretty things with particles
    public Sequence EnergyRewardsBurst(int pointsEarned, Transform energySource, Sequence rewardSeqeuence) {
      EnableGameRequestsIfAllowed(false);
      GenericRewardsConfig rc = GenericRewardsConfig.Instance;
      int rewardCount = Mathf.CeilToInt((float)pointsEarned / (float)rc.ExpPerParticleEffect);
      for (int i = 0; i < rewardCount; i++) {
        Transform newRewardParticles = UIManager.CreateUIElement(_EnergyRewardParticlePrefab, energySource).transform;

        // COZMO-9389: Cause of DOTween bug is that in the case of rewarding goals, the particles were spawned as a 
        // child of the Daily Goal cell. A bug in the Enable/Disable touches flow allowed you to, in certain cases, 
        // tap another tab or otherwise cause the Daily Goal cells to be destroyed before the tween finished, which in
        // turn would destroy the particles, which causes the target for DOTween to be null.
        // Fix is to ensure that the particles are parented under the HomeView so that they are properly cleaned up if 
        // HomeView is destroyed (HomeView owns the _RewardSequence). Also fixed issue where you could tap during 
        // rewards sequence.
        newRewardParticles.SetParent(this.transform, worldPositionStays: true);

        float xOffset = UnityEngine.Random.Range(rc.ExpParticleMinSpread, rc.ExpParticleMaxSpread);
        if (UnityEngine.Random.Range(0.0f, 1.0f) >= 0.5f) {
          xOffset *= -1.0f;
        }
        float yOffset = UnityEngine.Random.Range(rc.ExpParticleMinSpread, rc.ExpParticleMaxSpread);
        if (UnityEngine.Random.Range(0.0f, 1.0f) >= 0.5f) {
          yOffset *= -1.0f;
        }
        float exitTime = UnityEngine.Random.Range(0.0f, rc.ExpParticleStagger) + rc.ExpParticleBurst + rc.ExpParticleHold;
        Vector3 rewardTarget = new Vector3(newRewardParticles.position.x + xOffset, newRewardParticles.position.y + yOffset, newRewardParticles.position.z);
        // Use local move for the Vector3 Spread
        rewardSeqeuence.Insert(0.0f, newRewardParticles.DOLocalMove(rewardTarget, rc.ExpParticleBurst).SetEase(Ease.OutBack));
        // Use non local move to properly tween to target end transform position
        rewardSeqeuence.Insert(exitTime, newRewardParticles.DOMove(_EnergyRewardTarget.position, rc.ExpParticleLeave).SetEase(Ease.InBack).OnComplete(() => {
          CleanUpRewardParticles(newRewardParticles);
        }).OnStart(() => {
          // play the reward sound
          var playSound = newRewardParticles.GetComponent<Anki.Cozmo.Audio.PlaySound>();
          if (playSound != null) {
            playSound.Play();
          }
        }));
      }
      return rewardSeqeuence;
    }

    private void CleanUpRewardParticles(Transform toClean) {
      _EnergyBarEmitter.Emit(GenericRewardsConfig.Instance.BurstPerParticleHit);
      Destroy(toClean.gameObject);
    }

    private void ResolveRewardParticleBurst() {
      RewardedActionManager.Instance.SendPendingRewardsToInventory();
      if (ChestRewardManager.Instance.ChestPending == false) {
        EnableGameRequestsIfAllowed(true);
        UIManager.EnableTouchEvents(_kBurstEnergyAfterInitDisableKey);
      }
      // This is the thing that eventually opens lootview via HandleCheckForLootView handler
      UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());
    }

    private Transform GetGoalSource(DailyGoal goal) {
      Transform source = _EnergyRewardStart_CozmoTab;
      if (DailyGoalManager.Instance.GoalPanelInstance == null) {
        DAS.Warn("HomeView.GetGoalSource", "GoalPanelInstance is NULL, this should only be called with a GoalPanel active");
        return source;
      }
      else {
        DailyGoalPanel goalPanel = DailyGoalManager.Instance.GoalPanelInstance;
        for (int i = 0; i < goalPanel.GoalCells.Count; i++) {
          if (goalPanel.GoalCells[i].Goal == goal) {
            source = goalPanel.GoalCells[i].transform;
          }
        }
      }
      return source;
    }

    #endregion

    #region Request Game

    private void HandleAskForMinigame(Anki.Cozmo.ExternalInterface.RequestGameStart messageObject) {
      ChallengeData data = RobotEngineManager.Instance.RequestGameManager.GetDataForGameID(messageObject.gameRequested);
      // Do not send the minigame message if the challenge is invalid or currently not unlocked.
      if (data == null || !UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value)) {
        return;
      }

      var requestGameData = new AlertModalData("request_game_alert",
                                               LocalizationKeys.kRequestGameTitle,
                                               LocalizationKeys.kRequestGameDescription,
                                               new AlertModalButtonData("yes_play_game_button", LocalizationKeys.kButtonYes, HandleMiniGameConfirm),
                                               new AlertModalButtonData("no_cancel_game_button", LocalizationKeys.kButtonNo, HandleMiniGameRejection),
                                               icon: data.ChallengeIcon,
                                               dialogCloseAnimationFinishedCallback: HandleRequestDialogClose,
                                               titleLocArgs: new object[] { Localization.Get(data.ChallengeTitleLocKey) });

      var requestGamePriority = new ModalPriorityData(ModalPriorityLayer.VeryLow, 2,
                                                      LowPriorityModalAction.CancelSelf,
                                                      HighPriorityModalAction.Stack);

      Action<AlertModal> requestGameCreated = (alertModal) => {
        ContextManager.Instance.AppFlash(playChime: true);
        // start response timer
        _Stopwatch = new System.Diagnostics.Stopwatch(); // allways create new sintance - GC can take care of previous ones.
        _Stopwatch.Start();
        _CurrentChallengeId = data.ChallengeID;
        _CurrentChallengeUnlockID = data.UnlockId.Value;

        // Hook up callbacks
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Request_Game);
        _RequestDialog = alertModal;
      };

      Action<UIManager.CreationCancelledReason> requestGameCancelled = (cancelReason) => {
        HandleMiniGameRejection();
        HandleRequestDialogClose();
      };

      UIManager.OpenAlert(requestGameData, requestGamePriority, requestGameCreated,
                          creationCancelledCallback: requestGameCancelled,
                          overrideCloseOnTouchOutside: false);
    }

    private void HandleMiniGameRejection() {
      float elapsedSec = 0.0f;
      if (_Stopwatch != null) {
        _Stopwatch.Stop();
        elapsedSec = _Stopwatch.ElapsedMilliseconds / 1000.0f;
      }
      // If current challenge ID is null, robot has already canceled this request, so don't fire redundant DAS events
      if (_CurrentChallengeId != null) {
        DAS.Event("robot.request_app", _CurrentChallengeId, DASUtil.FormatExtraData("fail"));
        DAS.Event("robot.request_app_time", _CurrentChallengeId, DASUtil.FormatExtraData(elapsedSec.ToString()));
        _CurrentChallengeId = null;
      }

      RobotEngineManager.Instance.SendDenyGameStart();
    }

    private void HandleMiniGameConfirm() {
      float elapsedSec = 0.0f;
      if (_Stopwatch != null) {
        _Stopwatch.Stop();
        elapsedSec = _Stopwatch.ElapsedMilliseconds / 1000.0f;
      }
      DAS.Event("robot.request_app", _CurrentChallengeId, DASUtil.FormatExtraData("success"));
      DAS.Event("robot.request_app_time", _CurrentChallengeId, DASUtil.FormatExtraData(elapsedSec.ToString()));

      if (_RequestDialog != null) {
        _RequestDialog.DisableAllButtons();
        _RequestDialog.DialogClosed -= HandleRequestDialogClose;
      }

      if (MinigameConfirmed != null) {
        if (_CurrentChallengeUnlockID != UnlockId.Count) {
          MinigameConfirmed.Invoke(_CurrentChallengeId);
        }
        else {
          int cubesRequired = ChallengeDataList.Instance.GetChallengeDataById(_CurrentChallengeId).ChallengeConfig.NumCubesRequired();
          if (RobotEngineManager.Instance.CurrentRobot.LightCubes.Count < cubesRequired) {
            // challenge request has become null due to cube(s) disconnecting.
            string challengeTitle = Localization.Get(ChallengeDataList.Instance.GetChallengeDataById(_CurrentChallengeId).ChallengeTitleLocKey);
            OpenNeedCubesAlert(RobotEngineManager.Instance.CurrentRobot.LightCubes.Count, cubesRequired, challengeTitle);
          }
          else {
            DAS.Error("HomeView.HandleMiniGameConfirm", "challenge request is null for an unknown reason");
          }
        }
      }

      _CurrentChallengeId = null;
    }

    private void HandleExternalRejection(object messageObject) {
      DAS.Info(this, "HandleExternalRejection");
      float elapsedSec = 0.0f;
      if (_Stopwatch != null) {
        _Stopwatch.Stop();
        elapsedSec = _Stopwatch.ElapsedMilliseconds / 1000.0f;
      }
      // If current challenge ID is null, we have already responded to this request, so don't fire redundant DAS events
      if (_CurrentChallengeId != null) {
        DAS.Event("robot.request_app", _CurrentChallengeId, DASUtil.FormatExtraData("robot_canceled"));
        DAS.Event("robot.request_app_time", _CurrentChallengeId, DASUtil.FormatExtraData(elapsedSec.ToString()));
        _CurrentChallengeId = null;
      }

      if (_RequestDialog != null) {
        _RequestDialog.CloseDialog();
        EnableGameRequestsIfAllowed(true);
      }
    }

    private void HandleRequestDialogClose() {
      DAS.Info(this, "HandleUnexpectedClose");
      if (_RequestDialog != null) {
        _RequestDialog.DialogCloseAnimationFinished -= HandleRequestDialogClose;
        EnableGameRequestsIfAllowed(true);
      }
    }

    #endregion

    public void OpenNeedCubesAlert(int currentCubes, int neededCubes, string titleString) {
      var needCubesPriorityData = new ModalPriorityData(ModalPriorityLayer.Low, 1,
                                                        LowPriorityModalAction.CancelSelf,
                                                        HighPriorityModalAction.Stack);
      AlertModalButtonData openCubeHelpButtonData = CreateCubeHelpButtonData(needCubesPriorityData);
      AlertModalData needCubesData = CreateNeedMoreCubesAlertData(openCubeHelpButtonData, currentCubes,
                                                                  neededCubes, titleString);

      UIManager.OpenAlert(needCubesData, needCubesPriorityData);
    }

    private AlertModalButtonData CreateCubeHelpButtonData(ModalPriorityData basePriorityData) {
      var cubeHelpModalPriorityData = ModalPriorityData.CreateSlightlyHigherData(basePriorityData);
      Action cubeHelpButtonPressed = () => {
        UIManager.OpenModal(_CubeHelpModalPrefab, cubeHelpModalPriorityData, null);
      };
      return new AlertModalButtonData("open_cube_help_modal_button",
                                      LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalButton,
                                      cubeHelpButtonPressed);
    }

    private AlertModalData CreateNeedMoreCubesAlertData(AlertModalButtonData primaryButtonData, int currentCubes, int neededCubes, string titleString) {
      int differenceCubes = neededCubes - currentCubes;
      object[] descLocArgs = new object[] {
        differenceCubes,
        (ItemDataConfig.GetCubeData().GetAmountName(differenceCubes)),
        titleString
      };

      return new AlertModalData("game_needs_more_cubes_alert",
                                LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalTitle,
                                LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalDescription,
                                primaryButtonData, showCloseButton: true, descLocArgs: descLocArgs);
    }

    protected override void CleanUp() {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.EngineErrorCodeMessage>(HandleEngineErrorCode);
      _RequirementPointsProgressBar.ProgressUpdateCompleted -= HandleCheckForLootView;
      GameEventManager.Instance.OnGameEvent -= HandleGameEvents;
      RewardedActionManager.Instance.OnFreeplayRewardEvent -= HandleFreeplayRewardedAction;
      DailyGoalManager.Instance.OnRefreshDailyGoals -= UpdatePlayTabText;

      if (_HelpTipsModalInstance != null) {
        UIManager.CloseModal(_HelpTipsModalInstance);
      }

      if (_RewardSequence != null) {
        _RewardSequence.Kill();
      }

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.OnNumBlocksConnectedChanged -= HandleBlockConnectivityChanged;
      }

      Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded -= HandleItemValueChanged;
      playerInventory.ItemRemoved -= HandleItemValueChanged;
      playerInventory.ItemCountSet -= HandleItemValueChanged;
      if (_BurstEnergyAfterInitCoroutine != null) {
        StopCoroutine(_BurstEnergyAfterInitCoroutine);
      }

      UIManager.EnableTouchEvents();
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
                         .SetDelay(defaultSettings.StartTimeStagger_sec));

      float contentContainerAnimDuration = defaultSettings.MoveOpenDurationSeconds;
      openAnimation.Join(defaultSettings.CreateOpenMoveTween(_TabContentContainer.transform,
        _TabContentAnimationXOriginOffset,
        0,
        _TabContentOpenEase,
        contentContainerAnimDuration)
                         .SetDelay(defaultSettings.StartTimeStagger_sec));

      _TabContentContainer.alpha = 0f;
      openAnimation.Join(defaultSettings.CreateFadeInTween(_TabContentContainer, Ease.Unset, contentContainerAnimDuration * 0.3f));
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
                          .SetDelay(defaultSettings.StartTimeStagger_sec));

      closeAnimation.Join(defaultSettings.CreateCloseMoveTween(_TabButtonContainer,
        _TabButtonAnimationXOriginOffset,
        0)
                          .SetDelay(defaultSettings.StartTimeStagger_sec));
    }
  }
}
