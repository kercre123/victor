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

namespace Cozmo.Repair.UI {

  public enum ArrowInput {
    Up,
    Down
  };

  public class NeedsRepairModal : BaseModal {

    #region NESTED DEFINITIONS

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
      REVEAL_SYMBOLS,   //reveal one symbol at a time until all showing
      ENTER_SYMBOLS,    //user hitting symbol buttons to match pattern
      PATTERN_MISMATCH, //user submitted incorrect symbol
      PATTERN_MATCHED,  //full pattern entered successfully, increment round cleared icons
      ROBOT_RESPONSE,   //tell robot to animate the pattern and wait with screen dulled
      TUNE_UP_COMPLETE  //show any activity complete effects and robot anims
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

    #endregion

    #region SERIALIZED FIELDS

    [SerializeField]
    private NeedsMeter _RepairMeter = null;

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
    private float _ScanDuration = 3f; //in seconds

    [SerializeField]
    private float _MismatchDisplayDuration = 2f; //in seconds

    [SerializeField]
    private float _RobotResponseMaxTime = 10f; //in seconds

    [SerializeField]
    private float _InactivityTimeOut = 300f; //in seconds

    [SerializeField]
    private bool _CozmoMovedReactionsInterrupt = false;

    #endregion

    #region NON-SERIALIZED FIELDS

    private NeedBracketId _LastBracket = NeedBracketId.Count;

    private RepairModalState _CurrentModalState = RepairModalState.PRE_SCAN;
    private float _TimeInModalState = 0f;

    //framewise inputs to compartmentalize transition logic
    private bool _OpeningTweenComplete = false;
    private bool _ScanRequested = false;
    private bool _RepairNeeded = false;
    private bool _RepairRequested = false;
    private bool _RepairCompleted = false;
    private bool _RepairInterrupted = false;

    private TuneUpState _CurrentTuneUpState = TuneUpState.INTRO;
    private float _TimeInTuneUpState = 0f;

    private bool _MismatchDetected = false;
    private bool _FullPatternMatched = false;
    private bool _CalibrationRequested = false;
    private bool _RobotResponseDone = false;

    private RepairablePartId _PartToRepair = RepairablePartId.Head;
    private NeedsActionId _PartRepairAction = NeedsActionId.RepairHead;

    private List<ArrowInput> _TuneUpPatternToMatch = new List<ArrowInput>();
    private List<ArrowInput> _TuneUpValuesEntered = new List<ArrowInput>();

    private float _RevealTimer = 0f;
    private float _RevealProgressMaxWidth = 869f;
    private int _SymbolsShown = 0;
    private int _RoundIndex = 0;
    private int _RobotResponseIndex = 0;

    private List<RoundProgressPip> _RoundProgressPips = new List<RoundProgressPip>();
    private List<UpDownArrow> _UpDownArrows = new List<UpDownArrow>();

    private bool _Closed = false; //no need to refresh state logic after dialog close
    private AlertModal _InterruptedAlert = null;

    private bool _MeterPaused = false; //pause meter in certain states
    private bool _WaitForDropCube = false;

    //when this timer reaches zero, the modal will close
    //refreshes to _InactivityTimeOut on any touch or state change
    private float _InactivityTimer = float.MaxValue;

    #endregion

    #region LIFE SPAN

    public void InitializeRepairModal() {
      HubWorldBase.Instance.StopFreeplay();
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.CancelAllCallbacks();
        robot.CancelAction(RobotActionType.UNKNOWN);

        HubWorldBase.Instance.StopFreeplay();

        robot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kMinigameId,
                                       ReactionaryBehaviorEnableGroups.kDefaultMinigameTriggers);

        if (robot.Status(RobotStatusFlag.IS_CARRYING_BLOCK)) {
          _WaitForDropCube = true;
          robot.PlaceObjectOnGroundHere((success) => {
            _WaitForDropCube = false;
            PlayRobotRepairIdleAnim();
          });
        }
        else {
          PlayRobotRepairIdleAnim();
        }
      }

