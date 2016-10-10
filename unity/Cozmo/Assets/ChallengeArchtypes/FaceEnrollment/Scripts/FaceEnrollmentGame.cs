using UnityEngine;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    [SerializeField]
    private FaceEnrollmentInstructionsView _FaceEnrollmentInstructionsViewPrefab;
    public FaceEnrollmentInstructionsView FaceEnrollmentInstructionsViewPrefab {
      get { return _FaceEnrollmentInstructionsViewPrefab; }
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
    private GameObject _FaceEnrollmentHowToPlaySlidePrefab;
    public GameObject FaceEnrollmentHowToPlaySlidePrefab {
      get { return _FaceEnrollmentHowToPlaySlidePrefab; }
    }

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

    protected override void InitializeGame(MinigameConfigBase minigameConfig) {

      FaceEnrollmentGameConfig faceEnrollmentConfig = (FaceEnrollmentGameConfig)minigameConfig;

      _UpdateThresholdLastSeenSeconds = faceEnrollmentConfig.UpdateThresholdLastSeenSeconds;
      _UpdateThresholdLastEnrolledSeconds = faceEnrollmentConfig.UpdateThresholdLastEnrolledSeconds;
    }

    protected override void AddDisabledReactionaryBehaviors() {
      // meet cozmo has special logic for this stuff so override the base class and do nothing here.
      // reactionary behavior and custom freeplay behavior inside of MeetCozmo is set in
      // FaceSlideState class.
    }

    protected override void SetupViewAfterCozmoReady(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.SetupViewAfterCozmoReady(newView, data);

      // if we have no faces enrolled let's skip the face list UI and go directly to enroll a new face
      // with the default profile name pre-populated.
      if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
        EnterNameForNewFace(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
        // explicitly disable reactionary behaviors because we are now skipping FaceSlideState.
        SetReactionaryBehaviors(false);
      }
      else {
        _StateMachine.SetNextState(new FaceSlideState());
      }

    }

    public void EditExistingName(int faceID, string existingName) {
      FaceNameSlideState editNameSlideState = new FaceNameSlideState();
      editNameSlideState.Initialize(existingName, (string newFaceName) => {
        FaceEditDoneState faceEditEnteredState = new FaceEditDoneState();
        faceEditEnteredState.Initialize(faceID, existingName, newFaceName);
        _StateMachine.SetNextState(faceEditEnteredState);
      });
      _StateMachine.SetNextState(editNameSlideState);
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

    public void RequestReEnrollFace(int faceId, string faceName) {
      FaceEnrollInstructionState faceEnrollInstructionState = new FaceEnrollInstructionState();
      faceEnrollInstructionState.Initialize(faceName, () => _StateMachine.SetNextState(new FaceSlideState()), faceId);
      _StateMachine.SetNextState(faceEnrollInstructionState);
    }

    public void ShowShelf() {
      SharedMinigameView.ShowShelf();

      if (_ShowDoneShelf) {
        SharedMinigameView.ShelfWidget.SetWidgetText(LocalizationKeys.kFaceEnrollmentConditionAllChangesSaved);
        SharedMinigameView.ShelfWidget.ShowGameDoneButton(true);
        SharedMinigameView.ShelfWidget.GameDoneButtonPressed += () => {
          RaiseMiniGameQuit();
        };
      }
      else {
        SharedMinigameView.ShelfWidget.SetWidgetText(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDescription);
      }

    }

    public void SetReactionaryBehaviors(bool enable) {
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.AcknowledgeFace, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.AcknowledgeObject, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToCubeMoved, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToCliff, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToPickup, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToUnexpectedMovement, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToFrustration, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.KnockOverCubes, enable);
      }
    }

    protected override void CleanUpOnDestroy() {
      SetReactionaryBehaviors(true);
    }

  }

}
