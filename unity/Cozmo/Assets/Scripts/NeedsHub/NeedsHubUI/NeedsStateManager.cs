using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.Needs {
  public class NeedsStateManager : MonoBehaviour {

    [System.Serializable]
    public class SerializableNeedsActionIds : SerializableEnum<Anki.Cozmo.NeedsActionId> {
      public SerializableNeedsActionIds() {
        Value = NeedsActionId.Count;
      }

      public SerializableNeedsActionIds(NeedsActionId defaultVale) {
        Value = defaultVale;
      }
    }

    // Will notify delegates of any action whether the action resulted in a need change or not
    public delegate void LatestNeedActionRecieved(NeedsActionId actionReceived);
    public event LatestNeedActionRecieved OnNeedsActionReceived;

    // Only notifies delegate if the action resulted in a needs level change
    public delegate void LatestNeedLevelChangedHandler(NeedsActionId actionThatCausedChange);
    public event LatestNeedLevelChangedHandler OnNeedsLevelChanged;

    public delegate void LatestNeedBracketChangedHandler(NeedsActionId actionThatCausedChange, NeedId needThatChanged);
    public event LatestNeedBracketChangedHandler OnNeedsBracketChanged;

    public static NeedsStateManager Instance { get; private set; }

    private NeedsState _LatestStateFromEngine;
#if UNITY_EDITOR
    public NeedsState LatestStateFromEngine {
      get {
        if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
          return _LatestStateFromEngine;
        }
        return null;
      }
    }
#endif

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
      RobotEngineManager.Instance.AddCallback<StarLevelCompleted>(HandleStarLevelCompleted);
      RobotEngineManager.Instance.AddCallback<StarUnlocked>(HandleStarUnlocked);
      _LatestStateFromEngine = CreateNewNeedsState();
      _CurrentDisplayState = CreateNewNeedsState();

      // Request needs state from engine ASAP, so we can display the needs levels
      RequestNeedsState();
    }

    private void OnDestroy() {
      if (RobotEngineManager.Instance != null) {
        RobotEngineManager.Instance.RemoveCallback<NeedsState>(HandleNeedsStateFromEngine);
        RobotEngineManager.Instance.RemoveCallback<StarLevelCompleted>(HandleStarLevelCompleted);
        RobotEngineManager.Instance.RemoveCallback<StarUnlocked>(HandleStarUnlocked);
      }
    }

    private void RequestNeedsState() {
      RobotEngineManager.Instance.Message.GetNeedsState = new GetNeedsState();
      RobotEngineManager.Instance.SendMessage();
    }

    public NeedsValue GetCurrentDisplayValue(NeedId needId) {
      int needIndex = (int)needId;
      NeedsValue currentValue;
      currentValue.Value = _CurrentDisplayState.curNeedLevel[needIndex];
      currentValue.Bracket = _CurrentDisplayState.curNeedBracket[needIndex];
      return currentValue;
    }

    public NeedsValue PopLatestEngineValue(NeedId needId) {
      int needIndex = (int)needId;
      NeedsValue latestValue;
      latestValue.Value = _LatestStateFromEngine.curNeedLevel[needIndex];
      latestValue.Bracket = _LatestStateFromEngine.curNeedBracket[needIndex];
      _CurrentDisplayState.curNeedLevel[needIndex] = latestValue.Value;
      _CurrentDisplayState.curNeedBracket[needIndex] = latestValue.Bracket;
      return latestValue;
    }

    public NeedsValue PeekLatestEngineValue(NeedId needId) {
      int needIndex = (int)needId;
      NeedsValue latestValue;
      latestValue.Value = _LatestStateFromEngine.curNeedLevel[needIndex];
      latestValue.Bracket = _LatestStateFromEngine.curNeedBracket[needIndex];
      return latestValue;
    }

    public bool PeekLatestEnginePartIsBroken(RepairablePartId partId) {
      int partIndex = (int)partId;
      bool latestValue = _LatestStateFromEngine.partIsDamaged[partIndex];
      return latestValue;
    }

    public bool PopLatestEnginePartIsBroken(RepairablePartId partId) {
      int partIndex = (int)partId;
      bool latestValue = _LatestStateFromEngine.partIsDamaged[partIndex];
      _CurrentDisplayState.partIsDamaged[partIndex] = latestValue;
      return latestValue;
    }

    public int GetLatestStarAwardedFromEngine() {
      return _LatestStateFromEngine.numStarsAwarded;
    }

    public int GetLatestStarForNextUnlockFromEngine() {
      return _LatestStateFromEngine.numStarsForNextUnlock;
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
      // Pause everything ( onboarding )
      if (needId != NeedId.Count) {
        pauseStatesMessage.actionPause[(int)needId] = false;
      }
      RobotEngineManager.Instance.Message.SetNeedsPauseStates = pauseStatesMessage;
      RobotEngineManager.Instance.SendMessage();
    }

    // This will throw out any actions during state
    public void SetFullPause(bool fullPause) {
      SetNeedsPauseState pauseStateMessage = new SetNeedsPauseState();
      pauseStateMessage.pause = fullPause;
      RobotEngineManager.Instance.Message.SetNeedsPauseState = pauseStateMessage;
      RobotEngineManager.Instance.SendMessage();
    }

    public void ResumeAllNeeds() {
      SetNeedsPauseStates pauseStatesMessage = new SetNeedsPauseStates();
      pauseStatesMessage.decayPause = new bool[] { false, false, false };
      pauseStatesMessage.actionPause = new bool[] { false, false, false };
      pauseStatesMessage.decayDiscardAfterUnpause = new bool[] { true, true, true };
      RobotEngineManager.Instance.Message.SetNeedsPauseStates = pauseStatesMessage;
      RobotEngineManager.Instance.SendMessage();
    }

    public int GetLatestRepairRounds() {
      return _LatestStateFromEngine.repairRounds;
    }

