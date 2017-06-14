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

    protected override void InitializeGame(ChallengeConfigBase challengeConfigData) {

      FaceEnrollmentGameConfig faceEnrollmentConfig = (FaceEnrollmentGameConfig)challengeConfigData;

      _UpdateThresholdLastSeenSeconds = faceEnrollmentConfig.UpdateThresholdLastSeenSeconds;
      _UpdateThresholdLastEnrolledSeconds = faceEnrollmentConfig.UpdateThresholdLastEnrolledSeconds;

      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.MeetCozmo)) {
        if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
          SharedMinigameView.HideQuitButton();
        }
        OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.MeetCozmo);
      }
    }

    protected override void InitializeReactionaryBehaviorsForGameStart() {
      RobotEngineManager.Instance.CurrentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kMinigameId, ReactionaryBehaviorEnableGroups.kFaceEnrollmentTriggers);
    }

    protected override void SetupViewAfterCozmoReady(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.SetupViewAfterCozmoReady(newView, data);

      RobotEngineManager.Instance.CurrentRobot.ActivateHighLevelActivity(Anki.Cozmo.HighLevelActivity.MeetCozmoFindFaces);

      // if we have no faces enrolled let's skip the face list UI and go directly to enroll a new face
      // with the default profile name pre-populated.
      if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
        EnterNameForNewFace(string.Empty);
      }
      else {
        _StateMachine.SetNextState(new FaceSlideState());
      }

    }

    public void EnterNameForNewFace(string preFilledName) {
      FaceNameSlideState newNameSlideState = new FaceNameSlideState();
      newNameSlideState.Initialize(preFilledName, (string faceName) => {
        FaceEnrollInstructionState faceEnrollInstructionState = new FaceEnrollInstructionState();
        faceEnrollInstructionState.Initialize(faceName, () => EnterNameForNewFace(faceName));
        _StateMachine.SetNextState(faceEnrollInstructionState);
      });
      _StateMachine.SetNextState(newNameSlideState);
    }

    public void RequestReEnrollFace(int faceId, string nameForFace) {
      FaceEnrollInstructionState faceEnrollInstructionState = new FaceEnrollInstructionState();
      faceEnrollInstructionState.Initialize(nameForFace, () => _StateMachine.SetNextState(new FaceSlideState()), faceId);
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
      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.MeetCozmo)) {
        OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.MeetCozmo);
      }
    }

    private void HandleNewEnrollmentRequested() {
      // if this the first name then we should pre-populate the name with the profile name.
      if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
        EnterNameForNewFace(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
      }
      else {
        EnterNameForNewFace("");
      }
    }

    protected override void CleanUpOnDestroy() {
      CompleteMeetCozmoOnboarding();
    }

  }

}
