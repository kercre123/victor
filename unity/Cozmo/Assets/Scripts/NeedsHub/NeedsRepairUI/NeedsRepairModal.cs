using System.Collections.Generic;
using System.Linq;
using Anki.Cozmo;
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;
using UnityEngine.Events;
using Anki.Cozmo.ExternalInterface;
using Anki.Cozmo.Audio;

namespace Cozmo.Repair.UI {

  public enum ArrowInput {
    Up,
    Down
  };

  public class NeedsRepairModal : BaseModal {

    #region Nested Definitions

    //note: the integeter value of this state is fed as a parameter into _ModalStateAnimator
    public enum RepairModalState {
      OPENING = 0,  //modal is opening, wait
      PRE_SCAN = 1, //show button to start scan
      SCAN = 2,     //animations and effects revealing broken part status, repair buttons
      TUNE_UP = 3,  //user doing repair activity on a part
      REPAIRED = 4  //all broken parts have been repaired
    }

    public enum TuneUpState {
      INTRO,            //elements transitioning in
      START_ROUND,      //show pip to indicate which round we are on
      REVEAL_SYMBOLS,   //reveal one symbol at a time until all showing
      ENTER_SYMBOLS,    //user hitting symbol buttons to match pattern
      PATTERN_MISMATCH, //user submitted incorrect symbol
      PATTERN_MATCHED,  //full pattern entered successfully, increment round icons
      PLAY_REPAIR_SEQUENCE,   //tell robot to animate the pattern and wait with screen dulled
      RESPOND_ROUND_SUCCESS,  // play an animation to indicate the round successfully completed
      RESPOND_PART_REPAIRED,  // play an animation to indicate the part was successfully repaired
      WAIT_FOR_NEXT_ROUND,
      REPAIR_SEQUENCE_INTERRUPTED, // the robot's response was interrupted by being picked up - 
                                   // wait for the robot to be put back down and then re-play the response
    }

    [System.Serializable]
    public class TuneUpActivityRoundData {
      public int NumSymbols = 4;            //symbols to match per round
      public float SymbolRevealDelay = 1f;  //delay per symbol in seconds
    }

    [System.Serializable]
    public class RepairPartElements {
      public CozmoButton[] Buttons = null;
      public GameObject[] DamagedElements = null;
      public GameObject[] RepairedElements = null;
      public CozmoText TuneUpTitle = null;
    }

    private const string kNeedsRepairIdleLock = "needs_repair_idle";

    #endregion //Nested Definitions

    #region Serialized Fields

    [SerializeField]
    private RectTransform _MetersAnchor;

    [SerializeField]
    private NeedsMetersWidget _MetersWidgetPrefab;

    [SerializeField]
    private RepairPartElements _HeadElements = null;

    [SerializeField]
    private RepairPartElements _LiftElements = null;

    [SerializeField]
    private RepairPartElements _TreadsElements = null;

    [SerializeField]
    private Animator _ModalStateAnimator = null;

    [SerializeField]
    private CozmoButton _ScanButton = null;

    [SerializeField]
    private CozmoText _RepairButtonsInstruction = null;

    [SerializeField]
    private CozmoText _NoRepairsNeededNotice = null;

    [SerializeField]
    private CozmoButton _UpButton = null;

    [SerializeField]
    private GameObject _UpButtonWrongIndicator = null;

    [SerializeField]
    private CozmoButton _DownButton = null;

    [SerializeField]
    private GameObject _DownButtonWrongIndicator = null;

    [SerializeField]
    private CozmoButton _CalibrateButton = null;

    [SerializeField]
    private TuneUpActivityRoundData[] _RoundData = null;

    [SerializeField]
    private GameObject _RoundPipPrefab = null;

    [SerializeField]
    private RectTransform _RoundPipAnchor = null;

    [SerializeField]
    private GameObject _UpdownArrowPrefab = null;

    [SerializeField]
    private RectTransform _UpdownArrowAnchor = null;

    [SerializeField]
    private GameObject _RobotRespondingDimmerPanel = null;

    [SerializeField]
    private CozmoButton _ContinueButton = null;

    [SerializeField]
    private RectTransform _RevealProgressBar = null;

    [SerializeField]
    private float _PreScanMinTime_sec = 0.25f; //gives the elements time to animate on screen

    [SerializeField]
    private float _ScanDuration_sec = 3f;

    [SerializeField]
    private float _TuneUpIntroTime_sec = 1f;

    [SerializeField]
    private float _StartRoundTime_sec = 1f;

    [SerializeField]
    private float _MismatchDisplayDuration_sec = 2f;

    [SerializeField]
    private float _RobotResponseMaxTime_sec = 30f;

    [SerializeField]
    private float _InactivityTimeOut_sec = 300f;

    [SerializeField]
    private bool _CozmoMovedReactionsInterrupt = true;

    #endregion //Serialized Fields

    #region Non-serialized Fields

    private NeedsMetersWidget _MetersWidget;

    private NeedBracketId _LastBracketTransitionedInto = NeedBracketId.Count;

    private RepairModalState _CurrentModalState = RepairModalState.PRE_SCAN;
    private float _TimeInModalState_sec = 0f;

    //framewise inputs to compartmentalize transition logic
    private bool _OpeningTweenComplete = false;
    private bool _ScanRequested = false;
    private bool _RepairNeeded = false;
    private bool _RepairRequested = false;
    private bool _RepairCompleted = false;

    private TuneUpState _CurrentTuneUpState = TuneUpState.INTRO;
    private float _TimeInTuneUpState_sec = 0f;

    private bool _MismatchDetected = false;
    private bool _FullPatternMatched = false;
    private bool _CalibrationRequested = false;
    private bool _RobotResponseDone = false;

    // variable which allows the PlaySeverityTransition function to be locked
    // this allows the TuneUpComplete stage transition to fire off a needs action event
    // and then queue an appropriate response to Cozmo without having to worry about
    // a needs transition also being queued asyncronously
    private bool _ShouldPlayPartRepairAnimation = false;

    private OffTreadsState _RobotOffTreadsState = OffTreadsState.OnTreads;
    private bool _WaitingForAnimationsToBeRunnable = false;

    private RepairablePartId _PartToRepair = RepairablePartId.Head;
    private NeedsActionId _PartRepairAction = NeedsActionId.RepairHead;

    private List<ArrowInput> _TuneUpPatternToMatch = new List<ArrowInput>();
    private List<ArrowInput> _TuneUpValuesEntered = new List<ArrowInput>();

    private float _RevealTimer_sec = 0f;
    private float _RevealProgressMaxWidth = 869f;
    private int _SymbolsShown = 0;
    private int _RoundIndex = 0;
    private int _RobotResponseIndex = 0;

    private List<RoundProgressPip> _RoundProgressPips = new List<RoundProgressPip>();
    private List<UpDownArrow> _UpDownArrows = new List<UpDownArrow>();

    private AlertModal _InterruptedAlert = null;

    private bool _WaitForDropCube = false;

