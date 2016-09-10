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
      Profile,
      Settings
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

    [SerializeField]
    private HomeViewTab _SettingsTabPrefab;

    private HomeViewTab _CurrentTabInstance;
    private HomeTab _CurrentTab = HomeTab.Play;
    private HomeTab _PreviousTab = HomeTab.Play;

    [SerializeField]
    private GameObject _AnyUpgradeAffordableIndicator;

    [SerializeField]
    private CozmoButton[] _CozmoTabButtons;

    [SerializeField]
    private CozmoButton[] _PlayTabButtons;

    [SerializeField]
    private CozmoButton[] _ProfileTabButtons;

    [SerializeField]
    private GameObject _SettingsSelectedTabs;

    [SerializeField]
    private GameObject _CozmoSelectedTabs;

    [SerializeField]
    private GameObject _PlaySelectedTabs;

    [SerializeField]
    private GameObject _ProfileSelectedTabs;

    [SerializeField]
    private CozmoButton _SettingsButton;

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
    private System.Diagnostics.Stopwatch _Stopwatch = null;
    private string _CurrentChallengeId = null;

    public bool RewardSequenceActive = false;
    private Sequence _RewardSequence;

    [SerializeField]
    private GameObject _EnergyRewardParticlePrefab;
    [SerializeField]
    private Transform _EnergyRewardStart_PlayTab;
    [SerializeField]
    private Transform _EnergyRewardStart_CozmoTab;
    [SerializeField]
    private Transform _EnergyRewardTarget;

    private List<Transform> _EnergyRewardsList = new List<Transform>();

    [SerializeField]
    private AnkiTextLabel[] _DailyGoalsCompletionTexts;

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

    private AlertView _RequestDialog = null;

    private AlertView _BadLightDialog = null;

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
        return (_RequestDialog != null || _LootViewInstance != null || _BadLightDialog != null ||
                _HelpViewInstance != null || _HomeHubInstance.IsChallengeDetailsActive ||
                RewardSequenceActive || PauseManager.Instance.IsAnyDialogOpen);
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

      InitializeButtons(_CozmoTabButtons, HandleCozmoTabButton, "switch_to_cozmo_tab_button");
      InitializeButtons(_PlayTabButtons, HandlePlayTabButton, "switch_to_play_tab_button");
      InitializeButtons(_ProfileTabButtons, HandleProfileTabButton, "switch_to_profile_tab_button");

      _HelpButton.Initialize(HandleHelpButton, "help_button", DASEventViewName);
      _SettingsButton.Initialize(HandleSettingsButton, "settings_button", DASEventViewName);

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.EngineErrorCodeMessage>(HandleEngineErrorCode);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);

      // automatically apply chest rewards that are queued up incase the app is exitied during the middle
      // of a reward loot view flow.
      ChestRewardManager.Instance.TryPopulateChestRewards();
      ChestRewardManager.Instance.ApplyChestRewards();

      _RequirementPointsProgressBar.ProgressUpdateCompleted += HandleGreenPointsBarUpdateComplete;
      DailyGoalManager.Instance.OnRefreshDailyGoals += UpdatePlayTabText;
      GameEventManager.Instance.OnGameEvent += HandleDailyGoalCompleted;
      UpdatePlayTabText();

      Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded += HandleItemValueChanged;
      playerInventory.ItemRemoved += HandleItemValueChanged;
      CheckIfUnlockablesAffordableAndUpdateBadge();

      // If in SDK Mode, immediately open Settings and SDK view instead of PlayTab,
      // otherwise default to opening PlayTab
      if (DataPersistenceManager.Instance.Data.DeviceSettings.IsSDKEnabled) {
        HandleSettingsButton();
      }
      else {
        HandlePlayTabButton();
      }
      UpdatePuzzlePieceCount();

      ChestRewardManager.Instance.ChestGained += HandleChestGained;
    }

    private void InitializeButtons(CozmoButton[] buttons, UnityEngine.Events.UnityAction callback, string dasButtonName) {
      foreach (CozmoButton button in buttons) {
        button.Initialize(callback, dasButtonName, DASEventViewName);
      }
    }

    private void HandleEngineErrorCode(Anki.Cozmo.ExternalInterface.EngineErrorCodeMessage message) {
      if (_BadLightDialog != null && message.errorCode == Anki.Cozmo.EngineErrorCode.ImageQualityGood) {
        _BadLightDialog.CloseView();
      }
      if (HomeViewCurrentlyOccupied) {
        return;
      }
      if (message.errorCode == Anki.Cozmo.EngineErrorCode.ImageQualityTooBright ||
          message.errorCode == Anki.Cozmo.EngineErrorCode.ImageQualityTooDark) {
        CreateBadLightPopup();
      }
    }

    private void CreateBadLightPopup() {
      ContextManager.Instance.AppFlash(playChime: true);
      // Create alert view with Icon
      AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab_BadLight, overrideCloseOnTouchOutside: true);
      // Hook up callbacks
      alertView.SetCloseButtonEnabled(true);
      _BadLightDialog = alertView;
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
      SwitchToTab(HomeTab.Cozmo);
    }

    private void HandlePlayTabButton() {
      SwitchToTab(HomeTab.Play);
    }

    private void HandleProfileTabButton() {
      SwitchToTab(HomeTab.Profile);
    }

    private void HandleHelpButton() {
      _HelpViewInstance = UIManager.OpenView(_HelpViewPrefab);
    }

    private void HandleSettingsButton() {
      // Do not allow changing tabs while receiving chests
      if (HomeViewCurrentlyOccupied) {
        return;
      }
      if (_CurrentTab != HomeTab.Settings) {
        SwitchToTab(HomeTab.Settings);
      }
      else {
        SwitchToTab(_PreviousTab);
      }
    }

    private void SwitchToTab(HomeTab tab) {
      // Do not allow changing tabs while receiving chests
      if (HomeViewCurrentlyOccupied) {
        return;
      }
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

    private HomeViewTab GetHomeViewTabPrefab(HomeTab tab) {
      HomeViewTab tabPrefab = null;
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
        Destroy(_CurrentTabInstance.gameObject);
      }
    }

    private void ShowNewCurrentTab(HomeViewTab homeViewTabPrefab) {
      _ScrollRect.horizontalNormalizedPosition = 0.0f;
      _CurrentTabInstance = Instantiate(homeViewTabPrefab.gameObject).GetComponent<HomeViewTab>();
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
        if (_CurrentTab == HomeTab.Play) {
          CheckForRewardSequence();
        }
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

        goalProgressText = Localization.GetWithArgs(LocalizationKeys.kLabelFractionCount, goalsCompleted, totalGoals);
      }

      foreach (AnkiTextLabel textLabel in _DailyGoalsCompletionTexts) {
        textLabel.text = goalProgressText;
      }
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
      List<UnlockableInfo> unlockableUnlockData = UnlockablesManager.Instance.GetAvailableAndLocked();

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

    #region Reward Sequence and Lootview

    public void CheckForRewardSequence() {
      DailyGoalManager.Instance.ValidateExistingGoals();
      if (RewardedActionManager.Instance.RewardPending || DailyGoalManager.Instance.GoalsPending) {
        // If Rewards are pending, set sequence to active, shut down input until everything is done
        StartCoroutine(BurstEnergyAfterInit());
        UIManager.DisableTouchEvents();
      }
      else {
        // Otherwise set minigame need and update chest progress bar to whatever it should be at as
        // we enter the view.
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          DailyGoalManager.Instance.SetMinigameNeed();
        }
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints(), true);
      }
    }

    private IEnumerator BurstEnergyAfterInit() {
      yield return new WaitForFixedUpdate();
      _RewardSequence = DOTween.Sequence();
      // Only handle goal rewards 
      if (_CurrentTab == HomeTab.Play && DailyGoalManager.Instance.GoalsPending) {
        int goalPointOffset = 0;
        for (int i = 0; i < DailyGoalManager.Instance.PendingDailyGoals.Count; i++) {
          DailyGoal currGoal = DailyGoalManager.Instance.PendingDailyGoals[i];
          _RewardSequence = EnergyRewardsBurst(currGoal.PointsRewarded, GetGoalSource(currGoal), _RewardSequence);
          goalPointOffset += currGoal.PointsRewarded;
        }
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints() - goalPointOffset, ChestRewardManager.Instance.GetNextRequirementPoints(), true);
        DailyGoalManager.Instance.ResolveDailyGoalsEarned();
      }
      else {
        // Only do the offset for goal points since those are rewarded when the goal is completed
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints(), true);
      }
      Transform source = _EnergyRewardStart_PlayTab;
      if (_CurrentTab == HomeTab.Cozmo) {
        source = _EnergyRewardStart_CozmoTab;
      }
      if (RewardedActionManager.Instance.RewardPending) {
        RewardedActionManager.Instance.ResolveTagRewardCollisions();
        _RewardSequence = EnergyRewardsBurst(RewardedActionManager.Instance.TotalPendingEnergy, source, _RewardSequence);
      }
      // Prevent stray reward particles from being forgotten by dotween bug
      _RewardSequence.AppendInterval(GenericRewardsConfig.Instance.ExpParticleStagger);
      _RewardSequence.AppendCallback(ResolveRewardParticleBurst);
      _RewardSequence.Play();
    }


    // If we have a Chest Pending, open the loot view once the progress bar finishes filling.
    // If there are Rewards Pending, do this when the Energy Sequence ends.
    private void HandleGreenPointsBarUpdateComplete() {
      if (ChestRewardManager.Instance.ChestPending && _LootViewInstance == null
          && RewardedActionManager.Instance.RewardPending == false) {
        // phase Loot onboarding is two stages, the first one is just an explination pointing to your meter,
        // then the callback from Onboarding will open the loot view.
        if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Loot)) {
          OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.Loot);
          OnboardingManager.Instance.OnOnboardingStageStarted += HandleOnboardingLootStarted;
        }
        else {
          OpenLootView();
        }
      }
      else {
        // Update Minigame need now that daily goal progress has changed
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          DailyGoalManager.Instance.SetMinigameNeed();
        }
      }
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

    private void HandleOnboardingLootStarted(OnboardingManager.OnboardingPhases phase, int onboardingStage) {
      if (phase == OnboardingManager.OnboardingPhases.Loot && onboardingStage > 0) {
        OpenLootView();
        OnboardingManager.Instance.OnOnboardingStageStarted -= HandleOnboardingLootStarted;
      }
    }

    private void HandleLootViewCloseAnimationFinished() {
      _EmotionChipTag.gameObject.SetActive(true);
      RewardSequenceActive = false;
      _LootViewInstance = null;
      CheckIfUnlockablesAffordableAndUpdateBadge();
      UpdateChestProgressBar(0, ChestRewardManager.Instance.GetNextRequirementPoints(), true);
      UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());
    }

    // Create Energy Reward particles, set up Tween Sequence, and get it started, prevent game requests
    // and other obnoxious logic when we are doing pretty things with particles
    public Sequence EnergyRewardsBurst(int pointsEarned, Transform energySource, Sequence rewardSeqeuence) {
      RewardSequenceActive = true;
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.NoGame);
      }
      GenericRewardsConfig rc = GenericRewardsConfig.Instance;
      int rewardCount = Mathf.CeilToInt((float)pointsEarned / (float)rc.ExpPerParticleEffect);
      for (int i = 0; i < rewardCount; i++) {
        Transform newRewardParticles = UIManager.CreateUIElement(_EnergyRewardParticlePrefab, energySource).transform;
        _EnergyRewardsList.Add(newRewardParticles);
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
        RewardSequenceActive = false;
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.All);
        }
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());
        UIManager.EnableTouchEvents();
      }
      else {
        HandleChestGained();
      }

    }
    // If we earned a chest, have the progress bar reflect the previous requirement level at full.
    private void HandleChestGained() {
      UpdateChestProgressBar(ChestRewardManager.Instance.GetPreviousRequirementPoints(), ChestRewardManager.Instance.GetPreviousRequirementPoints());
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


    private void HandleAskForMinigame(object messageObject) {
      if (HomeViewCurrentlyOccupied || _CurrentTab == HomeTab.Settings) {
        // Avoid dupes or conflicting popups
        // Don't request games in Settings tab to avoid
        // being disruptive to SDK users and those changing
        // settings.
        return;
      }

      ChallengeData data = DailyGoalManager.Instance.CurrentChallengeToRequest;
      // Do not send the minigame message if the challenge is invalid or currently not unlocked.
      if (data == null || !UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value)) {
        return;
      }

      ContextManager.Instance.AppFlash(playChime: true);
      // start response timer
      _Stopwatch = new System.Diagnostics.Stopwatch(); // allways create new sintance - GC can take care of previous ones.
      _Stopwatch.Start();
      _CurrentChallengeId = data.ChallengeID;
      // Create alert view with Icon
      AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab_Icon, overrideCloseOnTouchOutside: false);
      // Hook up callbacks
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kButtonYes, HandleMiniGameConfirm);
      alertView.SetSecondaryButton(LocalizationKeys.kButtonNo, HandleMiniGameRejection);
      alertView.SetIcon(data.ChallengeIcon);
      alertView.ViewClosed += HandleRequestDialogClose;
      alertView.TitleLocKey = LocalizationKeys.kRequestGameTitle;
      alertView.DescriptionLocKey = LocalizationKeys.kRequestGameDescription;
      alertView.SetTitleArgs(new object[] { Localization.Get(data.ChallengeTitleLocKey) });
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Request_Game);
      _RequestDialog = alertView;
    }

    private void HandleMiniGameRejection() {
      float elapsedSec = 0.0f;
      if (_Stopwatch != null) {
        _Stopwatch.Stop();
        elapsedSec = _Stopwatch.ElapsedMilliseconds / 1000.0f;
      }
      DAS.Event("robot.request_app", _CurrentChallengeId, null,
        new Dictionary<string, string>() { { "$data", "fail" } });
      DAS.Event("robot.request_app_time", _CurrentChallengeId, null,
        new Dictionary<string, string>() { { "$data", elapsedSec.ToString() } });
      _CurrentChallengeId = null;

      RobotEngineManager.Instance.SendDenyGameStart();
    }

    private void HandleMiniGameConfirm() {
      float elapsedSec = 0.0f;
      if (_Stopwatch != null) {
        _Stopwatch.Stop();
        elapsedSec = _Stopwatch.ElapsedMilliseconds / 1000.0f;
      }
      DAS.Event("robot.request_app", _CurrentChallengeId, null,
        new Dictionary<string, string>() { { "$data", "success" } });
      DAS.Event("robot.request_app_time", _CurrentChallengeId, null,
        new Dictionary<string, string>() { { "$data", elapsedSec.ToString() } });
      _CurrentChallengeId = null;

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
      float elapsedSec = 0.0f;
      if (_Stopwatch != null) {
        _Stopwatch.Stop();
        elapsedSec = _Stopwatch.ElapsedMilliseconds / 1000.0f;
      }
      DAS.Event("robot.request_app", _CurrentChallengeId, null,
        new Dictionary<string, string>() { { "$data", "robot_canceled" } });
      DAS.Event("robot.request_app_time", _CurrentChallengeId, null,
        new Dictionary<string, string>() { { "$data", elapsedSec.ToString() } });

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
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.EngineErrorCodeMessage>(HandleEngineErrorCode);
      ChestRewardManager.Instance.ChestGained -= HandleChestGained;
      _RequirementPointsProgressBar.ProgressUpdateCompleted -= HandleGreenPointsBarUpdateComplete;
      GameEventManager.Instance.OnGameEvent -= HandleDailyGoalCompleted;
      DailyGoalManager.Instance.OnRefreshDailyGoals -= UpdatePlayTabText;

      if (_HelpViewInstance != null) {
        UIManager.CloseView(_HelpViewInstance);
      }

      Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded -= HandleItemValueChanged;
      playerInventory.ItemRemoved -= HandleItemValueChanged;
      StopCoroutine(BurstEnergyAfterInit());
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
