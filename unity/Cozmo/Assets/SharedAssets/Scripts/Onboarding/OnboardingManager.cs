using System;
using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo;
using Cozmo.HomeHub;
using DataPersistence;
using Onboarding;

public class OnboardingManager : MonoBehaviour {

  public static OnboardingManager Instance { get; private set; }

  private HomeView _HomeView;

  public enum OnboardingPhases : int {
    Home,
    Loot,
    Upgrades,
    None
  };

  // Since the phases are all done in getters anyways, for clarity edit individual names
  // in the editor rather than a dictionary...
  [SerializeField]
  private List<OnboardingBaseStage> _PhaseHomePrefabs;
  [SerializeField]
  private List<OnboardingBaseStage> _PhaseLootPrefabs;
  [SerializeField]
  private List<OnboardingBaseStage> _PhaseUpgradesPrefabs;

  private OnboardingPhases _CurrPhase = OnboardingPhases.None;

  // Must be in order of enums above
  [SerializeField]
  private OnboardingBaseStage[] _HomeOnboardingStages;
  private GameObject _CurrStageInst = null;

  private Transform _OnboardingTransform;

  [SerializeField]
  private GameObject _DebugLayerPrefab;
  private GameObject _DebugLayer;

  private void OnEnable() {
    if (Instance != null && Instance != this) {
      SetSpecificStage(-1);
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
      Anki.Debug.DebugConsoleData.Instance.AddConsoleFunction("Fill Loot Energy", "Unity", GiveEnergy);
    }
  }

  private int GetCurrStageInPhase(OnboardingPhases phase) {
    if (phase == OnboardingPhases.None) {
      return 0;
    }

    PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
    if (!profile.OnboardingStages.ContainsKey(phase)) {
      profile.OnboardingStages[phase] = 0;
    }
    return profile.OnboardingStages[phase];
  }
  private void SetCurrStageInPhase(int stage) {
    if (_CurrPhase != OnboardingPhases.None) {
      PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
      profile.OnboardingStages[_CurrPhase] = stage;
    }
  }

  private int GetMaxStageInPhase(OnboardingPhases phase) {
    switch (phase) {
    case OnboardingPhases.Home:
      return _PhaseHomePrefabs.Count;
    case OnboardingPhases.Loot:
      return _PhaseLootPrefabs.Count;
    case OnboardingPhases.Upgrades:
      return _PhaseUpgradesPrefabs.Count;
    }
    return 0;
  }
  private OnboardingBaseStage GetCurrStagePrefab() {
    int currStage = GetCurrStageInPhase(_CurrPhase);
    switch (_CurrPhase) {
    case OnboardingPhases.Home:
      return _PhaseHomePrefabs[currStage];
    case OnboardingPhases.Loot:
      return _PhaseLootPrefabs[currStage];
    case OnboardingPhases.Upgrades:
      return _PhaseUpgradesPrefabs[currStage];
    }
    return null;
  }

  public bool IsOnboardingRequired(OnboardingPhases phase) {
    return GetCurrStageInPhase(phase) < GetMaxStageInPhase(phase);
  }

  public void InitHomeHubOnboarding(HomeView homeview, Transform onboardingLayer) {
    _HomeView = homeview;
    _OnboardingTransform = onboardingLayer;

    if (IsOnboardingRequired(OnboardingPhases.Home)) {
      StartPhase(OnboardingPhases.Home);
    }
  }

  public void StartPhase(OnboardingPhases phase) {
    // End any previous phase, can only highlight one thing at once
    if (_CurrPhase != OnboardingPhases.None) {
      SetSpecificStage(GetMaxStageInPhase(_CurrPhase));
    }
    _CurrPhase = phase;
    SetSpecificStage(0);
  }

  public void GoToNextStage() {
    SetSpecificStage(GetCurrStageInPhase(_CurrPhase) + 1);
  }

  public void SetSpecificStage(int nextStage) {
    if (_CurrStageInst != null) {
      GameObject.Destroy(_CurrStageInst);
    }
    // Create the debug layer to have a few buttons to work with on screen easily for QA
    // who will have to see this all the time.
#if ENABLE_DEBUG_PANEL
    if ( _DebugLayer == null && _DebugLayerPrefab != null ) {
      _DebugLayer = UIManager.CreateUIElement(_DebugLayerPrefab, DebugMenuManager.Instance.DebugOverlayCanvas.transform);
    }
#endif
    SetCurrStageInPhase(nextStage);
    if (nextStage >= 0 && nextStage < GetMaxStageInPhase(_CurrPhase)) {
      OnboardingBaseStage stagePrefab = GetCurrStagePrefab();

      _CurrStageInst = UIManager.CreateUIElement(stagePrefab, _OnboardingTransform);
      UpdateStage(stagePrefab.ActiveTopBar, stagePrefab.ActiveMenuContent,
                  stagePrefab.ActiveTabButtons, stagePrefab.ReactionsEnabled);
    }
    else {
      // Officially done with this phase
      if (_DebugLayer != null) {
        GameObject.Destroy(_DebugLayer);
      }
      _CurrPhase = OnboardingPhases.None;
      UpdateStage();
      HomeHub.Instance.StartFreeplay(RobotEngineManager.Instance.CurrentRobot);
    }

    DataPersistenceManager.Instance.Save();
  }

  public HomeView GetHomeView() {
    return _HomeView;
  }

  #region DEBUG

  public void GiveEnergy(string param) {
    int energy = ChestRewardManager.Instance.GetNextRequirementPoints();
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount("experience", energy);
  }

  public void DebugSkipOne() {
    OnboardingManager.Instance.SkipPressed();
  }

  public void DebugSkipAll() {
    OnboardingManager.Instance.SetSpecificStage(GetMaxStageInPhase(Instance._CurrPhase));
  }
  private void SkipPressed() {
    if (_CurrStageInst != null) {
      OnboardingBaseStage stage = _CurrStageInst.GetComponent<OnboardingBaseStage>();
      if (stage) {
        stage.SkipPressed();
      }
    }
  }
  #endregion

  private void UpdateStage(bool showTopBar = true, bool showContent = true, bool showButtons = true, bool reactionsEnabled = true) {
    if (_HomeView) {
      _HomeView.TopBarContainer.gameObject.SetActive(showTopBar);
      _HomeView.TabContentContainer.gameObject.SetActive(showContent);
      _HomeView.TabButtonContainer.gameObject.SetActive(showButtons);
    }
    RobotEngineManager.Instance.CurrentRobot.EnableReactionaryBehaviors(reactionsEnabled);
  }

}