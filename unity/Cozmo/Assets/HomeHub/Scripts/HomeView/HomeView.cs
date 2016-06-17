using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using Cozmo.UI;
using DataPersistence;
using System.Linq;
using System;

namespace Cozmo.HomeHub {
  public class HomeView : BaseView {

    public enum HomeTab {
      Cozmo,
      Play,
      Profile
    }

    public System.Action<StatContainer, StatContainer, Transform[]> DailyGoalsSet;

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
    private CozmoButton _CozmoTabDownButton;

    [SerializeField]
    private GameObject _AnyUpgradeAffordableIndicator;

    [SerializeField]
    private CozmoButton _PlayTabButton;

    [SerializeField]
    private CozmoButton _PlayTabDownButton;

    [SerializeField]
    private CozmoButton _ProfileTabButton;

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
    private UnityEngine.UI.Text _CurrentRequirementPointsLabel;

    [SerializeField]
    private UnityEngine.UI.Text _RequirementPointsNeededLabel;

    [SerializeField]
    private UnityEngine.UI.ScrollRect _ScrollRect;

    [SerializeField]
    private LootView _LootViewPrefab;
    private LootView _LootViewInstance = null;

    [SerializeField]
    private AnkiTextLabel _DailyGoalsCompletionText;

    [SerializeField]
    private AnkiTextLabel _DailyGaolsCompletionTextDown;

    private HomeHub _HomeHubInstance;

    public HomeHub HomeHubInstance {
      get { return _HomeHubInstance; }
      private set { _HomeHubInstance = value; }
    }

    public delegate void ButtonClickedHandler(string challengeClicked, Transform buttonTransform);

    public event ButtonClickedHandler OnLockedChallengeClicked;
    public event ButtonClickedHandler OnUnlockedChallengeClicked;
    public event ButtonClickedHandler OnCompletedChallengeClicked;
    public event Action OnEndSessionClicked;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStates;

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById, HomeHub homeHubInstance) {
      _HomeHubInstance = homeHubInstance;

      DASEventViewName = "home_view";

      _ChallengeStates = challengeStatesById;

      HandlePlayTabButton();

      _CozmoTabButton.Initialize(HandleCozmoTabButton, "switch_to_cozmo_tab_button", DASEventViewName);
      _PlayTabButton.Initialize(HandlePlayTabButton, "switch_to_play_tab_button", DASEventViewName);
      _ProfileTabButton.Initialize(HandleProfileTabButton, "switch_to_profile_tab_button", DASEventViewName);

      _CozmoTabDownButton.Initialize(HandleCozmoTabButton, "switch_to_cozmo_tab_button", DASEventViewName);
      _PlayTabDownButton.Initialize(HandlePlayTabButton, "switch_to_play_tab_button", DASEventViewName);
      _ProfileTabDownButton.Initialize(HandleProfileTabButton, "switch_to_profile_tab_button", DASEventViewName);

      ChestRewardManager.Instance.ChestRequirementsGained += HandleChestRequirementsGained;
      ChestRewardManager.Instance.ChestGained += HandleChestGained;
      _RequirementPointsProgressBar.ProgressUpdateCompleted += HandleProgressUpdated;
      if (ChestRewardManager.Instance.ChestPending) {
        HandleChestGained();
      }
      else {
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());
      }

      GameEventManager.Instance.OnGameEvent += HandleDailyGoalCompleted;
      UpdatePlayTabText();

      Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded += HandleItemValueChanged;
      CheckIfUnlockablesAffordableAndUpdateBadge();
    }

    private void HandleChestGained() {
      UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetCurrentRequirementPoints());
    }

    // Opens loot view and fires and relevant events
    private void OpenLootView() {
      if (_LootViewInstance != null) {
        // Avoid dupes
        return;
      }
      if (_LootViewPrefab == null) {
        DAS.Error("HomeView.OpenLootView", "LootViewPrefab is NULL");
        return;
      }

      LootView alertView = UIManager.OpenView(_LootViewPrefab);
      alertView.LootBoxRewards = ChestRewardManager.Instance.PendingRewards;
      _LootViewInstance = alertView;
      _LootViewInstance.ViewCloseAnimationFinished += (() =>
        UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints()));
    }

    private void HandleChestRequirementsGained(int currentPoints, int numPointsNeeded) {
      // Ignore updating if we are in the process of showing a new LootView
      if (ChestRewardManager.Instance.ChestPending == false) {
        UpdateChestProgressBar(currentPoints, numPointsNeeded);
      }
    }

    private void UpdateChestProgressBar(int currentPoints, int numPointsNeeded) {
      float progress = ((float)currentPoints / (float)numPointsNeeded);
      _RequirementPointsProgressBar.SetProgress(progress);
      _CurrentRequirementPointsLabel.text = currentPoints.ToString();
      _RequirementPointsNeededLabel.text = numPointsNeeded.ToString();
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

    private void HandleProgressUpdated() {
      if (ChestRewardManager.Instance.ChestPending) {
        OpenLootView();
      }
    }

    public void SetChallengeStates(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      _ChallengeStates = challengeStatesById;
    }

    private void HandleCozmoTabButton() {
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
      ClearCurrentTab();
      ShowNewCurrentTab(_ProfileTabPrefab);
      UpdateTabGraphics(HomeTab.Profile);
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
    }

    private void UpdateTabGraphics(HomeTab currentTab) {
      _CozmoTabButton.gameObject.SetActive(currentTab != HomeTab.Cozmo);
      _CozmoTabDownButton.gameObject.SetActive(currentTab == HomeTab.Cozmo);

      _PlayTabButton.gameObject.SetActive(currentTab != HomeTab.Play);
      _PlayTabDownButton.gameObject.SetActive(currentTab == HomeTab.Play);

      _ProfileTabButton.gameObject.SetActive(currentTab != HomeTab.Profile);
      _ProfileTabDownButton.gameObject.SetActive(currentTab == HomeTab.Profile);
    }

    public Dictionary<string, ChallengeStatePacket> GetChallengeStates() {
      return _ChallengeStates;
    }

    public void HandleLockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnLockedChallengeClicked != null) {
        OnLockedChallengeClicked(challengeClicked, buttonTransform);
      }
    }

    public void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
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
      CheckIfUnlockablesAffordableAndUpdateBadge();
    }

    private void CheckIfUnlockablesAffordableAndUpdateBadge() {
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

    protected override void CleanUp() {
      ChestRewardManager.Instance.ChestRequirementsGained -= HandleChestRequirementsGained;
      ChestRewardManager.Instance.ChestGained -= HandleChestGained;
      _RequirementPointsProgressBar.ProgressUpdateCompleted -= HandleProgressUpdated;
      GameEventManager.Instance.OnGameEvent -= HandleDailyGoalCompleted;

      Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded -= HandleItemValueChanged;
    }
  }
}