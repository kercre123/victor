using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;
using Cozmo.Needs;
using Anki.Cozmo.ExternalInterface;

namespace Onboarding {

  public class OnboardingBaseStage : MonoBehaviour {

    public bool ActiveMenuContent { get { return _ActiveMenuContent; } }
    public bool DimBackground { get { return _DimBackground; } }
    public int DASPhaseID { get { return _DASPhaseID; } }
    public List<Anki.Cozmo.NeedId> DimNeedsMeters {
      get { return _DimNeedsMeters; }
    }
    public bool StageForceClosed { get; set; }

    public OnboardingButtonStates ButtonStateDiscover { get { return _ButtonStateDiscover; } }
    public OnboardingButtonStates ButtonStateRepair { get { return _ButtonStateRepair; } }
    public OnboardingButtonStates ButtonStateFeed { get { return _ButtonStateFeed; } }
    public OnboardingButtonStates ButtonStatePlay { get { return _ButtonStatePlay; } }

    [SerializeField]
    protected bool _DimBackground = false;

    [SerializeField]
    protected bool _ActiveMenuContent = false;

    [SerializeField]
    protected bool _ReactionsEnabled = true;

    [SerializeField]
    protected SerializableAnimationTrigger _LoopedAnim = new SerializableAnimationTrigger();

    [SerializeField]
    protected SerializableAnimationTrigger _CustomIdle = new SerializableAnimationTrigger();

    [SerializeField]
    protected bool _FreeplayEnabledOnExit = false;

    [SerializeField]
    protected int _DASPhaseID = 0;

    [SerializeField]
    protected string _OverrideTickerKey = string.Empty;

    [SerializeField]
    protected List<Anki.Cozmo.NeedId> _DimNeedsMeters;

    // Only let certain actions through
    [SerializeField]
    protected bool _EnableWhiteListOnly = true;
    [SerializeField]
    protected List<NeedsStateManager.SerializableNeedsActionIds> _WhiteListedActions;

    [SerializeField]
    protected float[] _ForceNeedValuesOnStart = new float[3] { -1.0f, -1.0f, -1.0f };

    [SerializeField]
    protected float[] _ForceNeedValuesOnEnd = new float[3] { -1.0f, -1.0f, -1.0f };

    [SerializeField]
    protected NeedsStateManager.SerializableNeedsActionIds _ForceNeedValuesOnEventId;
    [SerializeField]
    protected float[] _ForceNeedValuesOnEvent = new float[3] { -1.0f, -1.0f, -1.0f };

    // By default they are all paused during onboarding from decay
    [SerializeField]
    protected Anki.Cozmo.NeedId _UnPausedNeedMeter = Anki.Cozmo.NeedId.Count;

    // Serialized as an int, do not reorder/delete
    public enum OnboardingButtonStates {
      Hidden,
      Disabled,
      Sparkle,
      Active
    };

    [SerializeField]
    protected OnboardingButtonStates _ButtonStateDiscover = OnboardingButtonStates.Active;
    [SerializeField]
    protected OnboardingButtonStates _ButtonStateRepair = OnboardingButtonStates.Active;
    [SerializeField]
    protected OnboardingButtonStates _ButtonStateFeed = OnboardingButtonStates.Active;
    [SerializeField]
    protected OnboardingButtonStates _ButtonStatePlay = OnboardingButtonStates.Active;


    private const string kOnboardingIdleAnimLock = "onboarding_base_stage";

    public virtual void Start() {
      DAS.Info("DEV onboarding stage.started", name);

      // Early idle states need to loop the loading animation.
      IRobot currentRobot = RobotEngineManager.Instance.CurrentRobot;
      if (currentRobot != null) {
        // Really doesn't show a one frame pop to default idle between states
        if (_LoopedAnim.Value != Anki.Cozmo.AnimationTrigger.Count) {
          // The connection might still be playing the intro so queue this as next
          currentRobot.SendAnimationTrigger(_LoopedAnim.Value, loops: 0);
        }

        if (_ForceNeedValuesOnEventId.Value != Anki.Cozmo.NeedsActionId.Count) {
          NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;
        }
      }
      if (currentRobot != null) {
        // During onboarding we really don't want pet reactions or hiccups anywhere
        if (_ReactionsEnabled) {
          currentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kOnboardingBigReactionsOffId + name, ReactionaryBehaviorEnableGroups.kOnboardingBigReactionsOffTriggers);
        }
        else {
          // early phases of onboarding, no reactions
          currentRobot.DisableAllReactionsWithLock(ReactionaryBehaviorEnableGroups.kOnboardingUpdateStageId + name);
        }
      }
    }

