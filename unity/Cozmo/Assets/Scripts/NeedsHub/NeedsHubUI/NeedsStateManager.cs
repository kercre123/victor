using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.Needs {
  public class NeedsStateManager : MonoBehaviour {

    [System.Serializable]
    public class SerializableNeedsActionIds : SerializableEnum<Anki.Cozmo.NeedsActionId> {
    }

    public delegate void LatestNeedLevelChangedHandler(NeedsActionId actionThatCausedChange);
    public event LatestNeedLevelChangedHandler OnNeedsLevelChanged;

    public delegate void LatestNeedBracketChangedHandler(NeedsActionId actionThatCausedChange, NeedId needThatChanged);
    public event LatestNeedBracketChangedHandler OnNeedsBracketChanged;

    public static NeedsStateManager Instance { get; private set; }

    private NeedsState _LatestStateFromEngine;

    private NeedsState _CurrentDisplayState;

    private void OnEnable() {
      if (Instance != null && Instance != this) {
        Destroy(gameObject);
        return;
      }
      else {
        Instance = this;
      }
    }

    private void OnDisable() {
      StopAllCoroutines();
    }

    private void Awake() {
      RobotEngineManager.Instance.ConnectedToClient += (s) => RequestNeedsState();
      RobotEngineManager.Instance.AddCallback<NeedsState>(HandleNeedsStateFromEngine);
      _LatestStateFromEngine = CreateNewNeedsState();
      _CurrentDisplayState = CreateNewNeedsState();
    }

    private void OnDestroy() {
      if (RobotEngineManager.Instance != null) {
        RobotEngineManager.Instance.RemoveCallback<NeedsState>(HandleNeedsStateFromEngine);
      }
    }

    private void RequestNeedsState() {
      RobotEngineManager.Instance.Message.GetNeedsState = new GetNeedsState();
      RobotEngineManager.Instance.SendMessage();
    }

    public float GetCurrentDisplayValue(NeedId needId) {
      int needIndex = (int)needId;
      return _CurrentDisplayState.curNeedLevel[needIndex];
    }

    public float PopLatestEngineValue(NeedId needId) {
      int needIndex = (int)needId;
      float latestValue = _LatestStateFromEngine.curNeedLevel[needIndex];
      _CurrentDisplayState.curNeedLevel[needIndex] = latestValue;
      return latestValue;
    }

    public NeedBracketId GetCurrentDisplayBracket(NeedId needId) {
      int needIndex = (int)needId;
      return _CurrentDisplayState.curNeedBracket[needIndex];
    }

    public NeedBracketId PopLatestEngineBracket(NeedId needId) {
      int needIndex = (int)needId;
      NeedBracketId latestValue = _LatestStateFromEngine.curNeedBracket[needIndex];
      _CurrentDisplayState.curNeedBracket[needIndex] = latestValue;
      return latestValue;
    }

    public bool PopLatestEnginePartIsBroken(RepairablePartId partId) {
      int partIndex = (int)partId;
      bool latestValue = _LatestStateFromEngine.partIsDamaged[partIndex];
      _CurrentDisplayState.partIsDamaged[partIndex] = latestValue;
      return latestValue;
    }

    public void RegisterNeedActionCompleted(NeedsActionId actionIdToComplete) {
      RobotEngineManager.Instance.Message.RegisterNeedsActionCompleted =
                          Singleton<RegisterNeedsActionCompleted>.Instance.Initialize(actionIdToComplete);
      RobotEngineManager.Instance.SendMessage();
    }

    public void PauseExceptForNeed(NeedId needId) {
      SetNeedsPauseStates pauseStatesMessage = new SetNeedsPauseStates();
      pauseStatesMessage.decayPause = new bool[] { true, true, true };
      pauseStatesMessage.actionPause = new bool[] { true, true, true };
      pauseStatesMessage.actionPause[(int)needId] = false;
      RobotEngineManager.Instance.Message.SetNeedsPauseStates = pauseStatesMessage;
      RobotEngineManager.Instance.SendMessage();
    }

    public void ResumeAllNeeds() {
      SetNeedsPauseStates pauseStatesMessage = new SetNeedsPauseStates();
      pauseStatesMessage.decayPause = new bool[] { false, false, false };
      pauseStatesMessage.actionPause = new bool[] { false, false, false };
      pauseStatesMessage.decayDiscardAfterUnpause = new bool[] { false, false, false };
      RobotEngineManager.Instance.Message.SetNeedsPauseStates = pauseStatesMessage;
      RobotEngineManager.Instance.SendMessage();
    }

    private void HandleNeedsStateFromEngine(NeedsState newNeedsState) {
      bool needsValueChanged = false;
      for (int needIndex = 0; needIndex < (int)NeedId.Count; needIndex++) {
        if (!_LatestStateFromEngine.curNeedLevel[needIndex].IsNear(newNeedsState.curNeedLevel[needIndex], float.Epsilon)) {
          needsValueChanged = true;
          break;
        }
      }

      bool needsBracketChanged = false;
      List<int> needsThatChanged = new List<int>();
      for (int needIndex = 0; needIndex < (int)NeedId.Count; needIndex++) {
        if (_LatestStateFromEngine.curNeedBracket[needIndex] != newNeedsState.curNeedBracket[needIndex]) {
          needsBracketChanged = true;
          needsThatChanged.Add(needIndex);
        }
      }

      _LatestStateFromEngine = newNeedsState;

      if (needsValueChanged && OnNeedsLevelChanged != null) {
        OnNeedsLevelChanged(newNeedsState.actionCausingTheUpdate);
      }

      if (needsBracketChanged && OnNeedsBracketChanged != null) {
        foreach (int need in needsThatChanged) {
          OnNeedsBracketChanged(newNeedsState.actionCausingTheUpdate, (NeedId)need);
        }
      }
    }

    private NeedsState CreateNewNeedsState() {
      NeedsState needsState = new NeedsState();

      int numNeedsId = (int)NeedId.Count;
      needsState.curNeedLevel = new float[numNeedsId];
      needsState.curNeedBracket = new NeedBracketId[numNeedsId];
      for (int i = 0; i < numNeedsId; i++) {
        needsState.curNeedLevel[i] = 100f;
        needsState.curNeedBracket[i] = NeedBracketId.Full;
      }

      // Don't let them repair until they can fix on a connected robot
      int numParts = (int)RepairablePartId.Count;
      needsState.partIsDamaged = new bool[numParts];
      for (int i = 0; i < numParts; i++) {
        needsState.partIsDamaged[i] = false;
      }

      needsState.curNeedsUnlockLevel = 0;
      needsState.numStarsAwarded = 0;
      needsState.numStarsForNextUnlock = 0;

      return needsState;
    }
  }
}