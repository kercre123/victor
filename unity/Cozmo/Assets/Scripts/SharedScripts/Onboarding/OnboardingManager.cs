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
  public NeedsHubView NeedsView {
    get {
      return _NeedsHubView;
    }
  }

  // DO NOT DELETE ENUMS FROM THIS LIST. They are saved within the save file by name.
  public enum OnboardingPhases : int {
    InitialSetup,
    MeetCozmo,
    NurtureIntro, // includes repair
    FeedIntro,
    PlayIntro,
    RewardBox,
    DiscoverIntro,
    GameRequests,
    NotificationsPermission,
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
                                    OnboardingPhases.NotificationsPermission };

  public Action<OnboardingPhases, int> OnOnboardingStageStarted;
  public Action<OnboardingPhases> OnOnboardingPhaseStarted;
  public Action<OnboardingPhases> OnOnboardingPhaseCompleted;
  public Action<string> OnOverrideTickerString;
  public Action<string> OnOnboardingAnimEvent;

  public bool FirstTime { get; set; }

  private OnboardingPhases _CurrPhase = OnboardingPhases.None;

  private GameObject _CurrStageInst = null;

  private Transform _OnboardingTransform;

  private int _LastOnboardingPhaseCompletedRobot = 0;

  [SerializeField]
  private GameObjectDataLink _OnboardingUIPrefabData;
  private OnboardingUIWrapper _OnboardingUIInstance;

  [SerializeField]
  private List<int> _NumOnboardingStages;

  [SerializeField]
  private int _NotificationOnboardingReminder_Days = 7;

  [SerializeField]
  private int _NotificationOnboardingRepeatTimesMax = 3;

  private const string kOnboardingManagerIdleLock = "onboarding_manager_idle";

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
      if (IsOnboardingRequired(OnboardingPhases.GameRequests)) {
        RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
      }
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

      // This is sent when needs manager connects to a robot, lets us know if it's an "old robot" that can skip onboarding.
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.WantsNeedsOnboarding>(HandleGetWantsNeedsOnboarding);
      DataPersistence.DataPersistenceManager.Instance.OnSaveDataReset += HandleSaveDataReset;
    }
  }

  private void HandleSaveDataReset() {
    // In the event after this disconnect you connect to yet another device, we still want you to have default sparks
    Instance.GiveStartingInventory();
    Instance.FirstTime = true;
    Instance._LastOnboardingPhaseCompletedRobot = 0;
  }

  public int GetCurrStageInPhase(OnboardingPhases phase) {
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
  public bool IsAnyOnboardingRequired() {
    //Test is the final stage has been finished
    return IsOnboardingRequired(OnboardingPhases.PlayIntro);
  }
  public bool IsAnyOnboardingActive() {
    return _CurrPhase != OnboardingPhases.None;
  }
  public OnboardingPhases GetCurrentPhase() {
    return _CurrPhase;
  }
  public bool IsReturningUser() {
    PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
    int returningUserStage = 0;
    if (profile.OnboardingStages.TryGetValue(OnboardingPhases.Home, out returningUserStage)) {
      return returningUserStage > 0;
    }
    return false;
  }

  public bool IsOldRobot() {
    // FistBump is the first thing unlocked in the new nurture unlock order.
    return _LastOnboardingPhaseCompletedRobot > 0 ||
           UnlockablesManager.Instance.IsUnlocked(UnlockId.FistBump);
  }

  private void HandleGetWantsNeedsOnboarding(Anki.Cozmo.ExternalInterface.WantsNeedsOnboarding message) {
    _LastOnboardingPhaseCompletedRobot = message.onboardingStageCompleted;
  }

  public void InitInitalOnboarding(NeedsHubView needsHubView) {
    _NeedsHubView = needsHubView;
    if (_OnboardingUIInstance != null) {
      _OnboardingTransform = _OnboardingUIInstance.transform.parent.transform;
    }
    if (IsOnboardingRequired(OnboardingPhases.GameRequests)) {
      // Only request quicktap if nothing has been requested before.
      UnlockId[] firstRequest = { UnlockId.QuickTapGame };
      int[] firstWeight = { 100 };
      RobotEngineManager.Instance.Message.SetOverrideGameRequestWeights =
                Singleton<Anki.Cozmo.ExternalInterface.SetOverrideGameRequestWeights>.Instance.Initialize(
                            false, firstRequest, firstWeight, true);
      RobotEngineManager.Instance.SendMessage();
    }

    if (IsOnboardingRequired(OnboardingPhases.InitialSetup)) {
      StartPhase(OnboardingPhases.InitialSetup);
    }
    else if (IsOnboardingRequired(OnboardingPhases.NurtureIntro)) {
      StartPhase(OnboardingPhases.NurtureIntro);
    }
    else if (!DataPersistenceManager.Instance.Data.DefaultProfile.OSNotificationsPermissionsPromptShown) {
      PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
      // we've shown you notifications onboarding once, but you never got to the OS prompt, reset onboarding 
      // every week for 3 times to remind you.
      if (!IsOnboardingRequired(OnboardingPhases.NotificationsPermission) &&
          profile.NumTimesNotificationPermissionReminded < _NotificationOnboardingRepeatTimesMax) {
        TimeSpan lastSpammed = DateTime.Now - profile.LastTimeAskedAboutNotifications;
        // it's been a week, you might have changed your mind, and start when opening animation is done.
        if (lastSpammed.TotalDays > _NotificationOnboardingReminder_Days) {
          DAS.Event("notifications.weeklyreminder", "");
          profile.NumTimesNotificationPermissionReminded++;
          profile.OnboardingStages.Remove(OnboardingPhases.NotificationsPermission);
        }
      }
    }
    _NeedsHubView.DialogOpenAnimationFinished += HandleNeedsViewOpenAnimationCompleted;
  }

  private void HandleNeedsViewOpenAnimationCompleted() {
    if (_NeedsHubView != null) {
      _NeedsHubView.DialogOpenAnimationFinished -= HandleNeedsViewOpenAnimationCompleted;
      OnboardingManager.Instance.StartAnyPhaseIfNeeded();
    }
  }

  public void RestartPhaseAtStage(int stage = 0) {
    if (_CurrStageInst != null) {
      OnboardingBaseStage onboardingInfo = _CurrStageInst.GetComponent<OnboardingBaseStage>();
      onboardingInfo.StageForceClosed = true;
    }
    SetSpecificStage(stage);
  }

  public void StartPhase(OnboardingPhases phase) {
#if SDK_ONLY
    return;
#else
    if (DataPersistenceManager.Instance.Data.DeviceSettings.IsSDKEnabled) {
      return;
    }
    // End any previous phase, can only highlight one thing at once
    if (_CurrPhase != OnboardingPhases.None) {
      // None means we just want to clear everything so don't start something new
      if (_CurrStageInst != null) {
        GameObject.Destroy(_CurrStageInst);
      }
    }
    // Going to disconnect soon and won't be able to clean up a lot of onboarding stuff without a robot
    IRobot currentRobot = RobotEngineManager.Instance.CurrentRobot;
    if (currentRobot == null) {
      return;
    }
    int startStage = 0;
    _CurrPhase = phase;
    if (_CurrPhase == OnboardingPhases.InitialSetup) {
      currentRobot.PushIdleAnimation(AnimationTrigger.OnboardingIdle, kOnboardingManagerIdleLock);
      Cozmo.PauseManager.Instance.IsIdleTimeOutEnabled = false;
      Cozmo.PauseManager.Instance.ExitChallengeOnPause = false;

      DAS.Event("onboarding.start", FirstTime ? "1" : "0");
      // in the event they've ever booted the app before, or it's an old robot.
      // Skip the holding on charger phase because it is the worst and only should be a problem
      // for fresh from factory robots
      DAS.Info("onboarding.InitialSetup", "FirstTime: " + FirstTime + "RobotCompleted: " +
                  _LastOnboardingPhaseCompletedRobot + " isOldRobot: " + IsOldRobot());
      if (!FirstTime || IsOldRobot()) {
        startStage = 1;
      }
      // In demo mode skip to wake up
      if (DebugMenuManager.Instance.DemoMode) {
        startStage = 2;
      }
    } // end first phase complete
    RequestGameManager.Instance.DisableRequestGameBehaviorGroups();

    if (OnOnboardingPhaseStarted != null) {
      OnOnboardingPhaseStarted.Invoke(_CurrPhase);
    }
    DAS.Event("onboarding.checkpoint.started", _CurrPhase.ToString());
    // If assets are already loaded, go otherwise this will wait for callback.
    // It should always be loaded now
    if (PreloadOnboarding()) {
      SetSpecificStage(startStage);
    }
    // Because sparks are now saved on the robot they could have changed robots between these checkpoints.
    // Make sure they have a min number of sparks for places right before required to spend them.
    if (_CurrPhase == OnboardingPhases.InitialSetup ||
        _CurrPhase == OnboardingPhases.PlayIntro ||
        _CurrPhase == OnboardingPhases.NurtureIntro) {
      GiveStartingInventory();
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
    DAS.Event("onboarding.checkpoint.completed", _CurrPhase.ToString());
    IRobot currentRobot = RobotEngineManager.Instance.CurrentRobot;
    if (currentRobot == null) {
      return;
    }
    if (_CurrPhase == OnboardingPhases.InitialSetup) {
      Cozmo.PauseManager.Instance.IsIdleTimeOutEnabled = true;
      Cozmo.PauseManager.Instance.ExitChallengeOnPause = true;
    }

    bool kCompleted = _CurrPhase == OnboardingPhases.PlayIntro;
    const bool kSkipped = false;
    RobotEngineManager.Instance.Message.RegisterOnboardingComplete =
           new Anki.Cozmo.ExternalInterface.RegisterOnboardingComplete((int)_CurrPhase,
               kCompleted, kSkipped);
    RobotEngineManager.Instance.SendMessage();
  }

  // Because looking for a face takes awhile time, we try to load this early and invisible
  public void PrepMeetsCozmoPhase() {
    HubWorldBase instance = HubWorldBase.Instance;
    if (instance != null && instance is Cozmo.Hub.NeedsHub) {
      Cozmo.Hub.NeedsHub homeHubInstance = (Cozmo.Hub.NeedsHub)instance;
      if (_NeedsHubView != null) {
        ChallengeData data = Array.Find(ChallengeDataList.Instance.ChallengeData,
                                      (ChallengeData obj) => { return obj.UnlockId.Value == UnlockId.MeetCozmoGame; });
        // "FaceEnrollmentTest"
        if (data != null) {
          homeHubInstance.ForceStartChallenge(data.ChallengeID);
        }
      }
    }
  }
  public void StartAnyPhaseIfNeeded() {
    if (!IsOnboardingRequired(OnboardingPhases.InitialSetup) && IsOnboardingRequired(OnboardingPhases.MeetCozmo)) {
      PrepMeetsCozmoPhase();
      StartPhase(OnboardingPhases.MeetCozmo);
    }

    if (!IsOnboardingRequired(OnboardingPhases.NurtureIntro) && IsOnboardingRequired(OnboardingPhases.FeedIntro)) {
      StartPhase(OnboardingPhases.FeedIntro);
    }

    if (!IsOnboardingRequired(OnboardingPhases.FeedIntro) && IsOnboardingRequired(OnboardingPhases.PlayIntro)) {
      StartPhase(OnboardingPhases.PlayIntro);
    }

    if (!IsOnboardingRequired(OnboardingPhases.RewardBox) && IsOnboardingRequired(OnboardingPhases.NotificationsPermission)) {
      StartPhase(OnboardingPhases.NotificationsPermission);
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
          if (_OnboardingUIInstance != null) {
            _OnboardingTransform = _OnboardingUIInstance.transform.parent.transform;
          }
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

  public bool AllowFreeplayOnHubEnter() {
    return !IsOnboardingRequired(OnboardingPhases.InitialSetup) &&
           !IsOnboardingRequired(OnboardingPhases.RewardBox);
  }

  public bool IsOnboardingOverridingNavButtons() {
    if (_CurrStageInst != null) {
      OnboardingBaseStage onboardingInfo = _CurrStageInst.GetComponent<OnboardingBaseStage>();
      if (onboardingInfo != null) {
        bool isFeedSpecial = (onboardingInfo.ButtonStateFeed != OnboardingBaseStage.OnboardingButtonStates.Active);
        bool isPlaySpecial = (onboardingInfo.ButtonStatePlay != OnboardingBaseStage.OnboardingButtonStates.Active);
        bool isRepairSpecial = (onboardingInfo.ButtonStateRepair != OnboardingBaseStage.OnboardingButtonStates.Active);
        return isFeedSpecial || isPlaySpecial || isRepairSpecial;
      }
    }
    return false;
  }

  private void SetSpecificStage(int nextStage, bool canStartNewPhase = true) {
    if (_CurrStageInst != null) {
      GameObject.Destroy(_CurrStageInst);
      _CurrStageInst = null;
    }
    if (_OnboardingUIInstance == null) {
      DAS.Error("onboardingmanager.SetSpecificStage", "Onboarding Asset Bundle load not completed");
      return;
    }
    SetCurrStageInPhase(nextStage, _CurrPhase);
    if (nextStage >= 0 && nextStage < _OnboardingUIInstance.GetMaxStageInPhase(_CurrPhase)) {
      OnboardingBaseStage stagePrefab = GetCurrStagePrefab();
      // likely OnboardingUIWrapper.prefab needs to be updated if this ever happens legit
      if (stagePrefab == null) {
        DAS.Error("onboardingmanager.SetSpecificStage", "NO PREFAB SET FOR STAGE " + nextStage + " of phase " + _CurrPhase);
        _CurrPhase = OnboardingPhases.None;
        return;
      }
      _CurrStageInst = UIManager.CreateUIElement(stagePrefab, _OnboardingTransform);
      if (OnOnboardingStageStarted != null) {
        OnOnboardingStageStarted.Invoke(_CurrPhase, nextStage);
      }

      UpdateStage(stagePrefab);
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
      // immediately start next phase if we were already on needsview, otherwise start next on open anim callback
      if (completedPhase != OnboardingPhases.MeetCozmo && completedPhase != OnboardingPhases.PlayIntro) {
        if (canStartNewPhase) {
          StartAnyPhaseIfNeeded();
        }
      }
      UnloadIfDoneWithAllPhases();
      // Onboarding turns back on buttons by default, so force the brackets to update the buttons again.
      if (_NeedsHubView != null && _CurrPhase == OnboardingPhases.None) {
        _NeedsHubView.PopLatestBracketAndUpdateButtons();
      }
    }

    DataPersistenceManager.Instance.Save();
  }

  private void HandleAskForMinigame(Anki.Cozmo.ExternalInterface.RequestGameStart messageObject) {
    // First time they see this, thats all.
    if (IsOnboardingRequired(OnboardingPhases.GameRequests)) {
      CompletePhase(OnboardingPhases.GameRequests);
      RobotEngineManager.Instance.SendMessage();
    }
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestGameStart>(HandleAskForMinigame);
  }

  private void HandleRobotDisconnected(Anki.Cozmo.ExternalInterface.RobotDisconnected message) {
    // The UI is getting torn down and we're resetting, clear whatever happened.
    if (_CurrPhase != OnboardingPhases.None && _CurrStageInst != null) {
      _CurrStageInst.GetComponent<OnboardingBaseStage>().StageForceClosed = true;
      GameObject.Destroy(_CurrStageInst);
    }
    if (_CurrPhase != OnboardingPhases.None) {
      if (_OnboardingUIInstance != null) {
        _OnboardingUIInstance.RemoveDebugButtons();
      }
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
    OnboardingPhases lastPhase = _CurrPhase;
    // not including none
    int numStates = Enum.GetNames(typeof(OnboardingPhases)).Length - 1;
    for (int i = 0; i < numStates; ++i) {
      OnboardingManager.Instance.CompletePhase((OnboardingPhases)i, false);
    }
    if (lastPhase == OnboardingPhases.InitialSetup && _NeedsHubView != null) {
      _NeedsHubView.OnboardingSkipped();
    }
    if (HubWorldBase.Instance != null) {
      HubWorldBase.Instance.StartFreeplay();
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

  public void GiveStartingInventory() {
    PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
    Cozmo.ItemData itemData = Cozmo.ItemDataConfig.GetData(Cozmo.UI.GenericRewardsConfig.Instance.SparkID);
    int giveSparksAmount = itemData.StartingAmount;
    // If the robot has a lot of sparks give them that value.
    int currentSparks = profile.Inventory.GetItemAmount(Cozmo.UI.GenericRewardsConfig.Instance.SparkID);
    // If they have old "treats" saved on the app, which are now sparks, Give them those.
    int oldSparksAmount = profile.Inventory.GetItemAmount(Cozmo.UI.GenericRewardsConfig.Instance.TreatID);
    // We just want to be generious so they either get:
    // 1. An old user from the old app, give them treat sparks
    // 2. A brand new user gets a default amount of start sparks
    // 3. A device connecting to a robot that has had some nurture on it already might have that value if it's large.
    //      Since new sparks are saved on the robot.
    giveSparksAmount = Mathf.Max(oldSparksAmount, giveSparksAmount, currentSparks);
    if (giveSparksAmount != currentSparks) {
      profile.Inventory.SetItemAmount(itemData.ID, giveSparksAmount);
    }
  }

  private bool UpdateButtonState(Cozmo.UI.CozmoButton button, OnboardingBaseStage.OnboardingButtonStates buttonState) {
    button.gameObject.SetActive(buttonState != OnboardingBaseStage.OnboardingButtonStates.Hidden);
    button.Interactable = buttonState != OnboardingBaseStage.OnboardingButtonStates.Disabled;
    if (buttonState == OnboardingBaseStage.OnboardingButtonStates.Sparkle) {
      button.AlwaysShowGlintWhenEnabled = true;
    }
    else {
      button.AlwaysShowGlintWhenEnabled = false;
    }
    const string kNormalButtonSkinName = "NavHub_Button_Normal";
    const string kDimmedButtonSkinName = "NavHub_Button_Dimmed";
    CozmoImage bg = button.GetComponentInChildren<CozmoImage>();
    if (bg != null) {
      bg.LinkedComponentId = (buttonState == OnboardingBaseStage.OnboardingButtonStates.Disabled) ?
                             kDimmedButtonSkinName : kNormalButtonSkinName;
      bg.UpdateSkinnableElements();
    }

    return buttonState == OnboardingBaseStage.OnboardingButtonStates.Disabled;
  }

  private void UpdateStage(OnboardingBaseStage stage = null) {
    // default values for when we just want to reset
    // When we redo onboarding from scatch again, these should be pulled out into a nonmonobehavior normal class to
    // create a concept of default UI states.
    OnboardingBaseStage.OnboardingButtonStates showButtonDiscover = OnboardingBaseStage.OnboardingButtonStates.Active;
    OnboardingBaseStage.OnboardingButtonStates showButtonRepair = OnboardingBaseStage.OnboardingButtonStates.Active;
    OnboardingBaseStage.OnboardingButtonStates showButtonFeed = OnboardingBaseStage.OnboardingButtonStates.Active;
    OnboardingBaseStage.OnboardingButtonStates showButtonPlay = OnboardingBaseStage.OnboardingButtonStates.Active;
    bool showContent = true;
    bool showDimmer = false;
    List<NeedId> dimmedMeters = new List<NeedId>();
    if (stage != null) {
      showButtonDiscover = stage.ButtonStateDiscover;
      showButtonRepair = stage.ButtonStateRepair;
      showButtonFeed = stage.ButtonStateFeed;
      showButtonPlay = stage.ButtonStatePlay;
      showContent = stage.ActiveMenuContent;
      showDimmer = stage.DimBackground;
      dimmedMeters = stage.DimNeedsMeters;
    }

    if (_NeedsHubView != null) {
      _NeedsHubView.gameObject.SetActive(showContent);
      _NeedsHubView.OnboardingBlockoutImage.gameObject.SetActive(showDimmer);
      bool anyDimmed = false;
      anyDimmed |= UpdateButtonState(_NeedsHubView.DiscoverButton, showButtonDiscover);
      anyDimmed |= UpdateButtonState(_NeedsHubView.RepairButton, showButtonRepair);
      anyDimmed |= UpdateButtonState(_NeedsHubView.FeedButton, showButtonFeed);
      anyDimmed |= UpdateButtonState(_NeedsHubView.PlayButton, showButtonPlay);
      _NeedsHubView.NavBackgroundImage.LinkedComponentId = anyDimmed ?
                                                          ThemeKeys.Cozmo.Image.kNavHubContainerBGDimmed :
                                                          ThemeKeys.Cozmo.Image.kNavHubButtonNormal;
      _NeedsHubView.NavBackgroundImage.UpdateSkinnableElements();
      if (_NeedsHubView.MetersWidget != null) {
        _NeedsHubView.MetersWidget.DimNeedMeters(dimmedMeters);
      }
    }
  }

}