    public virtual void OnDestroy() {
      HubWorldBase instance = HubWorldBase.Instance;
      if (instance != null && _FreeplayEnabledOnExit) {
        instance.StartFreeplay();
      }
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
      IRobot currentRobot = RobotEngineManager.Instance.CurrentRobot;
      if (currentRobot != null) {
        if (_ReactionsEnabled) {
          currentRobot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kOnboardingBigReactionsOffId + name);
        }
        else {
          currentRobot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kOnboardingUpdateStageId + name);
        }
      }
      DAS.Info("DEV onboarding stage.ended", name);
    }

    public virtual void OnEnable() {
      Cozmo.PauseManager.Instance.AllowFreeplayOnResume = false;

      if (!string.IsNullOrEmpty(_OverrideTickerKey)) {
        if (OnboardingManager.Instance.OnOverrideTickerString != null) {
          OnboardingManager.Instance.OnOverrideTickerString.Invoke(Localization.Get(_OverrideTickerKey));
        }
      }

      // Force needs level set as a group to make sure no weird quiting or different robot connecting has happened
      if (System.Array.TrueForAll(_ForceNeedValuesOnStart, (float obj) => { return obj >= 0.0f; })) {
        RobotEngineManager.Instance.Message.ForceSetNeedsLevels =
                                Singleton<ForceSetNeedsLevels>.Instance.Initialize(_ForceNeedValuesOnStart);
        RobotEngineManager.Instance.SendMessage();
      }
      SetNeedsActionWhitelist(_EnableWhiteListOnly);
      NeedsStateManager.Instance.PauseExceptForNeed(_UnPausedNeedMeter);

      Init();
    }
    public virtual void OnDisable() {
      if (!string.IsNullOrEmpty(_OverrideTickerKey)) {
        if (OnboardingManager.Instance.OnOverrideTickerString != null) {
          OnboardingManager.Instance.OnOverrideTickerString.Invoke(null);
        }
      }

      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        // since the animation is looped was looped, force kill it.
        if (_LoopedAnim.Value != Anki.Cozmo.AnimationTrigger.Count) {
          robot.CancelAction(Anki.Cozmo.RobotActionType.PLAY_ANIMATION);
        }
      }
      // Clean up needs manager
      SetNeedsActionWhitelist(false);
      NeedsStateManager.Instance.ResumeAllNeeds();

      if (!StageForceClosed) {
        // forced end levels of needs
        if (System.Array.TrueForAll(_ForceNeedValuesOnEnd, (float obj) => { return obj >= 0.0f; })) {
          RobotEngineManager.Instance.Message.ForceSetNeedsLevels =
                                  Singleton<ForceSetNeedsLevels>.Instance.Initialize(_ForceNeedValuesOnEnd);
          RobotEngineManager.Instance.SendMessage();
        }
      }
      Cozmo.PauseManager.Instance.AllowFreeplayOnResume = true;
    }

    // This will get called when first enabled and when coming back from backgrounding when pause manager has suspended idles, etc.
    protected virtual void Init() {
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        if (_CustomIdle.Value != Anki.Cozmo.AnimationTrigger.Count) {
          RobotEngineManager.Instance.CurrentRobot.PushIdleAnimation(_CustomIdle.Value, kOnboardingIdleAnimLock + name);
        }
      }
    }

    public void SetNeedsActionWhitelist(bool listenOnlyForWhitelist) {
      Anki.Cozmo.NeedsActionId[] actionIds = null;
      if (_WhiteListedActions.Count > 0) {
        actionIds = new Anki.Cozmo.NeedsActionId[_WhiteListedActions.Count];
        for (int i = 0; i < actionIds.Length; ++i) {
          actionIds[i] = _WhiteListedActions[i].Value;
        }
      }
      RobotEngineManager.Instance.Message.SetNeedsActionWhitelist =
                        Singleton<SetNeedsActionWhitelist>.Instance.Initialize(listenOnlyForWhitelist, actionIds);
      RobotEngineManager.Instance.SendMessage();
    }

    private void HandleLatestNeedsLevelChanged(Anki.Cozmo.NeedsActionId actionId) {
      if (_ForceNeedValuesOnEventId.Value == actionId) {
        RobotEngineManager.Instance.Message.ForceSetNeedsLevels =
                                        Singleton<ForceSetNeedsLevels>.Instance.Initialize(_ForceNeedValuesOnEvent);
        RobotEngineManager.Instance.SendMessage();
      }
    }

    public virtual void SkipPressed() {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
