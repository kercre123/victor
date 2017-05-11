using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using System.Collections;
using UnityEngine;

namespace Cozmo.Needs {
  public class NeedsStateManager : MonoBehaviour {

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
      _SetNeedsRandomlyCoroutine = DecayNeedsOverTime();
      StartCoroutine(_SetNeedsRandomlyCoroutine);
    }

    private void OnDisable() {
      StopAllCoroutines();
    }

    // COZMO-10916: IVY TODO: Replace with request to engine for current state after engine is connected
    private void Awake() {
      _LatestStateFromEngine = CreateNewNeedsState();
      _CurrentDisplayState = CreateNewNeedsState();
      BreakPartsIfNeeded();
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
      StartCoroutine(DelayedActionCompleted(actionIdToComplete));
    }

    #region Stub Methods

    private const float _kStubDecayPerSec = 0.01f;
    private bool _ShouldDecay = true;
    private float _CachedDecay = 0f;
    private IEnumerator _SetNeedsRandomlyCoroutine;

    private const float _kStubNeedValueStart = 0.5f;
    private const NeedBracketId _kStubNeedBracketStart = NeedBracketId.Normal;
    private const float _kStubNeedValueBoost = 0.2f;

    private const float _kFakeEngineResponseLag_sec = 0.1f;

    private NeedsState CreateNewNeedsState() {
      NeedsState needsState = new NeedsState();

      int numNeedsId = (int)NeedId.Count;
      needsState.curNeedLevel = new float[numNeedsId];
      needsState.curNeedBracket = new NeedBracketId[numNeedsId];
      for (int i = 0; i < numNeedsId; i++) {
        needsState.curNeedLevel[i] = _kStubNeedValueStart;
        needsState.curNeedBracket[i] = _kStubNeedBracketStart;
      }

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

    // Fake recieving an event over the wire from engine
    private IEnumerator DelayedActionCompleted(NeedsActionId actionIdToComplete) {
      yield return new WaitForSeconds(_kFakeEngineResponseLag_sec);
      // COZMO-10916 IVY TODO: Use real clad messages to send event
      if (actionIdToComplete == NeedsActionId.RepairHead
          || actionIdToComplete == NeedsActionId.RepairLift
          || actionIdToComplete == NeedsActionId.RepairTreads) {
        int partIndex = -1;
        switch (actionIdToComplete) {
        case NeedsActionId.RepairHead:
          partIndex = (int)RepairablePartId.Head;
          break;
        case NeedsActionId.RepairLift:
          partIndex = (int)RepairablePartId.Lift;
          break;
        case NeedsActionId.RepairTreads:
          partIndex = (int)RepairablePartId.Treads;
          break;
        default:
          break;
        }
        _LatestStateFromEngine.partIsDamaged[partIndex] = false;

        ApplyBoost(NeedId.Repair, actionIdToComplete);
      }
      else if (actionIdToComplete == NeedsActionId.FeedRed
               || actionIdToComplete == NeedsActionId.FeedBlue
               || actionIdToComplete == NeedsActionId.FeedGreen) {
        ApplyBoost(NeedId.Energy, actionIdToComplete);
      }
      else if (actionIdToComplete == NeedsActionId.Play) {
        ApplyBoost(NeedId.Play, actionIdToComplete);
      }
    }

    private void ApplyBoost(NeedId needId, NeedsActionId actionId) {
      int index = (int)needId;
      if (needId == NeedId.Repair) {
        AddValue(index, 1f / (float)RepairablePartId.Count, actionId);
      }
      else {
        AddValue(index, _kStubNeedValueBoost, actionId);
      }

      if (OnNeedsLevelChanged != null) {
        OnNeedsLevelChanged(actionId);
      }
    }

    public void PauseDecay() {
      _ShouldDecay = false;
      _CachedDecay = 0f;
    }

    public void ResumeDecay() {
      _ShouldDecay = true;
      ApplyDecay(_CachedDecay);
    }

    // COZMO-10916 IVY TODO: Use real clad messages to change values
    private IEnumerator DecayNeedsOverTime() {
      while (true) {
        if (_ShouldDecay) {
          ApplyDecay(_kStubDecayPerSec);
        }
        else {
          _CachedDecay += _kStubDecayPerSec;
        }

        yield return new WaitForSeconds(1f);
      }
    }

    private void ApplyDecay(float decayToApply) {
      int numNeedsId = (int)NeedId.Count;
      for (int i = 0; i < numNeedsId; i++) {
        AddValue(i, -decayToApply, NeedsActionId.Decay);

        if (i == (int)NeedId.Repair) {
          BreakPartsIfNeeded();
        }
      }

      if (OnNeedsLevelChanged != null) {
        OnNeedsLevelChanged(NeedsActionId.Decay);
      }
    }

    private void AddValue(int needIndex, float delta, NeedsActionId actionId) {
      _LatestStateFromEngine.curNeedLevel[needIndex] += delta;
      _LatestStateFromEngine.curNeedLevel[needIndex] = Mathf.Clamp(_LatestStateFromEngine.curNeedLevel[needIndex],
                                                                   0f, 1f);
      int numBrackets = (int)NeedBracketId.Count;
      float pointsPerLevel = 1f / numBrackets;
      for (int i = 1; i <= numBrackets; i++) {
        if (_LatestStateFromEngine.curNeedLevel[needIndex] < pointsPerLevel * i) {
          NeedBracketId oldBracket = _LatestStateFromEngine.curNeedBracket[needIndex];
          NeedBracketId newBracket = (NeedBracketId)(numBrackets - i);
          if (oldBracket != newBracket) {
            _LatestStateFromEngine.curNeedBracket[needIndex] = newBracket;
            if (OnNeedsBracketChanged != null) {
              OnNeedsBracketChanged(actionId, (NeedId)needIndex);
            }
          }
          break;
        }
      }
    }

    private void BreakPartsIfNeeded() {
      int numParts = (int)RepairablePartId.Count;
      float pointsPerPart = 1f / numParts;
      int numPartsBroken = numParts - Mathf.CeilToInt(_LatestStateFromEngine.curNeedLevel[(int)NeedId.Repair] / pointsPerPart);
      int currentNumPartsBroken = 0;
      for (int i = 0; i < numParts; i++) {
        if (_LatestStateFromEngine.partIsDamaged[i]) {
          currentNumPartsBroken++;
        }
      }

      if (currentNumPartsBroken < numPartsBroken) {
        int numPartsToBreak = numPartsBroken - currentNumPartsBroken;
        int partIndex = 0;
        while (numPartsToBreak > 0 && partIndex < numParts) {
          if (!_LatestStateFromEngine.partIsDamaged[partIndex]) {
            _LatestStateFromEngine.partIsDamaged[partIndex] = true;
            numPartsToBreak--;
          }
          partIndex++;
        }
      }
    }
    #endregion
  }
}