    //when this timer reaches zero, the modal will close
    //refreshes to _InactivityTimeOut on any touch or state change
    private bool _InteractionDetected = false;
    private float _InactivityTimer_sec = float.MaxValue;

    private ushort _ScanSFXCallbackID = 0;

    private int _NumPartsRepaired = 0; //tallied for analytics
    private int _NumberOfBrokenParts = 0;
    private int _NumberOfBrokenPartsDisplayed = 0;

    private int _NumberOfRepairRounds = 1;

    #endregion //Non-serialized Fields

    #region Life Span

    public void InitializeRepairModal() {
      RobotEngineManager.Instance.AddCallback<RobotOffTreadsStateChanged>(HandleRobotOffTreadsStateChanged);
      RobotEngineManager.Instance.AddCallback<GoingToSleep>(HandleGoingToSleep);

      HubWorldBase.Instance.StopFreeplay();
      UpdateSeverityBracket();

      NeedsStateManager nsm = NeedsStateManager.Instance;

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.CancelAllCallbacks();
        robot.CancelAction(RobotActionType.UNKNOWN);

        HubWorldBase.Instance.StopFreeplay();

        NeedsValue latestValue = nsm.PeekLatestEngineValue(NeedId.Repair);
        SetDisableReactions(robot, latestValue.Bracket);

        if (robot.Status(RobotStatusFlag.IS_CARRYING_BLOCK)) {
          _WaitForDropCube = true;
          robot.PlaceObjectOnGroundHere((success) => {
            _WaitForDropCube = false;
            UpdateRobotRepairIdleAnim();
          });
        }
        else {
          // We want to play a get in IFF we're entering repair for the first time and
          // aren't in severe state - this is really hacky but we're about to branch and
          // this is the safest fix to this spaghetti code
          if (NeedBracketId.Critical != nsm.PopLatestEngineValue(NeedId.Repair).Bracket) {
            PlayGetInAnim(NeedBracketId.Normal,(success)=>{ UpdateRobotRepairIdleAnim();});
          } else {
            UpdateRobotRepairIdleAnim();
          }
        }
      }

      GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Nurture_Repair);

      nsm.PauseExceptForNeed(NeedId.Repair);

      _MetersWidget = UIManager.CreateUIElement(_MetersWidgetPrefab.gameObject,
                                                _MetersAnchor).GetComponent<NeedsMetersWidget>();
      _MetersWidget.Initialize(dasParentDialogName: DASEventDialogName,
                               baseDialog: this,
                               focusOnMeter: NeedId.Repair);

      InitializePartElements(HandleRepairHeadButtonPressed, "repair_head_button", _HeadElements);
      InitializePartElements(HandleRepairLiftButtonPressed, "repair_lift_button", _LiftElements);
      InitializePartElements(HandleRepairTreadsButtonPressed, "repair_treads_button", _TreadsElements);

      _NumberOfBrokenParts = RefreshAllPartElements();
      DAS.Event("activity.repair.num_broken_parts_on_modal_launch",
                DASEventDialogName,
                DASUtil.FormatExtraData(_NumberOfBrokenParts.ToString()));
      _NumberOfBrokenPartsDisplayed = _NumberOfBrokenParts;

      //start in scanner mode
      _ScanButton.Initialize(HandleScanButtonPressed, "scan_button", DASEventDialogName);
      _ScanButton.interactable = false;

      _UpButton.Initialize(HandleUpButtonPressed, "repair_up_button", DASEventDialogName);
      _UpButton.gameObject.SetActive(false);

      _UpButtonWrongIndicator.SetActive(false);

      _DownButton.Initialize(HandleDownButtonPressed, "repair_down_button", DASEventDialogName);
      _DownButton.gameObject.SetActive(false);

      _DownButtonWrongIndicator.SetActive(false);

      _CalibrateButton.Initialize(HandleCalibrateButtonPressed, "repair_callibrate_button", DASEventDialogName);
      _CalibrateButton.gameObject.SetActive(false);

      _NumberOfRepairRounds = nsm.GetLatestRepairRounds();
      if (_NumberOfRepairRounds > _RoundData.Length) {
        DAS.Error("activity.repair.too_many_repair_rounds", "Configured number of repair rounds is more than the prefab holds");
      }

      // If there's more than one round, display round indicators
      if (_NumberOfRepairRounds > 1) {
        while (_RoundProgressPips.Count < _NumberOfRepairRounds) {
          GameObject obj = GameObject.Instantiate(_RoundPipPrefab, _RoundPipAnchor);
          RoundProgressPip toggler = obj.GetComponent<RoundProgressPip>();
          _RoundProgressPips.Add(toggler);
          toggler.SetComplete(false);
        }
      }

      _RobotRespondingDimmerPanel.SetActive(false);

      _ContinueButton.Initialize(HandleUserClose, "repair_continue_button", DASEventDialogName);

      _RevealProgressMaxWidth = _RevealProgressBar.sizeDelta.x;

      EnterModalState(); //do any state set up if needed for our initial state