#if UNITY_EDITOR
    public void MockHandleNeedsStateFromEngine(NeedsState newNeedsState) {
      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        StartCoroutine(DelayedMockHandleNeedsStateFromEngine(newNeedsState));
      }
    }

    private System.Collections.IEnumerator DelayedMockHandleNeedsStateFromEngine(NeedsState newNeedsState) {
      yield return new WaitForSeconds(2f);
      HandleNeedsStateFromEngine(newNeedsState);
    }
#endif

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

      if (OnNeedsActionReceived != null) {
        OnNeedsActionReceived(newNeedsState.actionCausingTheUpdate);
      }

      if (needsValueChanged && OnNeedsLevelChanged != null) {
        OnNeedsLevelChanged(newNeedsState.actionCausingTheUpdate);
      }

      if (needsBracketChanged && OnNeedsBracketChanged != null) {
        foreach (int need in needsThatChanged) {
          OnNeedsBracketChanged(newNeedsState.actionCausingTheUpdate, (NeedId)need);
        }
      }
    }

    private void HandleStarLevelCompleted(StarLevelCompleted message) {
      DataPersistence.DataPersistenceManager.Instance.AddNewStarLevel(message);
    }

    private void HandleStarUnlocked(StarUnlocked message) {
      _LatestStateFromEngine.numStarsAwarded = message.currentStars;
    }

    private NeedsState CreateNewNeedsState() {
      NeedsState needsState = new NeedsState();

      int numNeedsId = (int)NeedId.Count;
      needsState.curNeedLevel = new float[numNeedsId];
      needsState.curNeedBracket = new NeedBracketId[numNeedsId];
      for (int i = 0; i < numNeedsId; i++) {
        needsState.curNeedLevel[i] = 0f;
        needsState.curNeedBracket[i] = NeedBracketId.Critical;
      }

      // Don't let them repair until they can fix on a connected robot
      int numParts = System.Enum.GetNames(typeof(RepairablePartId)).Length;
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

  public struct NeedsValue {
    public float Value;
    public NeedBracketId Bracket;
  }
}