      NeedsStateManager nsm = NeedsStateManager.Instance;
      nsm.PauseExceptForNeed(NeedId.Repair);

      _RepairMeter.Initialize(allowInput: false,
                              buttonClickCallback: null,
                              dasButtonName: "repair_need_meter_button",
                              dasParentDialogName: DASEventDialogName);

      _RepairMeter.ProgressBar.SetValueInstant(nsm.GetCurrentDisplayValue(NeedId.Repair).Value);

      InitializePartElements(HandleRepairHeadButtonPressed, "repair_head_button", _HeadElements);
      InitializePartElements(HandleRepairLiftButtonPressed, "repair_lift_button", _LiftElements);
      InitializePartElements(HandleRepairTreadsButtonPressed, "repair_treads_button", _TreadsElements);

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

      while (_RoundProgressPips.Count < _RoundData.Length) {
        GameObject obj = GameObject.Instantiate(_RoundPipPrefab, _RoundPipAnchor);
        RoundProgressPip toggler = obj.GetComponent<RoundProgressPip>();
        _RoundProgressPips.Add(toggler);
        toggler.SetComplete(false);
      }

      _RobotRespondingDimmerPanel.SetActive(false);

      _ContinueButton.Initialize(HandleUserClose, "repair_continue_button", DASEventDialogName);

      _RevealProgressMaxWidth = _RevealProgressBar.sizeDelta.x;

