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

  public Action<OnboardingPhases, int> OnOnboardingStageStarted;

  public bool FirstTime { get; set; }

  private OnboardingPhases _CurrPhase = OnboardingPhases.None;

  private GameObject _CurrStageInst = null;

  private Transform _OnboardingTransform;

  [SerializeField]
  private int _NumStagesHome = 9;
  [SerializeField]
  private int _NumStagesLoot = 2;
  [SerializeField]
  private int _NumStagesUpgrades = 1;
  [SerializeField]
  private GameObjectDataLink _OnboardingUIPrefabData;
  private OnboardingUIWrapper _OnboardingUIInstance;

  // The DAS phaseIDs are checkpoints from the design doc.
  private int _CurrDASPhaseID = -1;
  private float _CurrDASPhaseStartTime = 0;

  #region OUTLINEAREA
  // Attaches a small outline around a region of the UI
  // So we don't need to do any resolution math in the overlay.
  private Transform _OutlineRegionTransform;
  private GameObject _UIOutlineInstance;
  public void SetOutlineRegion(Transform outlineRegion) {
    _OutlineRegionTransform = outlineRegion;
  }
  public void ShowOutlineRegion(bool enable, bool useLargerOutline = false) {
    if (_UIOutlineInstance != null) {
      GameObject.Destroy(_UIOutlineInstance);
    }
    if (enable && _OutlineRegionTransform != null) {
      _UIOutlineInstance = UIManager.CreateUIElement(useLargerOutline ? _OnboardingUIInstance.GetOutlineLargePrefab() : _OnboardingUIInstance.GetOutlinePrefab(), _OutlineRegionTransform);
    }
  }
  #endregion

#if UNITY_EDITOR
  private bool _DebugDisplayOn = true;
#else
  private bool _DebugDisplayOn = false;
