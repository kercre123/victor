using Cozmo.Challenge;
using UnityEngine;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    [SerializeField]
    private FaceEnrollmentInstructionsModal _FaceEnrollmentInstructionsModalPrefab;
    public FaceEnrollmentInstructionsModal FaceEnrollmentInstructionsModalPrefab {
      get { return _FaceEnrollmentInstructionsModalPrefab; }
    }

    [SerializeField]
    private GameObject _FaceEnrollmentMultipleFacesErrorSlidePrefab;
    public GameObject FaceEnrollmentMultipleFacesErrorSlidePrefab {
      get { return _FaceEnrollmentMultipleFacesErrorSlidePrefab; }
    }

    [SerializeField]
    private EnterNameSlide _EnterNameSlidePrefab;
    public EnterNameSlide EnterNameSlidePrefab {
      get { return _EnterNameSlidePrefab; }
    }

    [SerializeField]
    private FaceEnrollmentListSlide _FaceListSlidePrefab;
    public FaceEnrollmentListSlide FaceListSlidePrefab {
      get { return _FaceListSlidePrefab; }
    }

    [SerializeField]
    private FaceEnrollmentDetailsSlide _FaceEnrollmentDetailsSlidePrefab;
    public FaceEnrollmentDetailsSlide FaceEnrollmentDetailsSlidePrefab {
      get { return _FaceEnrollmentDetailsSlidePrefab; }
    }

    [SerializeField]
    private FaceSlidesShelfContent _FaceEnrollmentShelfContentPrefab;
    private FaceSlidesShelfContent _FaceEnrollmentShelfContentInstance;

    [SerializeField]
    private FaceDetailsShelfContent _FaceDetailsShelfContentPrefab;
    private FaceDetailsShelfContent _FaceDetailsShelfContentInstance;

    [SerializeField]
    private FaceMultipleFacesErrorShelfContent _FaceMultipleFacesErrorShelfContentPrefab;
    private FaceMultipleFacesErrorShelfContent _FaceMultipleFacesErrorShelfContentInstance;

    private long _UpdateThresholdLastSeenSeconds;
    public long UpdateThresholdLastSeenSeconds {
      get { return _UpdateThresholdLastSeenSeconds; }
    }

    private long _UpdateThresholdLastEnrolledSeconds;
    public long UpdateThresholdLastEnrolledSeconds {
      get { return _UpdateThresholdLastEnrolledSeconds; }
    }

    private bool _ShowDoneShelf = false;

    public bool ShowDoneShelf {
      get { return _ShowDoneShelf; }
      set { _ShowDoneShelf = value; }
    }

    public void Awake() {
      OnSharedMinigameViewInitialized += HandleSharedMinigameViewInitialized;
      OnboardingManager.Instance.OnOnboardingPhaseCompleted += HandleOnboardingPhaseComplete;
      Cozmo.PauseManager.Instance.OnCozmoSleepCancelled += HandleCozmoSleepCancelled;
    }

    protected override void InitializeGame(ChallengeConfigBase challengeConfigData) {

      FaceEnrollmentGameConfig faceEnrollmentConfig = (FaceEnrollmentGameConfig)challengeConfigData;

      _UpdateThresholdLastSeenSeconds = faceEnrollmentConfig.UpdateThresholdLastSeenSeconds;
      _UpdateThresholdLastEnrolledSeconds = faceEnrollmentConfig.UpdateThresholdLastEnrolledSeconds;

      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.MeetCozmo)) {
        if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
          SharedMinigameView.HideQuitButton();
        }
        // Onboarding trains people to look here so just let players hit done and move on.
        ShowDoneShelf = true;
      }
    }

    protected override void InitializeReactionaryBehaviorsForGameStart() {
      RobotEngineManager.Instance.CurrentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kMinigameId, ReactionaryBehaviorEnableGroups.kFaceEnrollmentTriggers);
    }

    protected override string GetLoadingTextKey() {
      return LocalizationKeys.kFaceEnrollmentLabelCozmoPrep;
    }

    protected override void SetupViewAfterCozmoReady(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.SetupViewAfterCozmoReady(newView, data);

      StartFaceEnrollmentBehavior();
      if (SharedMinigameView.gameObject.activeInHierarchy) {
        LaunchInitialGameState();
      }
    }

    private void StartFaceEnrollmentBehavior() {
      RobotEngineManager.Instance.CurrentRobot.ActivateHighLevelActivity(Anki.Cozmo.HighLevelActivity.MeetCozmoFindFaces);
    }

    private void HandleCozmoSleepCancelled() {
      StartFaceEnrollmentBehavior();
    }

    // Since this has some co-routines state can't be run until minigame view is active
    // don't start until sharedminigame view is active
    private void LaunchInitialGameState() {
      // if we have no faces enrolled let's skip the face list UI and go directly to enroll a new face
      // with the default profile name pre-populated.
      if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
        EnterNameForNewFace(string.Empty);
      }
      else {
        _StateMachine.SetNextState(new FaceSlideState());
      }
    }

    private void OnCancelFaceScan() {
      if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
        //If this is the first face, go back to the needs hub.
        SharedMinigameView.HandleQuitConfirmed();
      }
      else {
        _StateMachine.SetNextState(new FaceSlideState());
      }
    }

    public void EnterNameForNewFace(string preFilledName) {
      FaceNameSlideState newNameSlideState = new FaceNameSlideState();
      newNameSlideState.Initialize(preFilledName, (string faceName) => {
        FaceEnrollInstructionState faceEnrollInstructionState = new FaceEnrollInstructionState();
        faceEnrollInstructionState.Initialize(faceName, OnCancelFaceScan);
        _StateMachine.SetNextState(faceEnrollInstructionState);
      });
      _StateMachine.SetNextState(newNameSlideState);
    }

    public void RequestReEnrollFace(int faceId, string nameForFace) {
      FaceEnrollInstructionState faceEnrollInstructionState = new FaceEnrollInstructionState();
      faceEnrollInstructionState.Initialize(nameForFace, OnCancelFaceScan, faceId);
      _StateMachine.SetNextState(faceEnrollInstructionState);
    }

    public void ShowFaceListShelf() {
      SharedMinigameView.ShowShelf();
      _FaceEnrollmentShelfContentInstance = SharedMinigameView.ShelfWidget.SetShelfContent(_FaceEnrollmentShelfContentPrefab).GetComponent<FaceSlidesShelfContent>();
      _FaceEnrollmentShelfContentInstance.AddNewPersonPressed += HandleNewEnrollmentRequested;
      if (_ShowDoneShelf) {
        _FaceEnrollmentShelfContentInstance.SetShelfTextKey(LocalizationKeys.kFaceEnrollmentConditionAllChangesSaved);
        _FaceEnrollmentShelfContentInstance.ShowDoneButton(true);
        _FaceEnrollmentShelfContentInstance.GameDoneButtonPressed += () => {
          RaiseChallengeQuit();
        };
      }
      else {
        _FaceEnrollmentShelfContentInstance.SetShelfTextKey(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDescription);
        _FaceEnrollmentShelfContentInstance.ShowDoneButton(false);
      }
      _FaceEnrollmentShelfContentInstance.ShowAddNewPersonButton(RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count < UnlockablesManager.Instance.FaceSlotsSize());
    }

    public void ShowDetailsShelf(System.Action onDeleteCallback) {
      SharedMinigameView.ShowShelf();
      _FaceDetailsShelfContentInstance = SharedMinigameView.ShelfWidget.SetShelfContent(_FaceDetailsShelfContentPrefab).GetComponent<FaceDetailsShelfContent>();
      _FaceDetailsShelfContentInstance.EraseFacePressed += onDeleteCallback;
    }

    public void ShowMultipleFacesErrorShelf(string nameForFace, System.Action tryAgainButtonCallback) {
      SharedMinigameView.ShowShelf();
      _FaceMultipleFacesErrorShelfContentInstance = SharedMinigameView.ShelfWidget.SetShelfContent(_FaceMultipleFacesErrorShelfContentPrefab).GetComponent<FaceMultipleFacesErrorShelfContent>();
      _FaceMultipleFacesErrorShelfContentInstance.OnRetryButton += tryAgainButtonCallback;
      _FaceMultipleFacesErrorShelfContentInstance.SetNameLabel(nameForFace);
    }

    public void HandleDetailsViewRequested(int faceID, string nameForFace) {
      FaceEnrollmentDetailsSlideState detailsState = new FaceEnrollmentDetailsSlideState();
      detailsState.Initialize(faceID, nameForFace);
      _StateMachine.SetNextState(detailsState);
    }

    public void CompleteMeetCozmoOnboarding() {
      if (!OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.InitialSetup) &&
          OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.MeetCozmo)) {
        OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.MeetCozmo);
      }
      OnboardingManager.Instance.OnOnboardingPhaseCompleted -= HandleOnboardingPhaseComplete;
    }

    private void HandleSharedMinigameViewInitialized(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      // Onboarding hides meet cozmo so it pops up without loading, reactived on completion see HandleOnboardingPhaseComplete
      newView.gameObject.SetActive(!OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.InitialSetup));
      OnSharedMinigameViewInitialized -= HandleSharedMinigameViewInitialized;
    }

    private void HandleOnboardingPhaseComplete(OnboardingManager.OnboardingPhases phase) {
      if (phase == OnboardingManager.OnboardingPhases.InitialSetup) {
        if (SharedMinigameView != null) {
          SharedMinigameView.gameObject.SetActive(true);
          LaunchInitialGameState();
        }
      }
    }

    private void HandleNewEnrollmentRequested() {
      EnterNameForNewFace(string.Empty);
    }

    public override void RaiseChallengeQuit() {
      base.RaiseChallengeQuit();
      // Onboarding should only be completed if we quit a normal way, not a backgrounding or app killing.
      CompleteMeetCozmoOnboarding();
    }

    protected override void CleanUpOnDestroy() {
      Cozmo.PauseManager.Instance.OnCozmoSleepCancelled -= HandleCozmoSleepCancelled;
    }

  }

}
