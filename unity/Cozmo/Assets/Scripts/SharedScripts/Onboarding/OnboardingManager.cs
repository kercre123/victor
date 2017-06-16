using Anki.Assets;
using Anki.Cozmo;
using Cozmo.RequestGame;
using DataPersistence;
using Onboarding;
using System;
using System.Collections.Generic;
using Cozmo.Needs.UI;
using Cozmo.Challenge;
using UnityEngine;

public class OnboardingManager : MonoBehaviour {

  public static OnboardingManager Instance { get; private set; }
  private NeedsHubView _NeedsHubView;

  public enum OnboardingPhases : int {
    InitialSetup,
    MeetCozmo,
    NurtureIntro, // includes repair
    FeedIntro,
    PlayIntro,
    RewardBox,
    DiscoverIntro,
    VoiceCommands,
    // Start OLD
    Home,
    Loot,
    Upgrades,
    DailyGoals,
    // End Old
    None
  };

  public static readonly OnboardingPhases[] kRequiredPhases = { OnboardingPhases.InitialSetup,
                                    OnboardingPhases.MeetCozmo, OnboardingPhases.NurtureIntro,OnboardingPhases.FeedIntro,
                                    OnboardingPhases.PlayIntro, OnboardingPhases.RewardBox,OnboardingPhases.DiscoverIntro,
                                    OnboardingPhases.VoiceCommands };

  public Action<OnboardingPhases, int> OnOnboardingStageStarted;
  public Action<OnboardingPhases> OnOnboardingPhaseCompleted;

  public bool FirstTime { get; set; }

  private OnboardingPhases _CurrPhase = OnboardingPhases.None;

  private GameObject _CurrStageInst = null;

  private Transform _OnboardingTransform;

  private bool _StageDisabledReactionaryBehaviors = false;

  [SerializeField]
  private GameObjectDataLink _OnboardingUIPrefabData;
  private OnboardingUIWrapper _OnboardingUIInstance;

  [SerializeField]
  private List<int> _NumOnboardingStages;

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
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleRobotDisconnected);
#if ENABLE_DEBUG_PANEL
      Anki.Debug.DebugConsoleData.Instance.AddConsoleFunction("Toggle Onboarding Debug Display", "Onboarding", ToggleOnboardingDebugDisplay);
      Anki.Debug.DebugConsoleData.Instance.AddConsoleFunction("Complete All Onboarding", "Onboarding", DebugCompleteAllOnboarding);