#endif

  private void OnEnable() {
    if (Instance != null && Instance != this) {
      SetSpecificStage(-1);
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
      GameEventManager.Instance.OnGameEvent += HandleDailyGoalCompleted;
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleRobotDisconnected);
      Anki.Debug.DebugConsoleData.Instance.AddConsoleFunction("Toggle Onboarding Debug Display", "Onboarding", ToggleOnboardingDebugDisplay);
      Anki.Debug.DebugConsoleData.Instance.AddConsoleFunction("Complete All Onboarding", "Onboarding", DebugCompleteAllOnboarding);
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
  public bool IsAnyOnboardingActive() {
    return _CurrPhase != OnboardingPhases.None;
  }

  public void InitHomeHubOnboarding(HomeView homeview) {
    _HomeView = homeview;
    _OnboardingTransform = homeview.transform;
    _HomeView.ViewClosed += HandleHomeViewClosed;

    if (IsOnboardingRequired(OnboardingPhases.Home)) {
      StartPhase(OnboardingPhases.Home);
    }
  }

  // Clear out the phase if the homeview closed unexpected like a disconnect.
  // The UI is destroyed but our save state will reinit us next time.
  private void HandleHomeViewClosed() {
    if (_CurrPhase != OnboardingPhases.None) {
      _CurrPhase = OnboardingPhases.None;
      if (_OnboardingUIInstance != null) {
        _OnboardingUIInstance.RemoveDebugButtons();
      }
    }
  }

  public void StartPhase(OnboardingPhases phase) {
#if SDK_ONLY
    return;
#else
    // End any previous phase, can only highlight one thing at once
    if (_CurrPhase != OnboardingPhases.None) {
      SetSpecificStage(GetMaxStageInPhase(_CurrPhase));
    }
    // Going to disconnect soon and won't be able to clean up a lot of onboarding stuff without a robot
    IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
    if (CurrentRobot == null) {
      return;
    }
    int startStage = 0;
    _CurrPhase = phase;
    if (_CurrPhase == OnboardingPhases.Home) {
      CurrentRobot.PushIdleAnimation(AnimationTrigger.OnboardingIdle);
      CurrentRobot.RequestEnableReactionaryBehavior("onboardingHome", BehaviorType.ReactToOnCharger, false);
      bool isOldRobot = UnlockablesManager.Instance.IsUnlocked(UnlockId.StackTwoCubes);
      Cozmo.PauseManager.Instance.IsIdleTimeOutEnabled = false;

      DAS.Event("onboarding.start", isOldRobot ? "1" : "0");
      // in the event they've ever booted the app before, or it's an old robot.
      // Skip the holding on charger phase because it is the worst and only should be a problem
      // for fresh from factory robots
      if (!FirstTime || isOldRobot) {
        startStage = 1;
      }

      // This is safe because you can't spend currency in the first phase of onboarding
      Cozmo.ItemData itemData = Cozmo.ItemDataConfig.GetHexData();
      int currAmt = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.GetItemAmount(RewardedActionManager.Instance.CoinID);
      if (currAmt < itemData.StartingAmount) {
        DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.SetItemAmount(RewardedActionManager.Instance.CoinID, itemData.StartingAmount);
      }
      // Hex piece bits are their own thing because they will be puzzle pieces..
      List<string> itemIDs = Cozmo.ItemDataConfig.GetAllItemIds();
      for (int i = 0; i < itemIDs.Count; ++i) {
        itemData = Cozmo.ItemDataConfig.GetData(itemIDs[i]);
        currAmt = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.GetItemAmount(itemData.ID);
        if (currAmt < itemData.StartingAmount) {
          DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.SetItemAmount(itemData.ID, itemData.StartingAmount);
        }
      }
    } // end first phase complete
    CurrentRobot.SetAvailableGames(BehaviorGameFlag.NoGame);

    // If assets are already loaded, go otherwise this will wait for callback.
    // It should always be loaded now
    if (PreloadOnboarding()) {
      SetSpecificStage(startStage);
    }
#endif
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
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior("onboardingHome", BehaviorType.ReactToOnCharger, true);
        RobotEngineManager.Instance.CurrentRobot.PopIdleAnimation();
      }
      Cozmo.PauseManager.Instance.IsIdleTimeOutEnabled = true;
    }
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.All);
    }
    ShowOutlineRegion(false);
  }

  public bool PreloadOnboarding() {
    if (_OnboardingUIInstance == null) {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_OnboardingUIPrefabData.AssetBundle, LoadOnboardingAssetsCallback);
      return false;
    }
    return true;
  }
  private void LoadOnboardingAssetsCallback(bool assetBundleSuccess) {
    if (assetBundleSuccess) {
      _OnboardingUIPrefabData.LoadAssetData((GameObject onboardingUIWrapperPrefab) => {
        if (_OnboardingUIInstance == null && onboardingUIWrapperPrefab != null) {
          GameObject wrapper = UIManager.CreateUIElement(onboardingUIWrapperPrefab.gameObject);
          _OnboardingUIInstance = wrapper.GetComponent<OnboardingUIWrapper>();
          if (_CurrPhase != OnboardingPhases.None) {
            SetSpecificStage(0);
          }
        }
      });
    }
    else {
      DAS.Error("OnboardingManager.LoadOnboardingAssetsCallback", "Failed to load asset bundle " + _OnboardingUIPrefabData.AssetBundle);
    }
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
    int nextDASPhaseID = -1;
    SetCurrStageInPhase(nextStage, _CurrPhase);
    if (nextStage >= 0 && nextStage < GetMaxStageInPhase(_CurrPhase)) {
      OnboardingBaseStage stagePrefab = GetCurrStagePrefab();
      // likely OnboardingUIWrapper.prefab needs to be updated if this ever happens legit
      if (stagePrefab == null) {
        DAS.Error("onboardingmanager.SetSpecificStage", "NO PREFAB SET FOR STAGE " + nextStage + " of phase " + _CurrPhase);
        _CurrPhase = OnboardingPhases.None;
        return;
      }
      nextDASPhaseID = stagePrefab.DASPhaseID;
      _CurrStageInst = UIManager.CreateUIElement(stagePrefab, _OnboardingTransform);
      if (OnOnboardingStageStarted != null) {
        OnOnboardingStageStarted.Invoke(_CurrPhase, nextStage);
      }
      UpdateStage(stagePrefab.ActiveTopBar, stagePrefab.ActiveBotBar, stagePrefab.ActiveMenuContent,
                  stagePrefab.ActiveTabButtons, stagePrefab.ReactionsEnabled);
      // Create the debug layer to have a few buttons to work with on screen easily for QA
      // who will have to see this all the time.
      if (Instance._DebugDisplayOn) {
        _OnboardingUIInstance.AddDebugButtons();
      }
    }
    else {
      PhaseCompletedInternal();
      _OnboardingUIInstance.RemoveDebugButtons();
      _CurrPhase = OnboardingPhases.None;
      UpdateStage();
      HomeHub.Instance.StartFreeplay(RobotEngineManager.Instance.CurrentRobot);
      UnloadIfDoneWithAllPhases();
    }
    // Just started something new...
    if (_CurrDASPhaseID != nextDASPhaseID) {
      // Not first time, record a transition out
      if (_CurrDASPhaseID != -1) {
        float timeSinceLastPhase = Time.time - _CurrDASPhaseStartTime;
        DAS.Event("onboarding.phase_time", _CurrDASPhaseID.ToString(), DASUtil.FormatExtraData(timeSinceLastPhase.ToString()));
      }

      // start recording the next one...
      _CurrDASPhaseStartTime = Time.time;
      _CurrDASPhaseID = nextDASPhaseID;
    }
    DataPersistenceManager.Instance.Save();
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

  private void HandleRobotDisconnected(Anki.Cozmo.ExternalInterface.RobotDisconnected message) {
    // The UI is getting torn down and we're resetting, clear whatever happened.
    _CurrPhase = OnboardingPhases.None;
    if (_DebugDisplayOn && _OnboardingUIInstance != null) {
      _OnboardingUIInstance.RemoveDebugButtons();
    }
  }

  #region DEBUG
  public void ToggleOnboardingDebugDisplay(string param) {
    Instance._DebugDisplayOn = !Instance._DebugDisplayOn;
    if (Instance._OnboardingUIInstance != null) {
      if (Instance._DebugDisplayOn) {
        Instance._OnboardingUIInstance.AddDebugButtons();
      }
      else {
        Instance._OnboardingUIInstance.RemoveDebugButtons();
      }
    }
  }

  public void DebugCompleteAllOnboarding(string param) {
    OnboardingManager.Instance.CompletePhase(OnboardingPhases.Home);
    OnboardingManager.Instance.CompletePhase(OnboardingPhases.DailyGoals);
    OnboardingManager.Instance.CompletePhase(OnboardingPhases.Loot);
    OnboardingManager.Instance.CompletePhase(OnboardingPhases.Upgrades);
  }

  public void DebugSkipOne() {
    OnboardingManager.Instance.SkipPressed();
  }

  public void DebugSkipAll() {
    // special debug case. Since this loot view is listening for a state starting it hangs so just let it start loot.
    if (Instance._CurrPhase == OnboardingPhases.Loot && Instance.GetCurrStageInPhase(OnboardingPhases.Loot) == 0) {
      Instance.GoToNextStage();
    }
    else {
      if (Instance._CurrPhase == OnboardingPhases.Home) {
        UIManager.Instance.BackgroundColorController.SetBackgroundColor(Cozmo.UI.BackgroundColorController.BackgroundColor.TintMe, Color.white);
      }
      OnboardingManager.Instance.SetSpecificStage(GetMaxStageInPhase(Instance._CurrPhase));
    }
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

  private void UpdateStage(bool showTopBar = true, bool showBotBar = true, bool showContent = true, bool showButtons = true, bool reactionsEnabled = true) {
    if (_HomeView) {
      _HomeView.TopBarContainer.gameObject.SetActive(showTopBar);
      _HomeView.BottomBarContainer.gameObject.SetActive(showBotBar);
      _HomeView.TabContentContainer.gameObject.SetActive(showContent);
      _HomeView.TabButtonContainer.gameObject.SetActive(showButtons);
    }
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.EnableReactionaryBehaviors(reactionsEnabled);
    }
  }

}