      if (_OptionalCloseDialogCozmoButton != null &&
          OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.NurtureIntro)) {
        _OptionalCloseDialogCozmoButton.gameObject.SetActive(false);
      }
    }

    private void SetDisableReactions(IRobot robot, NeedBracketId bracket) {
      robot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kMinigameId,
                                     bracket == NeedBracketId.Critical ?
                                     ReactionaryBehaviorEnableGroups.kRepairGameSevereTriggers :
                                     ReactionaryBehaviorEnableGroups.kDefaultMinigameTriggers);
    }

    protected override void RaiseDialogOpenAnimationFinished() {
      base.RaiseDialogOpenAnimationFinished();
      NeedsStateManager nsm = NeedsStateManager.Instance;
      if (nsm != null) {
        nsm.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;
      }
      HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
      _OpeningTweenComplete = true;
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.SetDefaultHeadAndLiftState(true, robot.GetHeadAngleFactor(), 0.0f);
      }
    }

    private void Update() {
      if (!IsClosed) {
        RefreshModalStateLogic(Time.deltaTime);
        UpdateInterruptModal();

        if (_InteractionDetected) {
          _InactivityTimer_sec = _InactivityTimeOut_sec;
        }
        else if (_InactivityTimer_sec > 0f) {

          _InactivityTimer_sec -= Time.deltaTime;

          if (_InactivityTimer_sec <= 0f && !OnboardingManager.Instance.IsAnyOnboardingActive()) {
            HandleUserClose();
          }
        }

        _InteractionDetected = false;
      }
    }

    private void UpdateInterruptModal() {
      if ((_RobotOffTreadsState == OffTreadsState.OnTreads) &&
        (_InterruptedAlert != null)) {
        _InterruptedAlert.CloseDialog();
      }

      if ((_CurrentModalState == RepairModalState.TUNE_UP) &&
          (_RobotOffTreadsState != OffTreadsState.OnTreads)) {
        ShowDontMoveCozmoAlert(_RobotOffTreadsState);
      }
    }

    protected override void RaiseDialogClosed() {
      ExitModalState(); //ensure that we do any state clean up before killing our fsm

      _ModalStateAnimator.SetInteger("ModalState", 0);
      _ModalStateAnimator.SetTrigger("StateChange");

      while (_RoundProgressPips.Count > 0) {
        GameObject.Destroy(_RoundProgressPips[0].gameObject);
        _RoundProgressPips.RemoveAt(0);
      }

      NeedsStateManager nsm = NeedsStateManager.Instance;
      nsm.ResumeAllNeeds();
      nsm.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;

      //RETURN TO FREEPLAY
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.SetDefaultHeadAndLiftState(false, 0.0f, 0.0f);
        robot.CancelAction(RobotActionType.PLAY_ANIMATION);
        PlayGetOutAnim(_LastBracketTransitionedInto);
        HubWorldBase.Instance.StartFreeplay();
        robot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kMinigameId);
      }

      DAS.Event("activity.repair.parts_repaired_in_modal", DASEventDialogName,
                DASUtil.FormatExtraData(_NumPartsRepaired.ToString()));

      base.RaiseDialogClosed();
    }

    protected override void CleanUp() {
      RobotEngineManager.Instance.RemoveCallback<RobotOffTreadsStateChanged>(HandleRobotOffTreadsStateChanged);
      RobotEngineManager.Instance.RemoveCallback<GoingToSleep>(HandleGoingToSleep);

      if (_InterruptedAlert != null) {
        _InterruptedAlert.CloseDialog();
      }
    }

    private void OnApplicationPause(bool pauseStatus) {
      // Since the window closes back they need instructions from start again
      if (pauseStatus && OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.NurtureIntro)) {
        OnboardingManager.Instance.RestartPhaseAtStage(1);
      }
      DAS.Debug("NeedsRepairModal.OnApplicationPause", "Application pause: " + pauseStatus);
      HandleUserClose();
    }

    #endregion //Life Span

    #region Modal FSM

    private void RefreshModalStateLogic(float deltaTime) {
      RefreshModalStateInputs();
      bool stateChanged = ModalStateTransition();
      ClearModalStateInputs();

      if (!stateChanged) {
        UpdateModalState(deltaTime);
      }
    }

    private void RefreshModalStateInputs() {
      _RepairCompleted = _CurrentModalState == RepairModalState.TUNE_UP && _CurrentTuneUpState == TuneUpState.WAIT_FOR_NEXT_ROUND;
    }

    private void ClearModalStateInputs() {
      _OpeningTweenComplete = false;
      _RepairRequested = false;
      _RepairCompleted = false;
    }

    //transitions can ONLY happen within this method
    private bool ModalStateTransition() {
      switch (_CurrentModalState) {
      case RepairModalState.OPENING:
        if (_OpeningTweenComplete) {
          return ChangeModalState(RepairModalState.PRE_SCAN);
        }
        break;
      case RepairModalState.PRE_SCAN:
        if (_TimeInModalState_sec > _PreScanMinTime_sec && _ScanRequested) {
          return ChangeModalState(RepairModalState.SCAN);
        }
        break;
      case RepairModalState.SCAN:
        if (_TimeInModalState_sec >= _ScanDuration_sec) {
          if (!_RepairNeeded) {
            return ChangeModalState(RepairModalState.REPAIRED);
          }
          if (_NumberOfBrokenParts != _NumberOfBrokenPartsDisplayed) {
            return ChangeModalState(RepairModalState.PRE_SCAN);
          }
        }
        if (_RepairRequested) {
          return ChangeModalState(RepairModalState.TUNE_UP);
        }
        break;
      case RepairModalState.TUNE_UP:
        if (_RepairCompleted) {
          if (!_RepairNeeded) {
            return ChangeModalState(RepairModalState.REPAIRED);
          }

          return ChangeModalState(RepairModalState.PRE_SCAN);
        }
        break;
      case RepairModalState.REPAIRED:
        if (_NumberOfBrokenParts != _NumberOfBrokenPartsDisplayed) {
          return ChangeModalState(RepairModalState.PRE_SCAN);
        }
        break;
      }
      return false;
    }

    private bool ChangeModalState(RepairModalState newState) {
      if (newState == _CurrentModalState) {
        return false;
      }

      ExitModalState();
      _CurrentModalState = newState;
      EnterModalState();

      return true;
    }

    private void EnterModalState() {
      _TimeInModalState_sec = 0f;

      _InactivityTimer_sec = _InactivityTimeOut_sec;

      _ModalStateAnimator.SetInteger("ModalState", (int)_CurrentModalState);
      _ModalStateAnimator.SetTrigger("StateChange");

      switch (_CurrentModalState) {
      case RepairModalState.PRE_SCAN:
        DisableAllRepairButtons();
        //only show button if the user hasn't already pressed it, elsewise go straight to scan
        _ScanButton.gameObject.SetActive(!_ScanRequested);
        _ScanButton.interactable = !_ScanRequested;
        break;
      case RepairModalState.SCAN:
        DAS.Event("activity.repair.scan", DASEventDialogName);
        _MetersWidget.RepairMeterPaused = true;
        _NumberOfBrokenPartsDisplayed = RefreshAllPartElements();
        DAS.Event("activity.repair.scan_num_broken_parts",
                  DASEventDialogName,
                  DASUtil.FormatExtraData(_NumberOfBrokenPartsDisplayed.ToString()));

        NeedsStateManager nsm = NeedsStateManager.Instance;
        float repairValue = nsm.PopLatestEngineValue(NeedId.Repair).Value;
        DAS.Event("activity.repair.scan_current_value",
                  DASEventDialogName,
                  DASUtil.FormatExtraData(repairValue.ToString()));

        NeedBracketId bracket = nsm.PopLatestEngineValue(NeedId.Repair).Bracket;
        DAS.Event("activity.repair.scan_current_bracket",
                  DASEventDialogName,
                  DASUtil.FormatExtraData(bracket.ToString()));

        _ScanSFXCallbackID = GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Scan,
                                     Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete,
                                     ScanSoundComplete);
        break;
      case RepairModalState.TUNE_UP:
        _TuneUpValuesEntered.Clear();
        _RoundIndex = 0;

        foreach (RoundProgressPip pip in _RoundProgressPips) {
          pip.SetComplete(false);
        }

        _RevealProgressBar.sizeDelta = new Vector2(0f, _RevealProgressBar.sizeDelta.y);

        _HeadElements.TuneUpTitle.gameObject.SetActive(_PartToRepair == RepairablePartId.Head);
        _LiftElements.TuneUpTitle.gameObject.SetActive(_PartToRepair == RepairablePartId.Lift);
        _TreadsElements.TuneUpTitle.gameObject.SetActive(_PartToRepair == RepairablePartId.Treads);

        _CurrentTuneUpState = TuneUpState.INTRO;

        EnterTuneUpState();
        break;
      case RepairModalState.REPAIRED:
        HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Level_Success);
        break;
      }
    }

    private void UpdateModalState(float deltaTime) {
      _TimeInModalState_sec += deltaTime;
      switch (_CurrentModalState) {
      case RepairModalState.PRE_SCAN: break;
      case RepairModalState.SCAN:
        if (_MetersWidget.RepairMeterPaused && _TimeInModalState_sec >= _ScanDuration_sec) {
          _MetersWidget.RepairMeterPaused = false;
          HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
          _NumberOfBrokenPartsDisplayed = RefreshAllPartElements();
        }
        break;
      case RepairModalState.TUNE_UP:
        RefreshTuneUpStateLogic(deltaTime);
        break;
      case RepairModalState.REPAIRED: break;
      }
    }

    private void ExitModalState() {
      switch (_CurrentModalState) {
      case RepairModalState.PRE_SCAN:
        _ScanButton.gameObject.SetActive(false);
        break;
      case RepairModalState.SCAN:
        _MetersWidget.RepairMeterPaused = false;
        HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);

        if (_TimeInModalState_sec < _ScanDuration_sec) {
          GameAudioClient.UnregisterCallbackHandler(_ScanSFXCallbackID);
          GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Scan_Stop);
          _ScanSFXCallbackID = 0;
        }
        break;
      case RepairModalState.TUNE_UP:
        ExitTuneUpState(TuneUpState.INTRO);

        while (_UpDownArrows.Count > 0) {
          GameObject.Destroy(_UpDownArrows[0].gameObject);
          _UpDownArrows.RemoveAt(0);
        }

        var robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot != null) {
          robot.TryResetHeadAndLift(null);
        }
        break;
      case RepairModalState.REPAIRED:
        break;
      }
    }

    #endregion //Modal FSM

    #region Tune-up Sub-FSM

    private void RefreshTuneUpStateLogic(float deltaTime) {
      RefreshTuneUpInputs();
      bool stateChanged = TuneUpStateTransition();
      ClearTuneUpInputs();

      if (!stateChanged) {
        UpdateTuneUpState(deltaTime);
      }
    }

    private void RefreshTuneUpInputs() {
      _FullPatternMatched = false;
      if (_CurrentTuneUpState == TuneUpState.ENTER_SYMBOLS) {
        _FullPatternMatched = _TuneUpValuesEntered.SequenceEqual(_TuneUpPatternToMatch);
      }

      _MismatchDetected = false;
      for (int i = 0; i < _TuneUpValuesEntered.Count; i++) {
        if (i >= _TuneUpPatternToMatch.Count || _TuneUpValuesEntered[i] != _TuneUpPatternToMatch[i]) {
          _MismatchDetected = true;
          break;
        }
      }
    }

    private void ClearTuneUpInputs() {
      _MismatchDetected = false;
      _FullPatternMatched = false;
      _CalibrationRequested = false;
      _RobotResponseDone = false;
    }

    //transitions can ONLY happen within this method
    private bool TuneUpStateTransition() {
      switch (_CurrentTuneUpState) {
      case TuneUpState.INTRO:
        if (_TimeInTuneUpState_sec >= _TuneUpIntroTime_sec) {
          return ChangeTuneUpState(TuneUpState.START_ROUND);
        }
        break;
      case TuneUpState.START_ROUND:
        if (_TimeInTuneUpState_sec >= _StartRoundTime_sec) {
          return ChangeTuneUpState(TuneUpState.REVEAL_SYMBOLS);
        }
        break;
      case TuneUpState.REVEAL_SYMBOLS:
        if (_SymbolsShown == _RoundData[_RoundIndex].NumSymbols) {
          return ChangeTuneUpState(TuneUpState.ENTER_SYMBOLS);
        }
        break;
      case TuneUpState.ENTER_SYMBOLS:
        if (_MismatchDetected) {
          return ChangeTuneUpState(TuneUpState.PATTERN_MISMATCH);
        }
        if (_FullPatternMatched) {
          return ChangeTuneUpState(TuneUpState.PATTERN_MATCHED);
        }
        break;
      case TuneUpState.PATTERN_MISMATCH:
        if (_TimeInTuneUpState_sec >= _MismatchDisplayDuration_sec) {
          return ChangeTuneUpState(TuneUpState.REVEAL_SYMBOLS);
        }
        break;
      case TuneUpState.PATTERN_MATCHED:
        if (_CalibrationRequested) {
          return ChangeTuneUpState(TuneUpState.PLAY_REPAIR_SEQUENCE);
        }
        break;
      case TuneUpState.PLAY_REPAIR_SEQUENCE:
        if (_RobotResponseDone || _TimeInTuneUpState_sec >= _RobotResponseMaxTime_sec) {
          if (_RoundIndex < _NumberOfRepairRounds - 1) {
            return ChangeTuneUpState(TuneUpState.START_ROUND);
          }
          return ChangeTuneUpState(TuneUpState.RESPOND_PART_REPAIRED);
        }
        if (_RobotOffTreadsState != OffTreadsState.OnTreads) {
          RobotEngineManager.Instance.CurrentRobot.CancelAction(Anki.Cozmo.RobotActionType.UNKNOWN);
          return ChangeTuneUpState(TuneUpState.REPAIR_SEQUENCE_INTERRUPTED);
        }
        break;

      case TuneUpState.REPAIR_SEQUENCE_INTERRUPTED:
        if (_RobotOffTreadsState == OffTreadsState.OnTreads) {
          return ChangeTuneUpState(TuneUpState.PLAY_REPAIR_SEQUENCE);
        }
        break;
      case TuneUpState.RESPOND_PART_REPAIRED: break;
      }
      return false;
    }

    private bool ChangeTuneUpState(TuneUpState newState) {
      if (newState == _CurrentTuneUpState) {
        return false;
      }

      ExitTuneUpState(newState);
      _CurrentTuneUpState = newState;
      EnterTuneUpState();

      return true;
    }

    private void EnterTuneUpState() {
      _TimeInTuneUpState_sec = 0f;

      _InactivityTimer_sec = _InactivityTimeOut_sec;

      switch (_CurrentTuneUpState) {
      case TuneUpState.INTRO:
        //let's get our initial symbols spawned early so they can transition on nicely
        int symbolsFirstRound = 0;
        if (_RoundData.Length > 0) {
          symbolsFirstRound = _RoundData[0].NumSymbols;
        }

        RefreshSymbols(symbolsFirstRound);
        break;
      case TuneUpState.START_ROUND:
        _RevealProgressBar.sizeDelta = new Vector2(0f, _RevealProgressBar.sizeDelta.y);
        _RevealProgressBar.gameObject.SetActive(true);

        _UpButton.Interactable = false;
        _DownButton.Interactable = false;
        _UpButton.gameObject.SetActive(true);
        _DownButton.gameObject.SetActive(true);

        _UpButtonWrongIndicator.SetActive(false);
        _DownButtonWrongIndicator.SetActive(false);

        RefreshSymbols(_RoundData[_RoundIndex].NumSymbols);

        if (_RoundIndex < _RoundProgressPips.Count) {
          _RoundProgressPips[_RoundIndex].SetComplete(true);
          GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Complete);
        }

        _TuneUpValuesEntered.Clear();
        _TuneUpPatternToMatch.Clear();

        break;
      case TuneUpState.REVEAL_SYMBOLS:
        _RevealProgressBar.sizeDelta = new Vector2(0f, _RevealProgressBar.sizeDelta.y);
        _RevealProgressBar.gameObject.SetActive(true);

        _UpButton.Interactable = false;
        _DownButton.Interactable = false;
        _UpButton.gameObject.SetActive(true);
        _DownButton.gameObject.SetActive(true);

        _UpButtonWrongIndicator.SetActive(false);
        _DownButtonWrongIndicator.SetActive(false);

        _TuneUpValuesEntered.Clear();
        _TuneUpPatternToMatch.Clear();

        int symbolsThisRound = _RoundData[_RoundIndex].NumSymbols;
        RefreshSymbols(symbolsThisRound);

        for (int i = 0; i < symbolsThisRound; i++) {
          _TuneUpPatternToMatch.Add(GetNextSequenceRand());
        }

        _SymbolsShown = 0;
        //shift these reveals
        _RevealTimer_sec = 0f;//_RoundData[_RoundIndex].SymbolRevealDelay;
        break;
      case TuneUpState.ENTER_SYMBOLS:
        _TuneUpValuesEntered.Clear();
        //make symbol entry buttons interactable
        _UpButton.Interactable = true;
        _DownButton.Interactable = true;
        _UpButton.gameObject.SetActive(true);
        _DownButton.gameObject.SetActive(true);
        break;
      case TuneUpState.PATTERN_MISMATCH:
        _UpButton.gameObject.SetActive(true);
        _DownButton.gameObject.SetActive(true);
        _UpButton.Interactable = false;
        _DownButton.Interactable = false;
        _RevealProgressBar.gameObject.SetActive(false);
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Error);
        DAS.Event("activity.repair.pattern_mismatched", DASEventDialogName);
        break;
      case TuneUpState.PATTERN_MATCHED:
        _CalibrateButton.gameObject.SetActive(true);
        _CalibrateButton.Interactable = true;
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Success_Round);
        DAS.Event("activity.repair.pattern_matched", DASEventDialogName);
        break;
      case TuneUpState.PLAY_REPAIR_SEQUENCE:
        // Proxy for "first time responding"
        if (_CalibrateButton.Interactable) {
          GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Calibrate);
        }
        _CalibrateButton.gameObject.SetActive(true);
        _CalibrateButton.Interactable = false;
        _RobotRespondingDimmerPanel.SetActive(true);

        var robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot != null) {
          robot.CancelAction(RobotActionType.PLAY_ANIMATION);
        }
        PlayRobotCalibrationResponseAnim();
        _RevealProgressBar.gameObject.SetActive(false);
        break;
      case TuneUpState.RESPOND_PART_REPAIRED:
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Success_Complete);
        _MetersWidget.RepairMeterPaused = true;
        // Final repair animation will play when the needs action complete callback is received
        _ShouldPlayPartRepairAnimation = true;
        NeedsStateManager.Instance.RegisterNeedActionCompleted(_PartRepairAction);

        DAS.Event("activity.repair.part_repaired", DASEventDialogName,
                  DASUtil.FormatExtraData(_PartRepairAction.ToString()));
        _NumPartsRepaired++;
        break;
      }
    }

    private void PlayPartRepairedAnimation() {
      if (!PlaySeverityTransitionIfNecessary(HandlePartRepairedAnimationFinished)) {
        AnimationTrigger animationTrigger = AnimationTrigger.Count;
        switch (_PartToRepair) {
        case RepairablePartId.Head:
          animationTrigger = AnimationTrigger.RepairPartRepaired_Head_Mild;
          break;
        case RepairablePartId.Lift:
          animationTrigger = AnimationTrigger.RepairPartRepaired_Lift_Mild;
          break;
        case RepairablePartId.Treads:
          animationTrigger = AnimationTrigger.RepairPartRepaired_Tread_Mild;
          break;
        default:
          break;
        }
        if (animationTrigger != AnimationTrigger.Count) {
          var currRobot = RobotEngineManager.Instance.CurrentRobot;
          currRobot.SendAnimationTrigger(animationTrigger,
            HandlePartRepairedAnimationFinished,
            QueueActionPosition.AT_END);
        }
      }
    }

    private void HandlePartRepairedAnimationFinished(bool success) {
      ChangeTuneUpState(TuneUpState.WAIT_FOR_NEXT_ROUND);
    }


    private void UpdateTuneUpState(float deltaTime) {
      _TimeInTuneUpState_sec += deltaTime;
      switch (_CurrentTuneUpState) {
      case TuneUpState.INTRO: break;
      case TuneUpState.START_ROUND: break;
      case TuneUpState.REVEAL_SYMBOLS:

        if (_RevealTimer_sec >= 0f) {
          _RevealTimer_sec -= deltaTime;
          if (_RevealTimer_sec < 0f) {
            if (_SymbolsShown < _TuneUpPatternToMatch.Count) {
              ArrowInput arrow = _TuneUpPatternToMatch[_SymbolsShown];
              _UpDownArrows[_SymbolsShown].Reveal(arrow);
              _SymbolsShown++;
              _RevealTimer_sec = _RoundData[_RoundIndex].SymbolRevealDelay;

              switch (arrow) {
              case ArrowInput.Up:
                GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Up);
                break;
              case ArrowInput.Down:
                GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Down);
                break;
              }
            }
          }
        }

        float revealProgressFactor = 1f;
        float totalDuration = (_RoundData[_RoundIndex].NumSymbols - 1) * _RoundData[_RoundIndex].SymbolRevealDelay;
        float timeSinceFirstArrow = _TimeInTuneUpState_sec;
        if (totalDuration > 0f) {
          revealProgressFactor = Mathf.Clamp01(timeSinceFirstArrow / totalDuration);
        }

        _RevealProgressBar.sizeDelta = new Vector2(revealProgressFactor * _RevealProgressMaxWidth,
                                                         _RevealProgressBar.sizeDelta.y);

        break;
      case TuneUpState.ENTER_SYMBOLS: break;
      case TuneUpState.PATTERN_MISMATCH: break;
      case TuneUpState.PATTERN_MATCHED: break;
      case TuneUpState.PLAY_REPAIR_SEQUENCE: break;
      case TuneUpState.REPAIR_SEQUENCE_INTERRUPTED: break;
      case TuneUpState.RESPOND_PART_REPAIRED: break;
      }
    }

    private void ExitTuneUpState(TuneUpState newTuneUpState) {
      switch (_CurrentTuneUpState) {
      case TuneUpState.INTRO: break;
      case TuneUpState.START_ROUND: break;
      case TuneUpState.REVEAL_SYMBOLS:
        _UpButton.gameObject.SetActive(false);
        _DownButton.gameObject.SetActive(false);
        _RevealProgressBar.sizeDelta = new Vector2(_RevealProgressMaxWidth, _RevealProgressBar.sizeDelta.y);
        break;
      case TuneUpState.ENTER_SYMBOLS:
        _UpButton.gameObject.SetActive(false);
        _DownButton.gameObject.SetActive(false);
        break;
      case TuneUpState.PATTERN_MISMATCH:
        _UpButton.gameObject.SetActive(false);
        _DownButton.gameObject.SetActive(false);
        break;
      case TuneUpState.PATTERN_MATCHED:
        _CalibrateButton.gameObject.SetActive(false);
        break;
      case TuneUpState.PLAY_REPAIR_SEQUENCE:
        if (newTuneUpState == TuneUpState.START_ROUND) {
          ReactivateUIAfterRepairSequence();
        }
        break;
      case TuneUpState.RESPOND_PART_REPAIRED:
        ReactivateUIAfterRepairSequence();
        break;
      case TuneUpState.WAIT_FOR_NEXT_ROUND:
        break;
      }
    }

    private void ReactivateUIAfterRepairSequence() {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      _RoundIndex++;
      _RobotRespondingDimmerPanel.SetActive(false);
      _CalibrateButton.gameObject.SetActive(false);
      if (robot != null) {
        robot.CancelCallback(HandleCalibrateAnimPlayed);
        robot.CancelCallback(HandleRobotResponseDone);
      }
      UpdateRobotRepairIdleAnim();
    }


    #endregion //Tune-up Sub-FSM

    #region Button Click Handlers

    private void HandleScanButtonPressed() {
      _ScanRequested = true;
    }

    private void HandleRepairHeadButtonPressed() {
      DisableAllRepairButtons();
      _RepairRequested = true;
      _PartToRepair = RepairablePartId.Head;
      _PartRepairAction = NeedsActionId.RepairHead;
    }

    private void HandleRepairLiftButtonPressed() {
      DisableAllRepairButtons();
      _RepairRequested = true;
      _PartToRepair = RepairablePartId.Lift;
      _PartRepairAction = NeedsActionId.RepairLift;
    }

    private void HandleRepairTreadsButtonPressed() {
      DisableAllRepairButtons();
      _RepairRequested = true;
      _PartToRepair = RepairablePartId.Treads;
      _PartRepairAction = NeedsActionId.RepairTreads;
    }

    private void HandleUpButtonPressed() {
      AppendTuneUpEntry(ArrowInput.Up);
      _InteractionDetected = true;
    }

    private void HandleDownButtonPressed() {
      AppendTuneUpEntry(ArrowInput.Down);
      _InteractionDetected = true;
    }

    private void HandleCalibrateButtonPressed() {
      _CalibrationRequested = true;
      _InteractionDetected = true;
    }

    #endregion //Button Click Handlers

    #region Robot Callback Handlers

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      if (_ShouldPlayPartRepairAnimation) {
        PlayPartRepairedAnimation();
        _ShouldPlayPartRepairAnimation = false;
      }
      else {
        UpdateRobotRepairIdleAnim();
      }

      _NumberOfBrokenParts = GetCurrentNumberOfBrokenParts();
    }

    // light up the next arrow, note: this may be removed if deemed too distracting by design
    private void HandleCalibrateAnimPlayed(bool success) {
      if (success) {
        if (_RobotResponseIndex < _UpDownArrows.Count) {
          _UpDownArrows[_RobotResponseIndex].Calibrated(_TuneUpPatternToMatch[_RobotResponseIndex]);
        }

        _RobotResponseIndex++;
      }
      else if (!_WaitingForAnimationsToBeRunnable) {
        var robot = RobotEngineManager.Instance.CurrentRobot;
        robot.WaitAction(1, PlayRobotCalibrationResponseAnim);
        _WaitingForAnimationsToBeRunnable = true;
      }
    }

    private void HandleRobotResponseDone(bool success) {
      HandleCalibrateAnimPlayed(success);
      if (!_WaitingForAnimationsToBeRunnable) {
        _RobotResponseDone = true;
      }
    }

    private void HandleRobotOffTreadsStateChanged(object messageObject) {
      RobotOffTreadsStateChanged offTreadsState = messageObject as RobotOffTreadsStateChanged;
      _RobotOffTreadsState = offTreadsState.treadsState;
    }

    // PauseManager has put us in behavior wait and likely we aren't going to continue, just shutdown.
    private void HandleGoingToSleep(Anki.Cozmo.ExternalInterface.GoingToSleep msg) {
      CloseDialog();
    }

    #endregion //Robot Callback Handlers

    #region Misc Helper Methods

    private void DisableAllRepairButtons() {
      foreach (CozmoButton button in _HeadElements.Buttons) {
        button.gameObject.SetActive(false);
      }
      foreach (CozmoButton button in _LiftElements.Buttons) {
        button.gameObject.SetActive(false);
      }
      foreach (CozmoButton button in _TreadsElements.Buttons) {
        button.gameObject.SetActive(false);
      }
    }

    private void AppendTuneUpEntry(ArrowInput arrow) {
      if (_CurrentModalState == RepairModalState.TUNE_UP
          && _CurrentTuneUpState == TuneUpState.ENTER_SYMBOLS
          && _TuneUpValuesEntered.Count < _TuneUpPatternToMatch.Count) {

        _TuneUpValuesEntered.Add(arrow);

        int indexLatest = _TuneUpValuesEntered.Count - 1;
        bool match = _TuneUpPatternToMatch[indexLatest] == _TuneUpValuesEntered[indexLatest];

        //if a match, turn correct arrow green
        if (match) {
          _UpDownArrows[indexLatest].Matched(_TuneUpValuesEntered[indexLatest]);

          if (arrow == ArrowInput.Up) {
            GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Up);
          }
          else {
            GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Down);
          }

        }
        else {
          //if a mismatch, turn the chosen incorrect arrow orange, hide the rest
          for (int i = 0; i < _UpDownArrows.Count; i++) {
            if (i != indexLatest) {
              _UpDownArrows[i].HideAll();
            }
          }

          _UpDownArrows[indexLatest].Mismatched(_TuneUpValuesEntered[indexLatest]);

          if (arrow == ArrowInput.Up) {
            _UpButtonWrongIndicator.SetActive(true);
          }
          else {
            _DownButtonWrongIndicator.SetActive(true);
          }
          IRobot robot = RobotEngineManager.Instance.CurrentRobot;
          if (robot != null) {
            NeedsStateManager nsm = NeedsStateManager.Instance;
            bool severe = nsm.GetCurrentDisplayValue(NeedId.Repair).Bracket == NeedBracketId.Critical;
            if (severe) {
              robot.SendAnimationTrigger(AnimationTrigger.RepairFailSevere);
            }
            else {
              robot.SendAnimationTrigger(AnimationTrigger.RepairFailMild);
            }
            Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Error);
          }
        }
      }
    }

    private void InitializePartElements(UnityAction action, string analyticsKey, RepairPartElements elements) {
      for (int i = 0; i < elements.Buttons.Length; i++) {
        elements.Buttons[i].Initialize(action, analyticsKey + i.ToString(), DASEventDialogName);
      }

      foreach (GameObject obj in elements.DamagedElements) {
        obj.SetActive(false);
      }

      foreach (GameObject obj in elements.RepairedElements) {
        obj.SetActive(false);
      }
    }

    private int GetCurrentNumberOfBrokenParts() {
      bool headBroken = NeedsStateManager.Instance.PeekLatestEnginePartIsBroken(RepairablePartId.Head);
      bool liftBroken = NeedsStateManager.Instance.PeekLatestEnginePartIsBroken(RepairablePartId.Lift);
      bool treadsBroken = NeedsStateManager.Instance.PeekLatestEnginePartIsBroken(RepairablePartId.Treads);

      int partsBroken = 0;
      if (headBroken) {
        partsBroken++;
      }
      if (liftBroken) {
        partsBroken++;
      }
      if (treadsBroken) {
        partsBroken++;
      }

      return partsBroken;
    }

    private int RefreshAllPartElements() {
      bool headBroken = NeedsStateManager.Instance.PopLatestEnginePartIsBroken(RepairablePartId.Head);
      RefreshPartElements(headBroken, _HeadElements);

      bool liftBroken = NeedsStateManager.Instance.PopLatestEnginePartIsBroken(RepairablePartId.Lift);
      RefreshPartElements(liftBroken, _LiftElements);

      bool treadsBroken = NeedsStateManager.Instance.PopLatestEnginePartIsBroken(RepairablePartId.Treads);
      RefreshPartElements(treadsBroken, _TreadsElements);

      _RepairNeeded = headBroken || liftBroken || treadsBroken;
      _RepairButtonsInstruction.gameObject.SetActive(_RepairNeeded);
      _NoRepairsNeededNotice.gameObject.SetActive(!_RepairNeeded);

      int partsBroken = 0;
      if (headBroken) {
        partsBroken++;
      }
      if (liftBroken) {
        partsBroken++;
      }
      if (treadsBroken) {
        partsBroken++;
      }
      GameAudioClient.SetMusicRoundState(Mathf.Max(1, partsBroken));

      return partsBroken;
    }

    private void RefreshPartElements(bool partBroken, RepairPartElements elements) {
      foreach (CozmoButton button in elements.Buttons) {
        button.gameObject.SetActive(partBroken);
      }

      foreach (GameObject obj in elements.DamagedElements) {
        obj.SetActive(partBroken);
      }

      foreach (GameObject obj in elements.RepairedElements) {
        obj.SetActive(!partBroken);
      }
    }

    private void RefreshSymbols(int symbolsThisRound) {
      while (_UpDownArrows.Count < symbolsThisRound) {
        GameObject obj = GameObject.Instantiate(_UpdownArrowPrefab, _UpdownArrowAnchor);
        _UpDownArrows.Add(obj.GetComponent<UpDownArrow>());
      }

      while (_UpDownArrows.Count > symbolsThisRound && _UpDownArrows.Count > 0) {
        GameObject.Destroy(_UpDownArrows[0].gameObject);
        _UpDownArrows.RemoveAt(0);
      }

      for (int i = 0; i < symbolsThisRound; i++) {
        _UpDownArrows[i].HideAll();
      }
    }

    private void PlayRobotCalibrationResponseAnim(bool ignoreCallback = false) {
      for (int i = 0; i < _UpDownArrows.Count; i++) {
        _UpDownArrows[i].HideAll();
      }

      _WaitingForAnimationsToBeRunnable = false;
      _RobotResponseIndex = 0;

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        // Ensure that Cozmo is upright before playing calibration response animations
        if (_RobotOffTreadsState != OffTreadsState.OnTreads) {
          robot.WaitAction(1, PlayRobotCalibrationResponseAnim);
          _WaitingForAnimationsToBeRunnable = true;
          return;
        }

        // Get the latest bracket value
        NeedsStateManager nsm = NeedsStateManager.Instance;
        bool severe = nsm.PopLatestEngineValue(NeedId.Repair).Bracket == NeedBracketId.Critical;

        // Start with a get in
        robot.SendAnimationTrigger((severe) ? AnimationTrigger.RepairFixSevereGetReady : AnimationTrigger.RepairFixMildGetReady,
                                   null,
                                   QueueActionPosition.AT_END);

        // Play an animation for every arrow
        int finalAnimIdx = _TuneUpPatternToMatch.Count - 1;
        for (int i = 0; i < finalAnimIdx; i++) {
          robot.SendAnimationTrigger(GetRobotArrowAnimation(_TuneUpPatternToMatch[i], severe),
                                     HandleCalibrateAnimPlayed,
                                     QueueActionPosition.AT_END);
        }

        // Play a round reaction if it's not the last round
        if (_RoundIndex < (_NumberOfRepairRounds - 1)) {
          robot.SendAnimationTrigger(GetRobotArrowAnimation(_TuneUpPatternToMatch[finalAnimIdx], severe),
            HandleCalibrateAnimPlayed,
            QueueActionPosition.AT_END);

          robot.SendAnimationTrigger((severe) ? AnimationTrigger.RepairFixSevereRoundReact : AnimationTrigger.RepairFixMildRoundReact,
            HandleRobotResponseDone,
            QueueActionPosition.AT_END);
        }
        else {
          robot.SendAnimationTrigger(GetRobotArrowAnimation(_TuneUpPatternToMatch[finalAnimIdx], severe),
            HandleRobotResponseDone,
            QueueActionPosition.AT_END);
        }
      }
    }

    private AnimationTrigger GetRobotArrowAnimation(ArrowInput arrowInput, bool isSevere) {
      AnimationTrigger animationTrigger = AnimationTrigger.RepairFixMildHeadUp;
      if (arrowInput == ArrowInput.Up) {
        if (isSevere) {
          switch (_PartToRepair) {
          case RepairablePartId.Head:
            animationTrigger = AnimationTrigger.RepairFixSevereHeadUp;
            break;
          case RepairablePartId.Lift:
            animationTrigger = AnimationTrigger.RepairFixSevereLiftUp;
            break;
          case RepairablePartId.Treads:
            animationTrigger = AnimationTrigger.RepairFixSevereWheelsForward;
            break;
          default:
            break;
          }
        }
        else { // mild
          switch (_PartToRepair) {
          case RepairablePartId.Head:
            animationTrigger = AnimationTrigger.RepairFixMildHeadUp;
            break;
          case RepairablePartId.Lift:
            animationTrigger = AnimationTrigger.RepairFixMildLiftUp;
            break;
          case RepairablePartId.Treads:
            animationTrigger = AnimationTrigger.RepairFixMildWheelsForward;
            break;
          default:
            break;
          }
        }
      }
      else { // arrowInput == ArrowInput.Down
        if (isSevere) {
          switch (_PartToRepair) {
          case RepairablePartId.Head:
            animationTrigger = AnimationTrigger.RepairFixSevereHeadDown;
            break;
          case RepairablePartId.Lift:
            animationTrigger = AnimationTrigger.RepairFixSevereLiftDown;
            break;
          case RepairablePartId.Treads:
            animationTrigger = AnimationTrigger.RepairFixSevereWheelsBack;
            break;
          default:
            break;
          }
        }
        else { // mild
          switch (_PartToRepair) {
          case RepairablePartId.Head:
            animationTrigger = AnimationTrigger.RepairFixMildHeadDown;
            break;
          case RepairablePartId.Lift:
            animationTrigger = AnimationTrigger.RepairFixMildLiftDown;
            break;
          case RepairablePartId.Treads:
            animationTrigger = AnimationTrigger.RepairFixMildWheelsBack;
            break;
          default:
            break;
          }
        }
      }
      return animationTrigger;
    }


    // returns true if a severity transition is necessary and queued
    // false otherwise
    private bool PlaySeverityTransitionIfNecessary(RobotCallback callback = null) {
      NeedBracketId lastBracket = _LastBracketTransitionedInto;
      bool severityChanged = UpdateSeverityBracket();
      NeedBracketId newBracket = _LastBracketTransitionedInto;

      //if our severity has changed, play get out and get in anims
      if (severityChanged) {
        PlayGetOutAnim(lastBracket, callback);

        var robot = RobotEngineManager.Instance.CurrentRobot;
        robot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kMinigameId);
        SetDisableReactions(robot, newBracket);
        return true;
      }
      return false;
    }


    // Return strue if severity has changed since the last time this function was called
    private bool UpdateSeverityBracket() {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedBracketId bracket = nsm.PopLatestEngineValue(NeedId.Repair).Bracket;

      bool severityChanged = _LastBracketTransitionedInto != bracket;
      //for animation purposees, warning and normal brackets are both mild
      if ((_LastBracketTransitionedInto == NeedBracketId.Warning && bracket == NeedBracketId.Normal)
        || (bracket == NeedBracketId.Warning && _LastBracketTransitionedInto == NeedBracketId.Normal)) {
        severityChanged = false;
      }

      _LastBracketTransitionedInto = bracket;
      return severityChanged;
    }

    private void UpdateRobotRepairIdleAnim() {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (!_WaitForDropCube && robot != null) {
        PlaySeverityTransitionIfNecessary();

        NeedsStateManager nsm = NeedsStateManager.Instance;
        NeedBracketId bracket = nsm.PopLatestEngineValue(NeedId.Repair).Bracket;

        AnimationTrigger idle = AnimationTrigger.RepairFixMildIdle;
        switch (bracket) {
        case NeedBracketId.Full:
          idle = AnimationTrigger.RepairIdleFullyRepaired;
          break;
        case NeedBracketId.Critical:
          idle = AnimationTrigger.RepairFixSevereIdle;
          break;
        }
        robot.PushIdleAnimation(idle, kNeedsRepairIdleLock);
      }
    }

    private void PlayGetOutAnim(NeedBracketId bracket, RobotCallback callback = null) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        switch (bracket) {
        case NeedBracketId.Critical:
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereGetOut, callback, QueueActionPosition.AT_END);
          break;
        case NeedBracketId.Normal:
        case NeedBracketId.Warning:
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildGetOut, callback, QueueActionPosition.AT_END);
          break;
        case NeedBracketId.Full:
          // We need the callback even though we don't have a get in
          robot.SendAnimationTrigger(AnimationTrigger.Count, callback, QueueActionPosition.AT_END);
          break;
        }
      }
    }

    private void PlayGetInAnim(NeedBracketId bracket, RobotCallback callback = null) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        switch (bracket) {
        case NeedBracketId.Critical:
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereGetIn, callback, QueueActionPosition.AT_END);
          break;
        case NeedBracketId.Normal:
        case NeedBracketId.Warning:
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildGetIn, callback, QueueActionPosition.AT_END);
          break;
        case NeedBracketId.Full:
          // We need the callback even though we don't have a get in
          robot.SendAnimationTrigger(AnimationTrigger.Count, callback, QueueActionPosition.AT_END);
          break;
        }
      }
    }

    private ArrowInput GetNextSequenceRand() {
      const int kMaxStreak = 2;
      int currIndex = _TuneUpPatternToMatch.Count;
      if (currIndex - kMaxStreak - 1 >= 0) {
        // in event of a streak, make sure we switch
        bool streakFound = true;
        for (int i = currIndex - kMaxStreak; i < currIndex; ++i) {
          streakFound &= (_TuneUpPatternToMatch[i] == _TuneUpPatternToMatch[i - 1]);
        }
        if (streakFound) {
          return _TuneUpPatternToMatch[currIndex - 1] == ArrowInput.Up ? ArrowInput.Down : ArrowInput.Up;
        }
      }
      return Random.value <= 0.5 ? ArrowInput.Up : ArrowInput.Down; ;
    }

    private void ScanSoundComplete(CallbackInfo callbackInfo) {
      if (!IsClosed) {
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Scan_Complete);
      }
    }

    private bool ShowDontMoveCozmoAlert(OffTreadsState offTreadsState) {
      if (_CozmoMovedReactionsInterrupt) {
        DAS.Event("NeedsRepairModal.ShowReactionAlert.OffTreads", DASEventDialogName, DASUtil.FormatExtraData(offTreadsState.ToString()));
        if (_InterruptedAlert == null) {
          ShowInterruptionAlert("cozmo_off_treads_by_user_alert", LocalizationKeys.kNeedsNeedsRepairModalCozmoNotOnTreadsTitle,
                                LocalizationKeys.kNeedsNeedsRepairModalCozmoNotOnTreadsBody);
        }
        return true;
      }
      return false;
    }

    private void ShowInterruptionAlert(string dasAlertName, string titleKey, string descriptionKey) {
      if (_InterruptedAlert == null) {
        var interruptedAlertData = new AlertModalData(dasAlertName, titleKey, descriptionKey);

        var interruptedAlertPriorityData = new ModalPriorityData(ModalPriorityLayer.High, 0,
                                                                 LowPriorityModalAction.CancelSelf,
                                                                 HighPriorityModalAction.Stack);

        System.Action<AlertModal> interruptedAlertCreated = (alertModal) => {
          alertModal.ModalClosedWithCloseButtonOrOutsideAnimationFinished += HandleInterruptionAlertClosed;
          _InterruptedAlert = alertModal;
          GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Game_End);
        };

        UIManager.OpenAlert(interruptedAlertData, interruptedAlertPriorityData, interruptedAlertCreated,
                  overrideCloseOnTouchOutside: false);
      }
    }

    private void HandleInterruptionAlertClosed() {
      _InterruptedAlert = null;
    }

    #endregion //Misc Helper Methods
  }
}