#endif
      // "Home" was the old tutorial, if they had it, just let them skip first part of new tutorial.
      if (IsReturningUser()) {
        PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
        profile.OnboardingStages[OnboardingPhases.InitialSetup] = GetMaxStageInPhase(OnboardingPhases.InitialSetup);
        profile.OnboardingStages[OnboardingPhases.MeetCozmo] = GetMaxStageInPhase(OnboardingPhases.MeetCozmo);
      }

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
    if ((int)phase < _NumOnboardingStages.Count) {
      numPhases = _NumOnboardingStages[(int)phase];
    }
    if (_OnboardingUIInstance != null) {
      // A logical check but we don't want to load the whole asset bundle if we don't have to
      int numPrefabs = _OnboardingUIInstance.GetMaxStageInPhase(phase);
      if (numPhases != numPrefabs) {
        DAS.Error("onboarding.setuperror.PrefabPhasesMismatch", phase + " prefabs: " + numPrefabs + " manager: " + numPhases);
        numPhases = numPrefabs;
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
  public bool IsReturningUser() {
    PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
    int returningUserStage = 0;
    if (profile.OnboardingStages.TryGetValue(OnboardingPhases.Home, out returningUserStage)) {
      return returningUserStage > 0;
    }
    return false;
  }

  public void InitInitalOnboarding(NeedsHubView needsHubView) {
    _NeedsHubView = needsHubView;
    _NeedsHubView.DialogClosed += HandleHomeViewClosed;
    _OnboardingTransform = _NeedsHubView.transform.parent.transform;

    if (IsOnboardingRequired(OnboardingPhases.InitialSetup)) {
      StartPhase(OnboardingPhases.InitialSetup);
    }
  }

  // Clear out the phase if the homeview closed unexpected like a disconnect.
  // The UI is destroyed but our save state will reinit us next time.
  private void HandleHomeViewClosed() {
    if (_CurrPhase != OnboardingPhases.None) {
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
      // None means we just want to clear everything so don't start something new
      if (_CurrStageInst != null) {
        GameObject.Destroy(_CurrStageInst);
      }
    }
    // Going to disconnect soon and won't be able to clean up a lot of onboarding stuff without a robot
    IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
    if (CurrentRobot == null) {
      return;
    }
    int startStage = 0;
    _CurrPhase = phase;
    if (_CurrPhase == OnboardingPhases.InitialSetup) {
      CurrentRobot.PushIdleAnimation(AnimationTrigger.OnboardingIdle);
      RobotEngineManager.Instance.CurrentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kOnboardingHomeId, ReactionaryBehaviorEnableGroups.kOnboardingHomeTriggers);
      bool isOldRobot = UnlockablesManager.Instance.IsUnlocked(UnlockId.StackTwoCubes);
      Cozmo.PauseManager.Instance.IsIdleTimeOutEnabled = false;

      DAS.Event("onboarding.start", isOldRobot ? "1" : "0");
      // in the event they've ever booted the app before, or it's an old robot.
      // Skip the holding on charger phase because it is the worst and only should be a problem
      // for fresh from factory robots
      if (!FirstTime || isOldRobot) {
        startStage = 1;
      }
      // In demo mode skip to wake up
      if (DebugMenuManager.Instance.DemoMode) {
        startStage = 2;
      }
    } // end first phase complete
    RequestGameManager.Instance.DisableRequestGameBehaviorGroups();

    Cozmo.Needs.NeedsStateManager.Instance.PauseExceptForNeed(NeedId.Count);

    // If assets are already loaded, go otherwise this will wait for callback.
    // It should always be loaded now
    if (PreloadOnboarding()) {
      SetSpecificStage(startStage);
    }
#endif
  }
  public void CompletePhase(OnboardingPhases phase, bool canStartNewPhase = true) {
    // If it's the current phase clean up UI.
    // Otherwise just set the save file forward.
    if (phase == _CurrPhase) {
      SetSpecificStage(GetMaxStageInPhase(_CurrPhase), canStartNewPhase);
    }
    else {
      SetCurrStageInPhase(GetMaxStageInPhase(phase), phase);
      DataPersistenceManager.Instance.Save();
    }
  }

  private void PhaseCompletedInternal() {
    if (OnOnboardingPhaseCompleted != null) {
      OnOnboardingPhaseCompleted.Invoke(_CurrPhase);
    }
    if (_CurrPhase == OnboardingPhases.InitialSetup) {
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kOnboardingHomeId);
        RobotEngineManager.Instance.CurrentRobot.PopIdleAnimation();
      }
      Cozmo.PauseManager.Instance.IsIdleTimeOutEnabled = true;
    }
    Cozmo.Needs.NeedsStateManager.Instance.ResumeAllNeeds();
    RequestGameManager.Instance.EnableRequestGameBehaviorGroups();
    ShowOutlineRegion(false);
  }

  public void StartAnyPhaseIfNeeded() {
    if (!IsOnboardingRequired(OnboardingPhases.InitialSetup) && IsOnboardingRequired(OnboardingPhases.MeetCozmo)) {
      HubWorldBase instance = HubWorldBase.Instance;
      if (instance != null && instance is Cozmo.Hub.NeedsHub) {
        Cozmo.Hub.NeedsHub homeHubInstance = (Cozmo.Hub.NeedsHub)instance;

        ChallengeData data = Array.Find(ChallengeDataList.Instance.ChallengeData,
                                      (ChallengeData obj) => { return obj.UnlockId.Value == UnlockId.MeetCozmoGame; });
        // "FaceEnrollmentTest"
        if (data != null) {
          homeHubInstance.ForceStartChallenge(data.ChallengeID);
        }
      }
    }

    if (!IsOnboardingRequired(OnboardingPhases.MeetCozmo) && IsOnboardingRequired(OnboardingPhases.NurtureIntro)) {
      StartPhase(OnboardingPhases.NurtureIntro);
    }

    if (!IsOnboardingRequired(OnboardingPhases.NurtureIntro) && IsOnboardingRequired(OnboardingPhases.FeedIntro)) {
      StartPhase(OnboardingPhases.FeedIntro);
    }

    if (!IsOnboardingRequired(OnboardingPhases.FeedIntro) && IsOnboardingRequired(OnboardingPhases.PlayIntro)) {
      StartPhase(OnboardingPhases.PlayIntro);
    }
  }

  public bool PreloadOnboarding() {
    bool allCompleted = Array.TrueForAll(kRequiredPhases, (OnboardingPhases phase) => { return !IsOnboardingRequired(phase); });
    if (allCompleted) {
      return false;
    }
    if (_OnboardingUIInstance == null) {
      AssetBundleManager.Instance.LoadAssetBundleAsync(_OnboardingUIPrefabData.AssetBundle, LoadOnboardingAssetsCallback);
      return false;
    }
    return true;
  }
  // spam clicking tabs, usually we lock tabs based on blockers in the prefab.
  // if they clicked away before the prefab load finished, just cancel and start the next time
  private bool VerifyCanStartPhase() {
    return _CurrPhase != OnboardingPhases.None;
  }
  private void LoadOnboardingAssetsCallback(bool assetBundleSuccess) {
    if (assetBundleSuccess) {
      _OnboardingUIPrefabData.LoadAssetData((GameObject onboardingUIWrapperPrefab) => {
        if (_OnboardingUIInstance == null && onboardingUIWrapperPrefab != null) {
          GameObject wrapper = UIManager.CreateUIElement(onboardingUIWrapperPrefab.gameObject);
          _OnboardingUIInstance = wrapper.GetComponent<OnboardingUIWrapper>();
          if (VerifyCanStartPhase()) {
            SetSpecificStage(0);
          }
          else {
            _CurrPhase = OnboardingPhases.None;
          }
        }
      });
    }
    else {
      DAS.Error("OnboardingManager.LoadOnboardingAssetsCallback", "Failed to load asset bundle " + _OnboardingUIPrefabData.AssetBundle);
    }
  }
  private void UnloadIfDoneWithAllPhases() {
    bool allCompleted = Array.TrueForAll(kRequiredPhases, (OnboardingPhases phase) => { return !IsOnboardingRequired(phase); });
    if (allCompleted) {
      AssetBundleManager.Instance.UnloadAssetBundle(_OnboardingUIPrefabData.AssetBundle);
      if (_OnboardingUIInstance != null) {
        GameObject.Destroy(_OnboardingUIInstance);
        _OnboardingUIInstance = null;
      }
    }
  }

  public void GoToNextStage() {
    // In demo mode skip to wake up
    if (DebugMenuManager.Instance.DemoMode && _CurrPhase == OnboardingPhases.InitialSetup) {
      // Completed the Show CubePhase
      if (GetCurrStageInPhase(_CurrPhase) == (_OnboardingUIInstance.GetMaxStageInPhase(_CurrPhase) - 1)) {
        CompletePhase(_CurrPhase);
      }
      else {
        SetSpecificStage(GetCurrStageInPhase(_CurrPhase) + 1);
      }
    }
    else {
      SetSpecificStage(GetCurrStageInPhase(_CurrPhase) + 1);
    }
  }

  private void SetSpecificStage(int nextStage, bool canStartNewPhase = true) {
    if (_CurrStageInst != null) {
      GameObject.Destroy(_CurrStageInst);
    }
    if (_OnboardingUIInstance == null) {
      DAS.Error("onboardingmanager.SetSpecificStage", "Onboarding Asset Bundle load not completed");
      return;
    }
    int nextDASPhaseID = -1;
    SetCurrStageInPhase(nextStage, _CurrPhase);
    if (nextStage >= 0 && nextStage < _OnboardingUIInstance.GetMaxStageInPhase(_CurrPhase)) {
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

      UpdateStage(stagePrefab.ButtonStateDiscover, stagePrefab.ButtonStateRepair, stagePrefab.ButtonStateFeed,
                  stagePrefab.ButtonStatePlay, stagePrefab.ActiveMenuContent, stagePrefab.ReactionsEnabled);
      // Create the debug layer to have a few buttons to work with on screen easily for QA
      // who will have to see this all the time.
      if (_DebugDisplayOn) {
        _OnboardingUIInstance.AddDebugButtons();
      }
    }
    else {
      PhaseCompletedInternal();
      _OnboardingUIInstance.RemoveDebugButtons();
      OnboardingPhases completedPhase = _CurrPhase;
      _CurrPhase = OnboardingPhases.None;
      UpdateStage();
      // only Meet Cozmo phase takes us out of pausing inside needshub
      if (completedPhase != OnboardingPhases.MeetCozmo) {
        HubWorldBase instance = HubWorldBase.Instance;
        if (instance != null) {
          instance.StartFreeplay(RobotEngineManager.Instance.CurrentRobot);
        }
        if (canStartNewPhase) {
          StartAnyPhaseIfNeeded();
        }
      }
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

  private void HandleRobotDisconnected(Anki.Cozmo.ExternalInterface.RobotDisconnected message) {
    // The UI is getting torn down and we're resetting, clear whatever happened.
    if (_CurrPhase != OnboardingPhases.None && _CurrStageInst != null) {
      GameObject.Destroy(_CurrStageInst);
    }
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
    // not including none
    int numStates = Enum.GetNames(typeof(OnboardingPhases)).Length - 1;
    for (int i = 0; i < numStates; ++i) {
      OnboardingManager.Instance.CompletePhase((OnboardingPhases)i, false);
    }
  }

  public void DebugSkipOne() {
    OnboardingManager.Instance.SkipPressed();
  }

  public void DebugSkipAll() {
    DebugCompleteAllOnboarding(string.Empty);
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

  private void UpdateButtonState(Cozmo.UI.CozmoButton button, OnboardingBaseStage.OnboardingButtonStates buttonState) {
    button.gameObject.SetActive(buttonState != OnboardingBaseStage.OnboardingButtonStates.Hidden);
    button.Interactable = buttonState != OnboardingBaseStage.OnboardingButtonStates.Disabled;
    if (buttonState == OnboardingBaseStage.OnboardingButtonStates.Sparkle) {
      SetOutlineRegion(button.transform);
      ShowOutlineRegion(true);
    }
  }

  private void UpdateStage(OnboardingBaseStage.OnboardingButtonStates showButtonDiscover = OnboardingBaseStage.OnboardingButtonStates.Active,
                            OnboardingBaseStage.OnboardingButtonStates showButtonRepair = OnboardingBaseStage.OnboardingButtonStates.Active,
                            OnboardingBaseStage.OnboardingButtonStates showButtonFeed = OnboardingBaseStage.OnboardingButtonStates.Active,
                            OnboardingBaseStage.OnboardingButtonStates showButtonPlay = OnboardingBaseStage.OnboardingButtonStates.Active,
                            bool showContent = true, bool reactionsEnabled = true) {
    if (_NeedsHubView != null) {
      _NeedsHubView.gameObject.SetActive(showContent);
      UpdateButtonState(_NeedsHubView.DiscoverButton, showButtonDiscover);
      UpdateButtonState(_NeedsHubView.RepairButton, showButtonRepair);
      UpdateButtonState(_NeedsHubView.FeedButton, showButtonFeed);
      UpdateButtonState(_NeedsHubView.PlayButton, showButtonPlay);
    }
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      if (reactionsEnabled) {
        if (_StageDisabledReactionaryBehaviors) {
          RobotEngineManager.Instance.CurrentRobot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kOnboardingUpdateStageId);
          _StageDisabledReactionaryBehaviors = false;
        }

      }
      else {
        if (!_StageDisabledReactionaryBehaviors) {
          RobotEngineManager.Instance.CurrentRobot.DisableAllReactionsWithLock(ReactionaryBehaviorEnableGroups.kOnboardingUpdateStageId);
          _StageDisabledReactionaryBehaviors = true;
        }
      }

    }
  }

}
