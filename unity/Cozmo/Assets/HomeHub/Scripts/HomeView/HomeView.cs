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

    public bool RewardSequenceActive = false;
    private Sequence _RewardSequence;

    [SerializeField]
    private GameObject _EnergyDooberPrefab;
    [SerializeField]
    private Transform _EnergyDooberStart;
    [SerializeField]
    private Transform _EnergyDooberEnd;
    private List<Transform> _EnergyDooberList = new List<Transform>();

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

      InitializeButtons(_CozmoTabButtons, HandleCozmoTabButton, "switch_to_cozmo_tab_button");
      InitializeButtons(_PlayTabButtons, HandlePlayTabButton, "switch_to_play_tab_button");
      InitializeButtons(_ProfileTabButtons, HandleProfileTabButton, "switch_to_profile_tab_button");

      _HelpButton.Initialize(HandleHelpButton, "help_button", DASEventViewName);
      _SettingsButton.Initialize(HandleSettingsButton, "settings_button", DASEventViewName);

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DenyGameStart>(HandleExternalRejection);

      _RequirementPointsProgressBar.ProgressUpdateCompleted += HandleGreenPointsBarUpdateComplete;
      UnlockablesManager.Instance.OnUnlockPopupRequested += HandleUnlockView;

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

    private void InitializeButtons(CozmoButton[] buttons, UnityEngine.Events.UnityAction callback, string dasButtonName) {
      foreach (CozmoButton button in buttons) {
        button.Initialize(callback, dasButtonName, DASEventViewName);
      }
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
      // Do not allow changing tabs while receiving chests
      if (HomeViewCurrentlyOccupied) {
        return;
      }
      SwitchToTab(HomeTab.Cozmo);
    }

    private void HandlePlayTabButton() {
      SwitchToTab(HomeTab.Play);
      // Check for Reward sequence whenever we enter the play tab
      CheckForRewardSequence();
    }

    private void HandleProfileTabButton() {
      // Do not allow changing tabs while receiving chests
      if (HomeViewCurrentlyOccupied) {
        return;
      }
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
      if (_CurrentTab != tab) {
        _PreviousTab = _CurrentTab;
      }
      _CurrentTab = tab;
      ClearCurrentTab();
      ShowNewCurrentTab(GetHomeViewTabPrefab(tab));
      UpdateTabGraphics(tab);
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

    #region Reward Sequence and Lootview

    private void CheckForRewardSequence() {
      if (RewardedActionManager.Instance.RewardPending || DailyGoalManager.Instance.GoalsPending) {
        // If Rewards are pending, set sequence to active, shut down input until everything is done
        // Don't bother updating the chest progress bar to current points 
        StartCoroutine(BurstEnergyAfterInit());
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

    // If we have a Chest Pending, open the loot view once the progress bar finishes filling.
    // If there are Rewards Pending, do this when the Energy Sequence ends.
    private void HandleGreenPointsBarUpdateComplete() {
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
      UpdateChestProgressBar(0, ChestRewardManager.Instance.GetNextRequirementPoints(), true);
      UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());
    }

    // Doobstorm 2016 - Create Energy Doobers, set up Tween Sequence, and get it started, prevent game requests
    // and other obnoxious logic when we are doing pretty things with particles
    public Sequence EnergyDooberBurst(int pointsEarned, Transform energySource, Sequence dooberSequence) {
      RewardSequenceActive = true;
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(Anki.Cozmo.BehaviorGameFlag.NoGame);
      }
      GenericRewardsConfig rc = GenericRewardsConfig.Instance;
      int doobCount = Mathf.CeilToInt((float)pointsEarned / rc.ExpPerParticleEffect);
      for (int i = 0; i < doobCount; i++) {
        Transform freshDoobz = UIManager.CreateUIElement(_EnergyDooberPrefab, energySource).transform;
        _EnergyDooberList.Add(freshDoobz);
        float xOffset = UnityEngine.Random.Range(rc.ExpParticleMinSpread, rc.ExpParticleMaxSpread);
        float yOffset = UnityEngine.Random.Range(rc.ExpParticleMinSpread, rc.ExpParticleMaxSpread);
        float exitTime = UnityEngine.Random.Range(0.0f, rc.ExpParticleStagger) + rc.ExpParticleBurst + rc.ExpParticleHold;
        Vector3 doobTarget = new Vector3(freshDoobz.position.x - xOffset, freshDoobz.position.y + yOffset, freshDoobz.position.z);
        // Use local move for the Vector3 Spread
        dooberSequence.Insert(0.0f, freshDoobz.DOLocalMove(doobTarget, rc.ExpParticleBurst).SetEase(Ease.OutBack));
        // Use non local move to properly tween to target end transform position
        dooberSequence.Insert(exitTime, freshDoobz.DOMove(_EnergyDooberEnd.position, rc.ExpParticleLeave).SetEase(Ease.InBack).OnComplete(() => (CleanUpDoober(freshDoobz))));
      }
      return dooberSequence;
    }

    private void CleanUpDoober(Transform toClean) {
      _EnergyBarEmitter.Emit(GenericRewardsConfig.Instance.BurstPerParticleHit);
      Destroy(toClean.gameObject);
    }

    private void ResolveDooberBurst() {
      RewardedActionManager.Instance.SendPendingRewardsToInventory();
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

    private IEnumerator BurstEnergyAfterInit() {
      yield return new WaitForFixedUpdate();
      _RewardSequence = DOTween.Sequence();
      if (DailyGoalManager.Instance.GoalsPending) {
        for (int i = 0; i < DailyGoalManager.Instance.PendingDailyGoals.Count; i++) {
          DailyGoal currGoal = DailyGoalManager.Instance.PendingDailyGoals[i];
          _RewardSequence = EnergyDooberBurst(currGoal.PointsRewarded, GetGoalSource(currGoal), _RewardSequence);
        }
        DailyGoalManager.Instance.ResolveDailyGoalsEarned();
      }
      else if (RewardedActionManager.Instance.RewardPending) {
        _RewardSequence = EnergyDooberBurst(RewardedActionManager.Instance.TotalPendingEnergy, _EnergyDooberStart, _RewardSequence);
      }
      // Prevent stray doobers from being forgotten by dotween bug
      _RewardSequence.AppendInterval(GenericRewardsConfig.Instance.ExpParticleStagger);
      _RewardSequence.AppendCallback(ResolveDooberBurst);
      _RewardSequence.Play();
    }

    // If we earned a chest, have the progress bar reflect the previous requirement level at full.
    private void HandleChestGained() {
      UpdateChestProgressBar(ChestRewardManager.Instance.GetPreviousRequirementPoints(), ChestRewardManager.Instance.GetPreviousRequirementPoints());
    }

    private Transform GetGoalSource(DailyGoal goal) {
      Transform source = _EnergyDooberStart;
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
      if (HomeViewCurrentlyOccupied) {
        // Avoid dupes
        return;
      }

      ChallengeData data = DailyGoalManager.Instance.CurrentChallengeToRequest;
      // Do not send the minigame message if the challenge is invalid or currently not unlocked.
      if (data == null || !UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value)) {
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
      ChestRewardManager.Instance.ChestGained -= HandleChestGained;
      _RequirementPointsProgressBar.ProgressUpdateCompleted -= HandleGreenPointsBarUpdateComplete;
      GameEventManager.Instance.OnGameEvent -= HandleDailyGoalCompleted;
      DailyGoalManager.Instance.OnRefreshDailyGoals -= UpdatePlayTabText;
      UnlockablesManager.Instance.OnUnlockPopupRequested -= HandleUnlockView;

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
