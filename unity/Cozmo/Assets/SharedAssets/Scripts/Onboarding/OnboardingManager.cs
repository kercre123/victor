using System;
using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo;
using Cozmo.HomeHub;
using DataPersistence;
using Onboarding;
using Anki.Assets;

public class OnboardingManager : MonoBehaviour {

  public static OnboardingManager Instance { get; private set; }

  private HomeView _HomeView;

  public enum OnboardingPhases : int {
    Home,
    Loot,
    Upgrades,
    DailyGoals,
    None
  };

  private OnboardingPhases _CurrPhase = OnboardingPhases.None;

  private GameObject _CurrStageInst = null;

  private Transform _OnboardingTransform;

  [SerializeField]
  private int _NumStagesHome = 7;
  [SerializeField]
  private int _NumStagesLoot = 1;
  [SerializeField]
  private int _NumStagesUpgrades = 1;
  [SerializeField]
  private GameObjectDataLink _OnboardingUIPrefabData;
  private OnboardingUIWrapper _OnboardingUIInstance;

  private void OnEnable() {
    if (Instance != null && Instance != this) {
      SetSpecificStage(-1);
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
      GameEventManager.Instance.OnGameEvent += HandleDailyGoalCompleted;
      Anki.Debug.DebugConsoleData.Instance.AddConsoleFunction("Fill Loot Energy", "Unity", GiveEnergy);
    }
  }

  private int GetCurrStageInPhase(OnboardingPhases phase) {
    if (phase == OnboardingPhases.None) {
      return 0;
    }

    PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
    if (!profile.OnboardingStages.ContainsKey(phase)) {
      profile.OnboardingStages[phase] = 0;
    }
    return profile.OnboardingStages[phase];
  }
  private void SetCurrStageInPhase(int stage, OnboardingPhases phase) {
    if (phase != OnboardingPhases.None) {
      PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
      profile.OnboardingStages[phase] = stage;
    }
  }

  private int GetMaxStageInPhase(OnboardingPhases phase) {
    int numPhases = 0;
    switch (phase) {
    case OnboardingPhases.Home:
      numPhases = _NumStagesHome;
      break;
    case OnboardingPhases.Loot:
      numPhases = _NumStagesLoot;
      break;
    case OnboardingPhases.Upgrades:
      numPhases = _NumStagesUpgrades;
      break;
    // Daily goals is a special case where it is just used to save state.
    case OnboardingPhases.DailyGoals:
      numPhases = _NumStagesUpgrades;
      break;
    }
    if (_OnboardingUIInstance != null) {
      // A logical check but we don't want to load the whole asset bundle if we don't have to
      int numPrefabs = _OnboardingUIInstance.GetMaxStageInPhase(phase);
      if (numPhases != numPrefabs) {
        DAS.Error("onboarding.setuperror.PrefabPhasesMismatch", "");
      }
    }
    return numPhases;
  }
  private OnboardingBaseStage GetCurrStagePrefab() {
    int currStage = GetCurrStageInPhase(_CurrPhase);
    if (_OnboardingUIInstance != null) {
      return _OnboardingUIInstance.GetCurrStagePrefab(currStage, _CurrPhase);
    }
    return null;
  }

  public bool IsOnboardingRequired(OnboardingPhases phase) {
    return GetCurrStageInPhase(phase) < GetMaxStageInPhase(phase);
  }

  public void InitHomeHubOnboarding(HomeView homeview, Transform onboardingLayer) {
    _HomeView = homeview;
    _OnboardingTransform = onboardingLayer;

    if (IsOnboardingRequired(OnboardingPhases.Home)) {
      StartPhase(OnboardingPhases.Home);
    }
  }

