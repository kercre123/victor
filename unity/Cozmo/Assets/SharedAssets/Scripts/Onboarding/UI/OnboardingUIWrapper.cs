using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;

namespace Onboarding {
  // Just a wrapper for abstracting the assetBundle bits of OnboardingManager
  public class OnboardingUIWrapper : MonoBehaviour {

    [SerializeField]
    private GameObject _DebugLayerPrefab;
    private GameObject _DebugLayer;

    [SerializeField]
    private List<OnboardingBaseStage> _PhaseHomePrefabs;
    [SerializeField]
    private List<OnboardingBaseStage> _PhaseLootPrefabs;
    [SerializeField]
    private List<OnboardingBaseStage> _PhaseUpgradesPrefabs;

    public void AddDebugButtons() {
      if (_DebugLayer == null && _DebugLayerPrefab != null) {
        _DebugLayer = UIManager.CreateUIElement(_DebugLayerPrefab, DebugMenuManager.Instance.DebugOverlayCanvas.transform);
      }
    }

    public void RemoveDebugButtons() {
      // Officially done with this phase
      if (_DebugLayer != null) {
        GameObject.Destroy(_DebugLayer);
      }
    }

    public int GetMaxStageInPhase(OnboardingManager.OnboardingPhases phase) {
      switch (phase) {
      case OnboardingManager.OnboardingPhases.Home:
        return _PhaseHomePrefabs.Count;
      case OnboardingManager.OnboardingPhases.Loot:
        return _PhaseLootPrefabs.Count;
      case OnboardingManager.OnboardingPhases.Upgrades:
        return _PhaseUpgradesPrefabs.Count;
      // Daily goals is a special case where it is just used to save state.co
      case OnboardingManager.OnboardingPhases.DailyGoals:
        return 1;
      }
      return 0;
    }
    public OnboardingBaseStage GetCurrStagePrefab(int currStage, OnboardingManager.OnboardingPhases phase) {
      switch (phase) {
      case OnboardingManager.OnboardingPhases.Home:
        return _PhaseHomePrefabs[currStage];
      case OnboardingManager.OnboardingPhases.Loot:
        return _PhaseLootPrefabs[currStage];
      case OnboardingManager.OnboardingPhases.Upgrades:
        return _PhaseUpgradesPrefabs[currStage];
      }
      return null;
    }

  }

}
