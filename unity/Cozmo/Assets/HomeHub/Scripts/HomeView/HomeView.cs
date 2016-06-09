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
      UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());

      GameEventManager.Instance.OnGameEvent += HandleDailyGoalCompleted;
      UpdatePlayTabText();
    }

    private void HandleChestGained() {
      UpdateChestProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());
      OpenLootView();
    }

    // Opens loot view and fires and relevant events
    private void OpenLootView() {
      if (_LootViewInstance != null) {
        // Avoid dupes
        return;
      }

      // Create alert view with Icon
      LootView alertView = UIManager.OpenView(_LootViewPrefab);
      _LootViewInstance = alertView;
    }

    private void HandleChestRequirementsGained(int currentPoints, int numPointsNeeded) {
      UpdateChestProgressBar(currentPoints, numPointsNeeded);
    }

    private void UpdateChestProgressBar(int currentPoints, int numPointsNeeded) {
      _RequirementPointsProgressBar.SetProgress((float)currentPoints / (float)numPointsNeeded);
      _CurrentRequirementPointsLabel.text = currentPoints.ToString();
      _RequirementPointsNeededLabel.text = numPointsNeeded.ToString();
    }

    public void SetChallengeStates(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      _ChallengeStates = challengeStatesById;
    }

    private void HandleCozmoTabButton() {
      if (_CurrentTab != null) {
        GameObject.Destroy(_CurrentTab.gameObject);
      }
      _ScrollRect.horizontalNormalizedPosition = 0.0f;
      _CurrentTab = GameObject.Instantiate(_CozmoTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);

      _CozmoTabButton.gameObject.SetActive(false);
      _CozmoTabDownButton.gameObject.SetActive(true);

      _PlayTabButton.gameObject.SetActive(true);
      _PlayTabDownButton.gameObject.SetActive(false);

      _ProfileTabButton.gameObject.SetActive(true);
      _ProfileTabDownButton.gameObject.SetActive(false);
    }

    private void HandlePlayTabButton() {
      if (_CurrentTab != null) {
        GameObject.Destroy(_CurrentTab.gameObject);
      }
      _ScrollRect.horizontalNormalizedPosition = 0.0f;
      _CurrentTab = GameObject.Instantiate(_PlayTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);

      _CozmoTabButton.gameObject.SetActive(true);
      _CozmoTabDownButton.gameObject.SetActive(false);

      _PlayTabButton.gameObject.SetActive(false);
      _PlayTabDownButton.gameObject.SetActive(true);

      _ProfileTabButton.gameObject.SetActive(true);
      _ProfileTabDownButton.gameObject.SetActive(false);
    }

    private void HandleProfileTabButton() {
      if (_CurrentTab != null) {
        GameObject.Destroy(_CurrentTab.gameObject);
      }
      _ScrollRect.horizontalNormalizedPosition = 0.0f;
      _CurrentTab = GameObject.Instantiate(_ProfileTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);

      _CozmoTabButton.gameObject.SetActive(true);
      _CozmoTabDownButton.gameObject.SetActive(false);

      _PlayTabButton.gameObject.SetActive(true);
      _PlayTabDownButton.gameObject.SetActive(false);

      _ProfileTabButton.gameObject.SetActive(false);
      _ProfileTabDownButton.gameObject.SetActive(true);
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

    protected override void CleanUp() {
      ChestRewardManager.Instance.ChestRequirementsGained -= HandleChestRequirementsGained;
      ChestRewardManager.Instance.ChestGained -= HandleChestGained;
      GameEventManager.Instance.OnGameEvent -= HandleDailyGoalCompleted;
    }

  }
}