  public void StartPhase(OnboardingPhases phase) {
    // End any previous phase, can only highlight one thing at once
    if (_CurrPhase != OnboardingPhases.None) {
      SetSpecificStage(GetMaxStageInPhase(_CurrPhase));
    }
    _CurrPhase = phase;
    if (_CurrPhase == OnboardingPhases.Home) {
      RobotEngineManager.Instance.CurrentRobot.PushIdleAnimation(AnimationTrigger.OnboardingIdle);
    }

    // we keep the asset bundle around because some phases lead into each other, in common cases
    // but don't have to.
    if (_OnboardingUIInstance == null) {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_OnboardingUIPrefabData.AssetBundle, LoadOnboardingAssetsCallback);
    }
    else {
      SetSpecificStage(0);
    }
  }
  public void CompletePhase(OnboardingPhases phase) {
    // If it's the current phase clean up UI.
    // Otherwise just set the save file forward.
    if (phase == _CurrPhase) {
      SetSpecificStage(GetMaxStageInPhase(_CurrPhase));
    }
    else {
      SetCurrStageInPhase(GetMaxStageInPhase(phase), phase);
      DataPersistenceManager.Instance.Save();
    }
  }

  private void PhaseCompletedInternal() {
    if (_CurrPhase == OnboardingPhases.Home) {
      RobotEngineManager.Instance.CurrentRobot.PopIdleAnimation();
    }
  }

  private void LoadOnboardingAssetsCallback(bool assetBundleSuccess) {
    _OnboardingUIPrefabData.LoadAssetData((GameObject onboardingUIWrapperPrefab) => {
      if (_OnboardingUIInstance == null && onboardingUIWrapperPrefab != null) {
        GameObject wrapper = UIManager.CreateUIElement(onboardingUIWrapperPrefab.gameObject);
        _OnboardingUIInstance = wrapper.GetComponent<OnboardingUIWrapper>();
        SetSpecificStage(0);
      }
    });
  }
  private void UnloadIfDoneWithAllPhases() {
    if (!IsOnboardingRequired(OnboardingPhases.Home) &&
        !IsOnboardingRequired(OnboardingPhases.Loot) &&
        !IsOnboardingRequired(OnboardingPhases.Upgrades)) {
      AssetBundleManager.Instance.UnloadAssetBundle(_OnboardingUIPrefabData.AssetBundle);
      if (_OnboardingUIInstance != null) {
        GameObject.Destroy(_OnboardingUIInstance);
        _OnboardingUIInstance = null;
      }
    }
  }

  public void GoToNextStage() {
    SetSpecificStage(GetCurrStageInPhase(_CurrPhase) + 1);
  }

  public void SetSpecificStage(int nextStage) {
    if (_CurrStageInst != null) {
      GameObject.Destroy(_CurrStageInst);
    }

    SetCurrStageInPhase(nextStage, _CurrPhase);
    if (nextStage >= 0 && nextStage < GetMaxStageInPhase(_CurrPhase)) {
      OnboardingBaseStage stagePrefab = GetCurrStagePrefab();

      _CurrStageInst = UIManager.CreateUIElement(stagePrefab, _OnboardingTransform);
      UpdateStage(stagePrefab.ActiveTopBar, stagePrefab.ActiveMenuContent,
                  stagePrefab.ActiveTabButtons, stagePrefab.ReactionsEnabled);
      // Create the debug layer to have a few buttons to work with on screen easily for QA
      // who will have to see this all the time.
      _OnboardingUIInstance.AddDebugButtons();
    }
    else {
      PhaseCompletedInternal();
      _OnboardingUIInstance.RemoveDebugButtons();
      _CurrPhase = OnboardingPhases.None;
      UpdateStage();
      HomeHub.Instance.StartFreeplay(RobotEngineManager.Instance.CurrentRobot);
      UnloadIfDoneWithAllPhases();
    }

    DataPersistenceManager.Instance.Save();
  }

  public HomeView GetHomeView() {
    return _HomeView;
  }

  private void HandleDailyGoalCompleted(GameEventWrapper gameEvent) {
    if (gameEvent.GameEventEnum == GameEvent.OnDailyGoalCompleted) {
      // If all completed, complete the tutorial and get new goals the next day...
      if (DataPersistenceManager.Instance.CurrentSession != null) {
        List<Cozmo.UI.DailyGoal> goals = DataPersistenceManager.Instance.CurrentSession.DailyGoals;
        bool allGoalsComplete = goals.TrueForAll(x => x.GoalComplete);
        if (allGoalsComplete) {
          CompletePhase(OnboardingPhases.DailyGoals);
        }
      }
    }
  }

  #region DEBUG

  public void GiveEnergy(string param) {
    int energy = ChestRewardManager.Instance.GetNextRequirementPoints();
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount("experience", energy);
  }

  public void DebugSkipOne() {
    OnboardingManager.Instance.SkipPressed();
  }

  public void DebugSkipAll() {
    OnboardingManager.Instance.SetSpecificStage(GetMaxStageInPhase(Instance._CurrPhase));
  }
  private void SkipPressed() {
    if (_CurrStageInst != null) {
      OnboardingBaseStage stage = _CurrStageInst.GetComponent<OnboardingBaseStage>();
      if (stage) {
        stage.SkipPressed();
      }
    }
  }
  #endregion

  private void UpdateStage(bool showTopBar = true, bool showContent = true, bool showButtons = true, bool reactionsEnabled = true) {
    if (_HomeView) {
      _HomeView.TopBarContainer.gameObject.SetActive(showTopBar);
      _HomeView.TabContentContainer.gameObject.SetActive(showContent);
      _HomeView.TabButtonContainer.gameObject.SetActive(showButtons);
    }
    RobotEngineManager.Instance.CurrentRobot.EnableReactionaryBehaviors(reactionsEnabled);
  }

}