      EnterModalState(); //do any state set up if needed for our initial state
    }

    protected override void RaiseDialogOpenAnimationFinished() {
      base.RaiseDialogOpenAnimationFinished();
      NeedsStateManager nsm = NeedsStateManager.Instance;
      if (nsm != null) {
        nsm.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;
      }
      HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
      _OpeningTweenComplete = true;
    }

    private void Update() {
      if (!_Closed) {
        RefreshModalStateLogic(Time.deltaTime);

        bool interactionDetected = false;
        if (Input.touchSupported) {
          interactionDetected = Input.GetTouch(0).phase == TouchPhase.Began;
        }
        else {
          interactionDetected = Input.GetMouseButtonDown(0);
        }

        if (interactionDetected) {
          _InactivityTimer = _InactivityTimeOut;
        }
        else {
          _InactivityTimer -= Time.deltaTime;

          if (_InactivityTimer <= 0f) {
            HandleUserClose();
          }
        }
      }
    }

    protected override void RaiseDialogClosed() {
      ExitModalState(); //ensure that we do any state clean up before killing our fsm
      _Closed = true;

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
        robot.CancelAction(RobotActionType.PLAY_ANIMATION);
        PlayGetOutAnim(_LastBracket);
        robot.SetIdleAnimation(AnimationTrigger.Count);
        HubWorldBase.Instance.StartFreeplay();
        robot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kMinigameId);
      }

      base.RaiseDialogClosed();
    }

    private void OnApplicationPause(bool pauseStatus) {
      DAS.Debug("NeedsRepairModal.OnApplicationPause", "Application pause: " + pauseStatus);
      HandleUserClose();
    }

    #endregion

    #region MODAL FSM

    private void RefreshModalStateLogic(float deltaTime) {
      RefreshModalStateInputs();
      bool stateChanged = ModalStateTransition();
      ClearModalStateInputs();

      if (!stateChanged) {
        UpdateModalState(deltaTime);
      }
    }

    private void RefreshModalStateInputs() {
      _RepairCompleted = _CurrentModalState == RepairModalState.TUNE_UP && _CurrentTuneUpState == TuneUpState.TUNE_UP_COMPLETE;
    }

    private void ClearModalStateInputs() {
      _OpeningTweenComplete = false;
      _ScanRequested = false;
      _RepairRequested = false;
      _RepairCompleted = false;
      _RepairInterrupted = false;
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
        if (_ScanRequested) {
          return ChangeModalState(RepairModalState.SCAN);
        }
        break;
      case RepairModalState.SCAN:
        if (_TimeInModalState >= _ScanDuration) {
          if (!_RepairNeeded) {
            return ChangeModalState(RepairModalState.REPAIRED);
          }
        }
        if (_RepairRequested) {
          return ChangeModalState(RepairModalState.TUNE_UP);
        }
        break;
      case RepairModalState.TUNE_UP:
        if (_RepairCompleted || _RepairInterrupted) {
          if (!_RepairNeeded) {
            return ChangeModalState(RepairModalState.REPAIRED);
          }

          return ChangeModalState(RepairModalState.SCAN);
        }
        break;
      case RepairModalState.REPAIRED:
        if (_RepairNeeded) {
          return ChangeModalState(RepairModalState.SCAN);
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
      _TimeInModalState = 0f;

      _InactivityTimer = _InactivityTimeOut;

      _ModalStateAnimator.SetInteger("ModalState", (int)_CurrentModalState);
      _ModalStateAnimator.SetTrigger("StateChange");

      switch (_CurrentModalState) {
      case RepairModalState.PRE_SCAN:
        _ScanButton.gameObject.SetActive(true);
        _ScanButton.interactable = true;
        break;
      case RepairModalState.SCAN:
        _MeterPaused = true;
        RefreshAllPartElements();
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Scan, Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete, ScanSoundComplete);
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

        RobotEngineManager.Instance.AddCallback<ReactionTriggerTransition>(HandleRobotReactionaryBehavior);

        EnterTuneUpState();
        break;
      case RepairModalState.REPAIRED:
        HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Level_Success);
        break;
      }
    }

    private void UpdateModalState(float deltaTime) {
      _TimeInModalState += deltaTime;
      switch (_CurrentModalState) {
      case RepairModalState.PRE_SCAN: break;
      case RepairModalState.SCAN:
        if (_MeterPaused && _TimeInModalState >= _ScanDuration) {
          _MeterPaused = false;
          HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
        }
        break;
      case RepairModalState.TUNE_UP:
        //pausing tune up activity when interrupt widget open
        if (_InterruptedAlert == null) {
          RefreshTuneUpStateLogic(deltaTime);
        }
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
        _MeterPaused = false;
        HandleLatestNeedsLevelChanged(NeedsActionId.NoAction);
        break;
      case RepairModalState.TUNE_UP:
        ExitTuneUpState();

        RobotEngineManager.Instance.RemoveCallback<ReactionTriggerTransition>(HandleRobotReactionaryBehavior);

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

    #endregion

    #region TUNE-UP SUB-FSM

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
        if (_TuneUpValuesEntered[i] == _TuneUpPatternToMatch[i]) continue;
        _MismatchDetected = true;
        break;
      }

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null && ShowReactionAlert(robot.CurrentReactionTrigger)) {
        _RepairInterrupted = true;
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
        if (_TimeInTuneUpState >= 1f) {
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
        if (_TimeInTuneUpState >= _MismatchDisplayDuration) {
          return ChangeTuneUpState(TuneUpState.REVEAL_SYMBOLS);
        }
        break;
      case TuneUpState.PATTERN_MATCHED:
        if (_CalibrationRequested) {
          return ChangeTuneUpState(TuneUpState.ROBOT_RESPONSE);
        }
        break;
      case TuneUpState.ROBOT_RESPONSE:
        if (_RobotResponseDone || _TimeInTuneUpState >= _RobotResponseMaxTime) {
          if (_RoundIndex < _RoundData.Length - 1) {
            return ChangeTuneUpState(TuneUpState.REVEAL_SYMBOLS);
          }
          return ChangeTuneUpState(TuneUpState.TUNE_UP_COMPLETE);
        }
        break;
      case TuneUpState.TUNE_UP_COMPLETE: break;
      }
      return false;
    }

    private bool ChangeTuneUpState(TuneUpState newState) {
      if (newState == _CurrentTuneUpState) {
        return false;
      }

      ExitTuneUpState();
      _CurrentTuneUpState = newState;
      EnterTuneUpState();

      return true;
    }

    private void EnterTuneUpState() {
      _TimeInTuneUpState = 0f;

      _InactivityTimer = _InactivityTimeOut;

      switch (_CurrentTuneUpState) {
      case TuneUpState.INTRO:
        //let's get our initial symbols spawned early so they can transition on nicely
        int symbolsFirstRound = 0;
        if (_RoundData.Length > 0) {
          symbolsFirstRound = _RoundData[0].NumSymbols;
        }

        RefreshSymbols(symbolsFirstRound);
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

        _TuneUpPatternToMatch.Clear();

        int symbolsThisRound = _RoundData[_RoundIndex].NumSymbols;
        RefreshSymbols(symbolsThisRound);

        for (int i = 0; i < symbolsThisRound; i++) {
          _TuneUpPatternToMatch.Add(GetNextSequenceRand());
        }

        _SymbolsShown = 0;
        //shift these reveals
        _RevealTimer = 0f;//_RoundData[_RoundIndex].SymbolRevealDelay;
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
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Error);
        break;
      case TuneUpState.PATTERN_MATCHED:
        _CalibrateButton.gameObject.SetActive(true);
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Success_Round);
        break;
      case TuneUpState.ROBOT_RESPONSE:
        _RobotRespondingDimmerPanel.SetActive(true);

        for (int i = 0; i < _UpDownArrows.Count; i++) {
          _UpDownArrows[i].HideAll();
        }
        PlayRobotCalibrationResponseAnim();
        _RevealProgressBar.gameObject.SetActive(false);
        break;
      case TuneUpState.TUNE_UP_COMPLETE:
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Success_Complete);
        _MeterPaused = true;
        NeedsStateManager.Instance.RegisterNeedActionCompleted(_PartRepairAction);
        break;
      }
    }

    private void UpdateTuneUpState(float deltaTime) {
      _TimeInTuneUpState += deltaTime;
      switch (_CurrentTuneUpState) {
      case TuneUpState.INTRO: break;
      case TuneUpState.REVEAL_SYMBOLS:

        if (_RevealTimer >= 0f) {
          _RevealTimer -= deltaTime;
          if (_RevealTimer < 0f) {
            if (_SymbolsShown < _TuneUpPatternToMatch.Count) {
              ArrowInput arrow = _TuneUpPatternToMatch[_SymbolsShown];
              _UpDownArrows[_SymbolsShown].Reveal(arrow);
              _SymbolsShown++;
              _RevealTimer = _RoundData[_RoundIndex].SymbolRevealDelay;

              switch (arrow) {
              case ArrowInput.Up:
                Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Up);
                break;
              case ArrowInput.Down:
                Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Down);
                break;
              }
            }
          }
        }

        float revealProgressFactor = 1f;
        float totalDuration = (_RoundData[_RoundIndex].NumSymbols - 1) * _RoundData[_RoundIndex].SymbolRevealDelay;
        float timeSinceFirstArrow = _TimeInTuneUpState;
        if (totalDuration > 0f) {
          revealProgressFactor = Mathf.Clamp01(timeSinceFirstArrow / totalDuration);
        }

        _RevealProgressBar.sizeDelta = new Vector2(revealProgressFactor * _RevealProgressMaxWidth,
                                                         _RevealProgressBar.sizeDelta.y);

        break;
      case TuneUpState.ENTER_SYMBOLS: break;
      case TuneUpState.PATTERN_MISMATCH: break;
      case TuneUpState.PATTERN_MATCHED: break;
      case TuneUpState.ROBOT_RESPONSE: break;
      case TuneUpState.TUNE_UP_COMPLETE: break;
      }
    }

    private void ExitTuneUpState() {
      switch (_CurrentTuneUpState) {
      case TuneUpState.INTRO: break;
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
      case TuneUpState.ROBOT_RESPONSE:
        if (_RoundIndex < _RoundProgressPips.Count) {
          _RoundProgressPips[_RoundIndex].SetComplete(true);
        }
        _RoundIndex++;
        _RobotRespondingDimmerPanel.SetActive(false);

        var robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot != null) {
          robot.CancelCallback(HandleCalibrateAnimMiddle);
          robot.CancelCallback(HandleRobotResponseDone);
        }
        PlayRobotRepairIdleAnim();
        break;
      case TuneUpState.TUNE_UP_COMPLETE:
        break;
      }
    }

    #endregion

    #region BUTTON CLICK HANDLERS

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
    }

    private void HandleDownButtonPressed() {
      AppendTuneUpEntry(ArrowInput.Down);
    }

    private void HandleCalibrateButtonPressed() {
      _CalibrationRequested = true;
    }

    #endregion

    #region ROBOT CALLBACK HANDLERS

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      NeedsStateManager nsm = NeedsStateManager.Instance;

      PlayRobotRepairIdleAnim();

      RefreshAllPartElements();

      if (!_MeterPaused) {
        _RepairMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Repair).Value);
      }
    }

    // light up the next arrow, note: this may be removed if deemed too distracting by design
    private void HandleCalibrateAnimMiddle(bool success) {
      if (_RobotResponseIndex < _UpDownArrows.Count) {
        _UpDownArrows[_RobotResponseIndex].Calibrated(_TuneUpPatternToMatch[_RobotResponseIndex]);
      }
      _RobotResponseIndex++;

      if (_RobotResponseIndex >= _UpDownArrows.Count && _RoundIndex >= _RoundData.Length - 1) {
        NeedsStateManager nsm = NeedsStateManager.Instance;
        bool severe = nsm.PopLatestEngineValue(NeedId.Repair).Bracket == NeedBracketId.Critical;
        if (severe) {
          HandleRobotResponseDone(true);
        }
      }
    }

    private void HandleRobotResponseDone(bool success) {
      _RobotResponseDone = true;
    }

    private void HandleRobotReactionaryBehavior(object messageObject) {
      ReactionTriggerTransition behaviorTransition = messageObject as ReactionTriggerTransition;
      if (behaviorTransition.newTrigger == ReactionTrigger.ReturnedToTreads) {
        var robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot != null) {
          robot.TryResetHeadAndLift(null);
        }
      }

      //only let certain reactions interrupt this modal
      if (behaviorTransition.oldTrigger == ReactionTrigger.NoneTrigger) {
        ShowReactionAlert(behaviorTransition.newTrigger);
      }
    }

    #endregion

    #region MISC HELPER METHODS

    private void DisableAllRepairButtons() {
      foreach (CozmoButton button in _HeadElements.Buttons) {
        button.Interactable = false;
      }
      foreach (CozmoButton button in _LiftElements.Buttons) {
        button.Interactable = false;
      }
      foreach (CozmoButton button in _TreadsElements.Buttons) {
        button.Interactable = false;
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
            Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Up);
          }
          else {
            Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Down);
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

    private void RefreshAllPartElements() {
      bool headBroken = NeedsStateManager.Instance.PopLatestEnginePartIsBroken(RepairablePartId.Head);
      RefreshPartElements(headBroken, _HeadElements);

      bool liftBroken = NeedsStateManager.Instance.PopLatestEnginePartIsBroken(RepairablePartId.Lift);
      RefreshPartElements(liftBroken, _LiftElements);

      bool treadsBroken = NeedsStateManager.Instance.PopLatestEnginePartIsBroken(RepairablePartId.Treads);
      RefreshPartElements(treadsBroken, _TreadsElements);

      _RepairNeeded = headBroken || liftBroken || treadsBroken;
      _RepairButtonsInstruction.gameObject.SetActive(_RepairNeeded);
      _NoRepairsNeededNotice.gameObject.SetActive(!_RepairNeeded);
    }

    private void RefreshPartElements(bool partBroken, RepairPartElements elements) {
      foreach (CozmoButton button in elements.Buttons) {
        button.Interactable = partBroken;
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

    private void PlayRobotCalibrationResponseAnim() {
      _RobotResponseIndex = 0;

      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Calibrate);

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {

        robot.CancelAction(RobotActionType.PLAY_ANIMATION);

        //start with intro anims
        NeedsStateManager nsm = NeedsStateManager.Instance;
        bool severe = nsm.PopLatestEngineValue(NeedId.Repair).Bracket == NeedBracketId.Critical;

        if (severe) {
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereGetReady, null, QueueActionPosition.AT_END);
        }
        else {
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildGetReady, null, QueueActionPosition.AT_END);
        }

        switch (_PartToRepair) {
        case RepairablePartId.Head:
          for (int i = 0; i < _TuneUpPatternToMatch.Count; ++i) {
            if (_TuneUpPatternToMatch[i] == ArrowInput.Up) {
              if (severe) {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereHeadUp, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
              else {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildHeadUp, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
            }
            else {
              if (severe) {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereHeadDown, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
              else {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildHeadDown, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
            }
          }
          robot.SetHeadAngle(0.0f, null, QueueActionPosition.AT_END);
          break;
        case RepairablePartId.Lift:

          //fix lift height
          if (severe) {
            robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereRaiseLift, null, QueueActionPosition.AT_END);
          }
          else {
            robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildRaiseLift, null, QueueActionPosition.AT_END);
          }

          for (int i = 0; i < _TuneUpPatternToMatch.Count; ++i) {
            if (_TuneUpPatternToMatch[i] == ArrowInput.Up) {
              if (severe) {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereLiftUp, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
              else {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildLiftUp, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
            }
            else {
              if (severe) {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereLiftDown, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
              else {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildLiftDown, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
            }
          }
          robot.SetLiftHeight(0.0f, null, QueueActionPosition.AT_END);
          break;
        case RepairablePartId.Treads:
          for (int i = 0; i < _TuneUpPatternToMatch.Count; ++i) {
            if (_TuneUpPatternToMatch[i] == ArrowInput.Up) {
              if (severe) {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereWheelsForward, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
              else {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildWheelsForward, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
            }
            else {
              if (severe) {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereWheelsBack, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
              else {
                robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildWheelsBack, HandleCalibrateAnimMiddle, QueueActionPosition.AT_END);
              }
            }
          }
          break;
        }

        // Always end with a victory...
        if (severe) {
          if (_RoundIndex < _RoundData.Length - 1) {
            robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereRoundReact, HandleRobotResponseDone, QueueActionPosition.AT_END);
          }
        }
        else {
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildRoundReact, HandleRobotResponseDone, QueueActionPosition.AT_END);
        }
      }
    }

    private void PlayRobotRepairIdleAnim() {

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (!_WaitForDropCube && robot != null) {

        NeedsStateManager nsm = NeedsStateManager.Instance;
        NeedBracketId bracket = nsm.PopLatestEngineValue(NeedId.Repair).Bracket;

        bool severityChanged = _LastBracket != bracket;
        //for animation purposees, warning and normal brackets are both mild
        if ((_LastBracket == NeedBracketId.Warning && bracket == NeedBracketId.Normal)
            || (bracket == NeedBracketId.Warning && _LastBracket == NeedBracketId.Normal)) {
          severityChanged = false;
        }

        //if our severity has changed, play get out and get in anims
        if (severityChanged) {
          PlayGetOutAnim(_LastBracket);
          PlayGetInAnim(bracket);
        }

        AnimationTrigger idle = AnimationTrigger.RepairFixMildIdle;
        switch (bracket) {
        case NeedBracketId.Full:
          idle = AnimationTrigger.RepairIdleFullyRepaired;
          break;
        case NeedBracketId.Critical:
          idle = AnimationTrigger.RepairFixSevereIdle;
          break;
        }

        robot.SetIdleAnimation(idle);

        _LastBracket = bracket;
      }
    }

    private void PlayGetOutAnim(NeedBracketId bracket) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        switch (bracket) {
        case NeedBracketId.Critical:
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereGetOut, null, QueueActionPosition.AT_END);
          break;
        case NeedBracketId.Normal:
        case NeedBracketId.Warning:
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildGetOut, null, QueueActionPosition.AT_END);
          break;
        }
      }
    }

    private void PlayGetInAnim(NeedBracketId bracket) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        switch (bracket) {
        case NeedBracketId.Critical:
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixSevereGetIn, null, QueueActionPosition.AT_END);
          break;
        case NeedBracketId.Normal:
        case NeedBracketId.Warning:
          robot.SendAnimationTrigger(AnimationTrigger.RepairFixMildGetIn, null, QueueActionPosition.AT_END);
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

    private void ScanSoundComplete(Anki.Cozmo.Audio.CallbackInfo callbackInfo) {
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Scan_Complete);
    }

    private bool ShowReactionAlert(ReactionTrigger trigger) {

      if (_CozmoMovedReactionsInterrupt) {
        switch (trigger) {
        case ReactionTrigger.RobotPickedUp:
        case ReactionTrigger.PlacedOnCharger:
        case ReactionTrigger.RobotOnBack:
        case ReactionTrigger.RobotOnFace:
        case ReactionTrigger.RobotOnSide:
          DAS.Event("robot.interrupt", DASEventDialogName, DASUtil.FormatExtraData(trigger.ToString()));
          ShowDontMoveCozmoAlert();
          return true;
        }
      }

      return false;
    }

    private void ShowDontMoveCozmoAlert() {
      if (_InterruptedAlert == null) {
        ShowInterruptionAlert("cozmo_moved_by_user_alert", LocalizationKeys.kMinigameDontMoveCozmoTitle,
                                           LocalizationKeys.kMinigameDontMoveCozmoDescription);
      }
    }

    private void ShowInterruptionAlert(string dasAlertName, string titleKey, string descriptionKey) {
      CreateInterruptionAlert(dasAlertName, titleKey, descriptionKey);
    }

    private void CreateInterruptionAlert(string dasAlertName, string titleKey, string descriptionKey) {
      if (_InterruptedAlert == null) {
        var interruptedAlertData = new AlertModalData(dasAlertName, titleKey, descriptionKey,
                                new AlertModalButtonData("okay_button", LocalizationKeys.kButtonOkay,
                                             clickCallback: CloseInterruptionAlert));

        var interruptedAlertPriorityData = new ModalPriorityData(ModalPriorityLayer.High, 0,
                                     LowPriorityModalAction.Queue,
                                     HighPriorityModalAction.Stack);

        System.Action<AlertModal> interruptedAlertCreated = (alertModal) => {
          alertModal.ModalClosedWithCloseButtonOrOutsideAnimationFinished += CloseInterruptionAlert;
          alertModal.ModalForceClosedAnimationFinished += () => {
            _InterruptedAlert = null;
            CreateInterruptionAlert(dasAlertName, titleKey, descriptionKey);
          };
          _InterruptedAlert = alertModal;
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Game_End);
        };

        UIManager.OpenAlert(interruptedAlertData, interruptedAlertPriorityData, interruptedAlertCreated,
                  overrideCloseOnTouchOutside: false);
      }
    }

    private void CloseInterruptionAlert() {
      if (_InterruptedAlert != null) {
        UIManager.CloseModal(_InterruptedAlert);
        _InterruptedAlert = null;
      }
      _RepairInterrupted = true;
    }

    #endregion

  }
}