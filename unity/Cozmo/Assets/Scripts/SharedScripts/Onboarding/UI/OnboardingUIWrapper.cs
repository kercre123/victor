using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;

namespace Onboarding {

  // Just forcing a list of Lists to be drawn without making a custom drawer.
  [System.Serializable]
  public class OnboardingPhasePrefabList {
    public List<OnboardingBaseStage> onboardingPrefab;
  }

  // Just a wrapper for abstracting the assetBundle bits of OnboardingManager
  public class OnboardingUIWrapper : MonoBehaviour {

    [SerializeField]
    private GameObject _DebugLayerPrefab;
    private GameObject _DebugLayer;

    [SerializeField]
    private List<OnboardingPhasePrefabList> _NurtureStagesPrefabs;

    public void AddDebugButtons() {
      if (_DebugLayer == null && _DebugLayerPrefab != null) {
        _DebugLayer = UIManager.CreateUIElement(_DebugLayerPrefab, DebugMenuManager.Instance.DebugOverlayCanvas.transform);
      }
    }

    public void RemoveDebugButtons() {
      // Officially done with this phase
      if (_DebugLayer != null) {
        GameObject.Destroy(_DebugLayer);
        _DebugLayer = null;
      }
    }

    public int GetMaxStageInPhase(OnboardingManager.OnboardingPhases phase) {
      // UI-less phases, binary for completed or not.
      if (phase == OnboardingManager.OnboardingPhases.GameRequests) {
        return 1;
      }
      // Old phases, no need to know exact length anymore
      if ((int)phase >= _NurtureStagesPrefabs.Count) {
        return 0;
      }
      return _NurtureStagesPrefabs[(int)phase].onboardingPrefab.Count;
    }
    public OnboardingBaseStage GetCurrStagePrefab(int currStage, OnboardingManager.OnboardingPhases phase) {
      switch (phase) {
      case OnboardingManager.OnboardingPhases.Home:
      case OnboardingManager.OnboardingPhases.Loot:
      case OnboardingManager.OnboardingPhases.Upgrades:
      case OnboardingManager.OnboardingPhases.None:
      case OnboardingManager.OnboardingPhases.DailyGoals:
        return null;
      }
      return _NurtureStagesPrefabs[(int)phase].onboardingPrefab[currStage];
    }

  }

}
