using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using System.Collections;
using UnityEngine;

namespace Cozmo.Needs {
  public class NeedsStateManager : MonoBehaviour {

    public delegate void LatestNeedLevelChangedHandler();
    public event LatestNeedLevelChangedHandler OnNeedsLevelChanged;

    public static NeedsStateManager Instance { get; private set; }

    private IEnumerator _SetNeedsRandomlyCoroutine;

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
      _SetNeedsRandomlyCoroutine = TestSetNeedsRandomly();
      StartCoroutine(_SetNeedsRandomlyCoroutine);
    }

    private void OnDisable() {
      StopCoroutine(_SetNeedsRandomlyCoroutine);
    }

    private void Awake() {
      _LatestStateFromEngine = CreateNewNeedsState();
      _CurrentDisplayState = CreateNewNeedsState();
    }

    private NeedsState CreateNewNeedsState() {
      NeedsState needsState = new NeedsState();

      int numNeedsId = (int)NeedId.Count;
      needsState.curNeedLevel = new float[numNeedsId];
      needsState.curNeedBracket = new NeedBracketId[numNeedsId];
      for (int i = 0; i < numNeedsId; i++) {
        needsState.curNeedLevel[i] = 0f;

        // IVY TODO: Just stubbing for now
        needsState.curNeedBracket[i] = NeedBracketId.Critical;
      }

      int numParts = (int)RepairablePartId.Count;
      needsState.partIsDamaged = new bool[numParts];
      for (int i = 0; i < numParts; i++) {
        // IVY TODO: Just stubbing for now
        needsState.partIsDamaged[i] = true;
      }

      // IVY TODO: Not used for now
      needsState.curNeedsUnlockLevel = 0;
      needsState.numStarsAwarded = 0;
      needsState.numStarsForNextUnlock = 0;

      return needsState;
    }

    public float GetCurrentDisplayValue(NeedId needId) {
      return _CurrentDisplayState.curNeedLevel[(int)needId];
    }

    public float PopLatestEngineValue(NeedId needId) {
      float latestValue = _LatestStateFromEngine.curNeedLevel[(int)needId];
      _CurrentDisplayState.curNeedLevel[(int)needId] = latestValue;
      return latestValue;
    }

    public float PeekLatestEngineValue(NeedId needId) {
      return _LatestStateFromEngine.curNeedLevel[(int)needId];
    }

    // IVY TODO: Use real clad messages to change values
    private IEnumerator TestSetNeedsRandomly() {
      while (true) {
        int numNeedsId = (int)NeedId.Count;
        for (int i = 0; i < numNeedsId; i++) {
          _LatestStateFromEngine.curNeedLevel[i] = Random.Range(0f, 1f);
        }

        if (OnNeedsLevelChanged != null) {
          OnNeedsLevelChanged();
        }

        yield return new WaitForSeconds(5f);
      }
    }
  }
}