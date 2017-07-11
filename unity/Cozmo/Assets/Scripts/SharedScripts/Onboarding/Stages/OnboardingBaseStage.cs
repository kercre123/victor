using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;
using Cozmo.Needs;
using Anki.Cozmo.ExternalInterface;

namespace Onboarding {

  public class OnboardingBaseStage : MonoBehaviour {

    public bool ActiveMenuContent { get { return _ActiveMenuContent; } }
    public bool ReactionsEnabled { get { return _ReactionsEnabled; } }
    public bool DimBackground { get { return _DimBackground; } }
    public int DASPhaseID { get { return _DASPhaseID; } }
    public List<Anki.Cozmo.NeedId> DimNeedsMeters {
      get { return _DimNeedsMeters; }
    }

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
    protected Anki.Cozmo.NeedsActionId _ForceNeedValuesOnEventId = Anki.Cozmo.NeedsActionId.Count;
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
          currentRobot.SendAnimationTrigger(_LoopedAnim.Value,
                                            HandleLoopedAnimationComplete, Anki.Cozmo.QueueActionPosition.NEXT);
        }

        if (_ForceNeedValuesOnEventId != Anki.Cozmo.NeedsActionId.Count) {
          NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;
        }
      }
    }

    public virtual void OnDestroy() {
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.CancelCallback(HandleLoopedAnimationComplete);
      }

      HubWorldBase instance = HubWorldBase.Instance;
      if (instance != null && _FreeplayEnabledOnExit) {
        instance.StartFreeplay();
      }
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;

      DAS.Info("DEV onboarding stage.ended", name);
    }

    public virtual void OnEnable() {
      if (!string.IsNullOrEmpty(_OverrideTickerKey)) {
        if (OnboardingManager.Instance.OnOverrideTickerString != null) {
          OnboardingManager.Instance.OnOverrideTickerString.Invoke(Localization.Get(_OverrideTickerKey));
        }
      }

      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        if (_CustomIdle.Value != Anki.Cozmo.AnimationTrigger.Count) {
          RobotEngineManager.Instance.CurrentRobot.PushIdleAnimation(_CustomIdle.Value, kOnboardingIdleAnimLock);
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

    }
    public virtual void OnDisable() {
      if (!string.IsNullOrEmpty(_OverrideTickerKey)) {
        if (OnboardingManager.Instance.OnOverrideTickerString != null) {
          OnboardingManager.Instance.OnOverrideTickerString.Invoke(null);
        }
      }

      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        if (_CustomIdle.Value != Anki.Cozmo.AnimationTrigger.Count) {
          robot.RemoveIdleAnimation(kOnboardingIdleAnimLock);
        }
      }
      // Clean up needs manager
      SetNeedsActionWhitelist(false);
      NeedsStateManager.Instance.ResumeAllNeeds();

      // forced end levels of needs
      if (System.Array.TrueForAll(_ForceNeedValuesOnEnd, (float obj) => { return obj >= 0.0f; })) {
        RobotEngineManager.Instance.Message.ForceSetNeedsLevels =
                                Singleton<ForceSetNeedsLevels>.Instance.Initialize(_ForceNeedValuesOnEnd);
        RobotEngineManager.Instance.SendMessage();
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
      if (_ForceNeedValuesOnEventId == actionId) {
        RobotEngineManager.Instance.Message.ForceSetNeedsLevels =
                                        Singleton<ForceSetNeedsLevels>.Instance.Initialize(_ForceNeedValuesOnEvent);
        RobotEngineManager.Instance.SendMessage();
      }
    }

    public virtual void SkipPressed() {
      OnboardingManager.Instance.GoToNextStage();
    }

    protected virtual void HandleLoopedAnimationComplete(bool success = true) {
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(_LoopedAnim.Value, HandleLoopedAnimationComplete);
      }
    }
  }

}
