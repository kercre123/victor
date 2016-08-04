using System;
using UnityEngine;
using Anki.Cozmo;
using Cozmo.HomeHub;
using DataPersistence;
using Onboarding;

public class OnboardingManager : MonoBehaviour {

  public static OnboardingManager Instance { get; private set; }

  private Cozmo.HomeHub.HomeView _HomeView;

  // Each phase is made up of several stages,
  // This can be used to checkpoint them, and is the total screens
  public enum OnboardingPhases : int {
    First = 4,
    Second,
    Third,
    Count
  };

  // Must be in order of enums above
  [SerializeField]
  private OnboardingBaseStage[] _HomeOnboardingStages;
  private GameObject _CurrStageInst = null;
  private int _CurrStage = -1;

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
      // Resets every time
      //DataPersistenceManager.Instance.Data.DefaultProfile.OnboardingHomeStage = 0;
      Instance = this;
    }
  }

  public bool IsOnboardingRequiredHome() {
    return DataPersistenceManager.Instance.Data.DefaultProfile.OnboardingHomeStage < _HomeOnboardingStages.Length;
  }

  public void InitHomeHubOnboarding(Cozmo.HomeHub.HomeView homeview) {
    _HomeView = homeview;
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Onboarding);
    // TODO: Get more design on where "checkpoints" are.
    SetSpecificStage(0);
  }

  private void ExitHomeHubOnboarding() {
    // Exit back into a normal view...
    if (_DebugLayer != null) {
      GameObject.Destroy(_DebugLayer);
    }

    HomeHub.Instance.StartFreeplay(RobotEngineManager.Instance.CurrentRobot);
    UpdateStage();
  }

  public void GoToNextStage() {
    SetSpecificStage(_CurrStage + 1);
  }

  public void SetSpecificStage(int nextStage) {

    if (_CurrStageInst != null) {
      GameObject.Destroy(_CurrStageInst);
    }
    // Create the debug layer to have a few buttons to work with on screen easily for QA
    // who will have to see this all the time.
#if ENABLE_DEBUG_PANEL
    if (_DebugLayer == null && _DebugLayerPrefab != null && _HomeView != null) {
      _DebugLayer = UIManager.CreateUIElement(_DebugLayerPrefab, _HomeView.transform);
    }
#endif
    _CurrStage = nextStage;
    DataPersistenceManager.Instance.Data.DefaultProfile.OnboardingHomeStage = _CurrStage;
    if (_CurrStage >= 0 && _CurrStage < _HomeOnboardingStages.Length) {
      _CurrStageInst = UIManager.CreateUIElement(_HomeOnboardingStages[_CurrStage], _HomeView.transform);
      UpdateStage(_HomeOnboardingStages[_CurrStage].ActiveTopBar,
                   _HomeOnboardingStages[_CurrStage].ActiveMenuContent,
                   _HomeOnboardingStages[_CurrStage].ActiveTabButtons,
                   _HomeOnboardingStages[_CurrStage].ReactionsEnabled);

      // TODO: when reskin done, put in a gameObject here. It's basically below the energy but above the content.
      _CurrStageInst.transform.SetSiblingIndex(_HomeView.transform.childCount - 4);
    }
    else {
      ExitHomeHubOnboarding();
    }

    DataPersistenceManager.Instance.Save();
  }
  public void GiveEnergy(int energy) {
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount("experience", energy);
    _HomeView.EnergyDooberBurst(energy);
  }

  public HomeView GetHomeView() {
    return _HomeView;
  }

  #region DEBUG
  public void DebugSkipOne() {
    OnboardingManager.Instance.SkipPressed();
  }

  public void DebugSkipAll() {
    OnboardingManager.Instance.SetSpecificStage(_HomeOnboardingStages.Length);
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