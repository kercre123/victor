using System.Collections.Generic;
using System.Linq;
using Anki.Cozmo;
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;
using UnityEngine.Events;

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
    private CozmoButton _DownButton = null;

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
    private CanvasGroup _RobotRespondingGroup = null;

    [SerializeField]
    private CozmoButton _ContinueButton = null;

    [SerializeField]
    private RectTransform _RevealProgressTransform = null;

    [SerializeField]
    private float _ScanDuration = 3f; //in seconds

    [SerializeField]
    private float _MismatchDisplayDuration = 2f;

    [SerializeField]
    private float _RobotResponseMaxTime = 10f;

    [SerializeField]
    private float _LiftTooHighToTuneUp_mm = 60f;

    [SerializeField]
    private float _LiftTooLowToTuneUp_mm = 55f;

    #endregion

    #region NON-SERIALIZED FIELDS

    private RepairModalState _CurrentModalState = RepairModalState.PRE_SCAN;
    private float _TimeInModalState = 0f;

    //framewise inputs to compartmentalize transition logic
    private bool bOpeningTweenComplete = false;
    private bool bScanRequested = false;
    private bool bRepairNeeded = false;
    private bool bRepairRequested = false;
    private bool bRepairCompleted = false;

    private TuneUpState _CurrentTuneUpState = TuneUpState.INTRO;
    private float _TimeInTuneUpState = 0f;

    private bool bMismatchDetected = false;
    private bool bFullPatternMatched = false;
    private bool bCalibrationRequested = false;
    private bool bRobotResponseDone = false;

    private RepairablePartId _PartToRepair = RepairablePartId.Head;
    private NeedsActionId _PartRepairAction = NeedsActionId.RepairHead;

    private List<ArrowInput> _TuneUpPatternToMatch = new List<ArrowInput>();
    private List<ArrowInput> _TuneUpValuesEntered = new List<ArrowInput>();

    private float _RevealTimer = 0f;
    private float _RevealMaskWidth = 869f;
    private int _SymbolsShown = 0;
    private int _RoundIndex = 0;
    private int _RobotResponseIndex = 0;

    private List<RoundProgressPip> _RoundProgressPips = new List<RoundProgressPip>();
    private List<UpDownArrow> _UpDownArrows = new List<UpDownArrow>();

    private bool _Closed = false; //no need to refresh state logic after dialog close

    #endregion

    #region LIFE SPAN

    public void InitializeRepairModal() {

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.SetEnableFreeplayActivity(false);
        robot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kMinigameId, ReactionaryBehaviorEnableGroups.kDefaultMinigameTriggers);
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

      _DownButton.Initialize(HandleDownButtonPressed, "repair_down_button", DASEventDialogName);
      _DownButton.gameObject.SetActive(false);

      _CalibrateButton.Initialize(HandleCalibrateButtonPressed, "repair_callibrate_button", DASEventDialogName);
      _CalibrateButton.gameObject.SetActive(false);

      while (_RoundProgressPips.Count < _RoundData.Length) {
        GameObject obj = GameObject.Instantiate(_RoundPipPrefab, _RoundPipAnchor);
        RoundProgressPip toggler = obj.GetComponent<RoundProgressPip>();
        _RoundProgressPips.Add(toggler);
        toggler.SetComplete(false);
      }

      _RobotRespondingGroup.alpha = 0f;

      _ContinueButton.Initialize(HandleUserClose, "repair_continue_button", DASEventDialogName);

      _RevealMaskWidth = _RevealProgressTransform.sizeDelta.x;

      EnterModalState(); //do any state set up if needed for our initial state
    }

    protected override void RaiseDialogOpenAnimationFinished() {
      base.RaiseDialogOpenAnimationFinished();
      NeedsStateManager.Instance.OnNeedsLevelChanged += HandleLatestNeedsLevelChanged;

      RefreshAllPartElements();
      bOpeningTweenComplete = true;
    }

    private void Update() {
      if (!_Closed) RefreshModalStateLogic(Time.deltaTime);
    }

    protected override void RaiseDialogClosed() {
      ExitModalState(); //ensure that we do any state clean up before killing our fsm
      _Closed = true;

      while (_RoundProgressPips.Count > 0) {
        GameObject.Destroy(_RoundProgressPips[0].gameObject);
        _RoundProgressPips.RemoveAt(0);
      }

      NeedsStateManager.Instance.ResumeAllNeeds();
      NeedsStateManager.Instance.OnNeedsLevelChanged -= HandleLatestNeedsLevelChanged;
      //RETURN TO FREEPLAY
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.SetEnableFreeplayLightStates(true);
        robot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kMinigameId);
        robot.SetEnableFreeplayActivity(true);
      }

      base.RaiseDialogClosed();
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
      bRepairCompleted = _CurrentModalState == RepairModalState.TUNE_UP && _CurrentTuneUpState == TuneUpState.TUNE_UP_COMPLETE;
    }

    private void ClearModalStateInputs() {
      bOpeningTweenComplete = false;
      bScanRequested = false;
      bRepairRequested = false;
      bRepairCompleted = false;
    }

    //transitions can ONLY happen within this method
    private bool ModalStateTransition() {
      switch (_CurrentModalState) {
      case RepairModalState.OPENING:
        if (bOpeningTweenComplete) {
          return ChangeModalState(RepairModalState.PRE_SCAN);
        }
        break;
      case RepairModalState.PRE_SCAN:
        if (bScanRequested) {
          return ChangeModalState(RepairModalState.SCAN);
        }
        break;
      case RepairModalState.SCAN:
        if (_TimeInModalState >= _ScanDuration) {
          if (!bRepairNeeded) {
            return ChangeModalState(RepairModalState.REPAIRED);
          }
        }
        if (bRepairRequested) {
          return ChangeModalState(RepairModalState.TUNE_UP);
        }
        break;
      case RepairModalState.TUNE_UP:
        if (bRepairCompleted) {
          if (!bRepairNeeded) {
            return ChangeModalState(RepairModalState.REPAIRED);
          }

          return ChangeModalState(RepairModalState.SCAN);
        }
        break;
      case RepairModalState.REPAIRED:
        if (bRepairNeeded) {
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

      _ModalStateAnimator.SetInteger("ModalState", (int)_CurrentModalState);
      _ModalStateAnimator.SetTrigger("StateChange");

      switch (_CurrentModalState) {
      case RepairModalState.PRE_SCAN:
        _ScanButton.gameObject.SetActive(true);
        _ScanButton.interactable = true;
        break;
      case RepairModalState.SCAN:
        RefreshAllPartElements();
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Scan, Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete, ScanSoundComplete);
        break;
      case RepairModalState.TUNE_UP:
        _TuneUpValuesEntered.Clear();
        _RoundIndex = 0;

        foreach (RoundProgressPip pip in _RoundProgressPips) {
          pip.SetComplete(false);
        }

        _RevealProgressTransform.gameObject.SetActive(false);
        _RevealProgressTransform.sizeDelta = new Vector2(0f, _RevealProgressTransform.sizeDelta.y);

        _HeadElements.TuneUpTitle.gameObject.SetActive(_PartToRepair == RepairablePartId.Head);
        _LiftElements.TuneUpTitle.gameObject.SetActive(_PartToRepair == RepairablePartId.Lift);
        _TreadsElements.TuneUpTitle.gameObject.SetActive(_PartToRepair == RepairablePartId.Treads);

        _CurrentTuneUpState = TuneUpState.INTRO;
        EnterTuneUpState();
        break;
      case RepairModalState.REPAIRED:
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Level_Success);
        break;
      }
    }

    private void UpdateModalState(float deltaTime) {
      _TimeInModalState += deltaTime;
      switch (_CurrentModalState) {
      case RepairModalState.PRE_SCAN: break;
      case RepairModalState.SCAN: break;
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
        break;
      case RepairModalState.TUNE_UP:
        ExitTuneUpState();

        while (_UpDownArrows.Count > 0) {
          GameObject.Destroy(_UpDownArrows[0].gameObject);
          _UpDownArrows.RemoveAt(0);
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
      bFullPatternMatched = false;
      if (_CurrentTuneUpState == TuneUpState.ENTER_SYMBOLS) {
        bFullPatternMatched = _TuneUpValuesEntered.SequenceEqual(_TuneUpPatternToMatch);
      }

      bMismatchDetected = false;
      for (int i = 0; i < _TuneUpValuesEntered.Count; i++) {
        if (_TuneUpValuesEntered[i] == _TuneUpPatternToMatch[i]) continue;
        bMismatchDetected = true;
        break;
      }
    }

    private void ClearTuneUpInputs() {
      bMismatchDetected = false;
      bFullPatternMatched = false;
      bCalibrationRequested = false;
      bRobotResponseDone = false;
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
        if (bMismatchDetected) {
          return ChangeTuneUpState(TuneUpState.PATTERN_MISMATCH);
        }
        if (bFullPatternMatched) {
          return ChangeTuneUpState(TuneUpState.PATTERN_MATCHED);
        }
        break;
      case TuneUpState.PATTERN_MISMATCH:
        if (_TimeInTuneUpState >= _MismatchDisplayDuration) {
          return ChangeTuneUpState(TuneUpState.REVEAL_SYMBOLS);
        }
        break;
      case TuneUpState.PATTERN_MATCHED:
        if (bCalibrationRequested) {
          return ChangeTuneUpState(TuneUpState.ROBOT_RESPONSE);
        }
        break;
      case TuneUpState.ROBOT_RESPONSE:
        if (bRobotResponseDone || _TimeInTuneUpState >= _RobotResponseMaxTime) {
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
      switch (_CurrentTuneUpState) {
      case TuneUpState.INTRO: break;
      case TuneUpState.REVEAL_SYMBOLS:
        _RevealProgressTransform.gameObject.SetActive(false);
        _RevealProgressTransform.sizeDelta = new Vector2(0f, _RevealProgressTransform.sizeDelta.y);

        _UpButton.Interactable = false;
        _DownButton.Interactable = false;
        _UpButton.gameObject.SetActive(true);
        _DownButton.gameObject.SetActive(true);

        _TuneUpPatternToMatch.Clear();

        int symbolsThisRound = _RoundData[_RoundIndex].NumSymbols;
        while (_UpDownArrows.Count < symbolsThisRound) {
          GameObject obj = GameObject.Instantiate(_UpdownArrowPrefab, _UpdownArrowAnchor);
          _UpDownArrows.Add(obj.GetComponent<UpDownArrow>());
        }

        while (_UpDownArrows.Count > symbolsThisRound && _UpDownArrows.Count > 0) {
          GameObject.Destroy(_UpDownArrows[0].gameObject);
          _UpDownArrows.RemoveAt(0);
        }

        for (int i = 0; i < symbolsThisRound; i++) {
          _TuneUpPatternToMatch.Add(GetNextSequenceRand());
          _UpDownArrows[i].HideAll();
        }

        _SymbolsShown = 0;
        _RevealTimer = _RoundData[_RoundIndex].SymbolRevealDelay;
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
        //turn incorrect symbol red
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Pattern_Error);
        break;
      case TuneUpState.PATTERN_MATCHED:
        //show next round completed icon
        _CalibrateButton.gameObject.SetActive(true);
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Success_Round);
        break;
      case TuneUpState.ROBOT_RESPONSE:
        //send calibration anim message to robot
        _RobotRespondingGroup.alpha = 1f;
        PlayRobotCalibrationResponseAnim();
        break;
      case TuneUpState.TUNE_UP_COMPLETE:
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Success_Complete);
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

              float revealProgressFactor = Mathf.Clamp01((float)_SymbolsShown / _RoundData[_RoundIndex].NumSymbols);
              _RevealProgressTransform.sizeDelta = new Vector2(revealProgressFactor * _RevealMaskWidth, _RevealProgressTransform.sizeDelta.y);
              _RevealProgressTransform.gameObject.SetActive(true);

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
        break;
      case TuneUpState.ENTER_SYMBOLS:
        _UpButton.gameObject.SetActive(false);
        _DownButton.gameObject.SetActive(false);
        break;
      case TuneUpState.PATTERN_MISMATCH: break;
      case TuneUpState.PATTERN_MATCHED:
        _CalibrateButton.gameObject.SetActive(false);
        break;
      case TuneUpState.ROBOT_RESPONSE:
        if (_RoundIndex < _RoundProgressPips.Count) {
          _RoundProgressPips[_RoundIndex].SetComplete(true);
        }
        _RoundIndex++;
        _RobotRespondingGroup.alpha = 0f;

        var robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot != null) {
          robot.CancelCallback(HandleCalibrateAnimMiddle);
          robot.CancelCallback(HandleRobotResponseDone);
        }
        break;
      case TuneUpState.TUNE_UP_COMPLETE:
        break;
      }
    }

    #endregion

    #region BUTTON CLICK HANDLERS

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionId) {
      switch (actionId) {
      case NeedsActionId.RepairHead:
      case NeedsActionId.RepairLift:
      case NeedsActionId.RepairTreads:
        NeedsStateManager nsm = NeedsStateManager.Instance;
        _RepairMeter.ProgressBar.SetTargetAndAnimate(nsm.PopLatestEngineValue(NeedId.Repair).Value);
        RefreshAllPartElements();
        break;
      }
    }

    private void HandleScanButtonPressed() {
      bScanRequested = true;
    }

    private void HandleRepairHeadButtonPressed() {
      DisableAllRepairButtons();
      bRepairRequested = true;
      _PartToRepair = RepairablePartId.Head;
      _PartRepairAction = NeedsActionId.RepairHead;
    }

    private void HandleRepairLiftButtonPressed() {
      DisableAllRepairButtons();
      bRepairRequested = true;
      _PartToRepair = RepairablePartId.Lift;
      _PartRepairAction = NeedsActionId.RepairLift;
    }

    private void HandleRepairTreadsButtonPressed() {
      DisableAllRepairButtons();
      bRepairRequested = true;
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
      bCalibrationRequested = true;
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
      if (_CurrentModalState != RepairModalState.TUNE_UP) return;
      if (_CurrentTuneUpState != TuneUpState.ENTER_SYMBOLS) return;
      if (_TuneUpValuesEntered.Count >= _TuneUpPatternToMatch.Count) return;

      _TuneUpValuesEntered.Add(arrow);

      int indexLatest = _TuneUpValuesEntered.Count - 1;
      bool match = _TuneUpPatternToMatch[indexLatest] == _TuneUpValuesEntered[indexLatest];

      //if a match, turn correct arrow green
      if (match) {
        _UpDownArrows[indexLatest].Matched(_TuneUpValuesEntered[indexLatest]);
      }
      else {
        //if a mismatch, turn the chosen incorrect arrow orange
        _UpDownArrows[indexLatest].Mismatched(_TuneUpValuesEntered[indexLatest]);
      }

      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Tap_Arrow);
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

      bRepairNeeded = headBroken || liftBroken || treadsBroken;
      _RepairButtonsInstruction.gameObject.SetActive(bRepairNeeded);
      _NoRepairsNeededNotice.gameObject.SetActive(!bRepairNeeded);
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

    private void PlayRobotCalibrationResponseAnim() {
      _RobotResponseIndex = 0;

      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Repair_Calibrate);

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {

        //start with intro anims
        NeedsStateManager nsm = NeedsStateManager.Instance;
        bool severe = nsm.PopLatestEngineValue(NeedId.Repair).Bracket == NeedBracketId.Critical;

        //fix lift height before ready anim if we are going to tun lift
        if (_PartToRepair == RepairablePartId.Lift) {
          if (robot.LiftHeight <= _LiftTooLowToTuneUp_mm) {
            if (severe) {
              robot.SendAnimationTrigger(AnimationTrigger.RepairGetReadySevereRaiseLift, null, QueueActionPosition.NEXT);
            }
            else {
              robot.SendAnimationTrigger(AnimationTrigger.RepairGetReadyMildRaiseLift, null, QueueActionPosition.NEXT);
            }
          }
          else if (robot.LiftHeight >= _LiftTooHighToTuneUp_mm) {
            if (severe) {
              robot.SendAnimationTrigger(AnimationTrigger.RepairGetReadySevereLowerLift, null, QueueActionPosition.NEXT);
            }
            else {
              robot.SendAnimationTrigger(AnimationTrigger.RepairGetReadyMildLowerLift, null, QueueActionPosition.NEXT);
            }
          }
        }


        if (severe) {
          robot.SendAnimationTrigger(AnimationTrigger.RepairGetReadySevere, null, QueueActionPosition.NEXT);
        }
        else {
          robot.SendAnimationTrigger(AnimationTrigger.RepairGetReadyMild, null, QueueActionPosition.NEXT);
        }

        switch (_PartToRepair) {
        case RepairablePartId.Head:
          for (int i = 0; i < _TuneUpPatternToMatch.Count; ++i) {
            if (_TuneUpPatternToMatch[i] == ArrowInput.Up) {
              robot.SendAnimationTrigger(AnimationTrigger.RepairHeadUp, HandleCalibrateAnimMiddle, QueueActionPosition.NEXT);
            }
            else {
              robot.SendAnimationTrigger(AnimationTrigger.RepairHeadDown, HandleCalibrateAnimMiddle, QueueActionPosition.NEXT);
            }
          }
          robot.SetHeadAngle(0.0f, null, QueueActionPosition.NEXT);
          break;
        case RepairablePartId.Lift:
          for (int i = 0; i < _TuneUpPatternToMatch.Count; ++i) {
            if (_TuneUpPatternToMatch[i] == ArrowInput.Up) {
              robot.SendAnimationTrigger(AnimationTrigger.RepairLiftUp, HandleCalibrateAnimMiddle, QueueActionPosition.NEXT);
            }
            else {
              robot.SendAnimationTrigger(AnimationTrigger.RepairLiftDown, HandleCalibrateAnimMiddle, QueueActionPosition.NEXT);
            }
          }
          robot.SetLiftHeight(0.0f, null, QueueActionPosition.NEXT);
          break;
        case RepairablePartId.Treads:
          for (int i = 0; i < _TuneUpPatternToMatch.Count; ++i) {
            if (_TuneUpPatternToMatch[i] == ArrowInput.Up) {
              robot.SendAnimationTrigger(AnimationTrigger.RepairWheelsFront, HandleCalibrateAnimMiddle, QueueActionPosition.NEXT);
            }
            else {
              robot.SendAnimationTrigger(AnimationTrigger.RepairWheelsBack, HandleCalibrateAnimMiddle, QueueActionPosition.NEXT);
            }
          }
          break;
        }

        // Always end with a victory...
        robot.SendAnimationTrigger(AnimationTrigger.SparkSuccess, HandleRobotResponseDone, QueueActionPosition.AT_END);
      }
    }

    // light up the next arrow, note: this may be removed if deemed too distracting by design
    private void HandleCalibrateAnimMiddle(bool success) {
      if (_RobotResponseIndex < _UpDownArrows.Count) {
        _UpDownArrows[_RobotResponseIndex].Calibrated(_TuneUpPatternToMatch[_RobotResponseIndex]);
      }
      _RobotResponseIndex++;
    }

    private void HandleRobotResponseDone(bool success) {
      bRobotResponseDone = true;
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
    #endregion

  }
}