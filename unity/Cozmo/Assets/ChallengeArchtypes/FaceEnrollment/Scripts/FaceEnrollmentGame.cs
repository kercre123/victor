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

    [SerializeField]
    private FaceEnrollmentShelfContent _FaceEnrollmentShelfContentPrefab;
    private FaceEnrollmentShelfContent _FaceEnrollmentShelfContentInstance;

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
      // Meet Cozmo has special logic for this stuff so override the base class.
      // Reactionary behavior and custom freeplay behavior inside of MeetCozmo is set in
      // FaceSlideState class.

      // Disable ReactToPet to avoid false positive disruptions
      _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToPet);
    }

    protected override void SetupViewAfterCozmoReady(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.SetupViewAfterCozmoReady(newView, data);

      RobotEngineManager.Instance.CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.MeetCozmoFindFaces);

      // if we have no faces enrolled let's skip the face list UI and go directly to enroll a new face
      // with the default profile name pre-populated.
      if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
        EnterNameForNewFace(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
      }
      else {
        if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeFaceEnrollmentHowToPlay) {
          ShowHowToPlay();
          DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeFaceEnrollmentHowToPlay = false;
        }
        else {
          _StateMachine.SetNextState(new FaceSlideState());
        }

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
      _FaceEnrollmentShelfContentInstance = SharedMinigameView.ShelfWidget.SetShelfContent(_FaceEnrollmentShelfContentPrefab).GetComponent<FaceEnrollmentShelfContent>();
      if (_ShowDoneShelf) {
        _FaceEnrollmentShelfContentInstance.SetShelfTextKey(LocalizationKeys.kFaceEnrollmentConditionAllChangesSaved);
        _FaceEnrollmentShelfContentInstance.ShowDoneButton(true);
        _FaceEnrollmentShelfContentInstance.GameDoneButtonPressed += () => {
          RaiseMiniGameQuit();
        };
      }
      else {
        _FaceEnrollmentShelfContentInstance.SetShelfTextKey(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDescription);
        _FaceEnrollmentShelfContentInstance.ShowDoneButton(false);
      }

      _FaceEnrollmentShelfContentInstance.HowToPlayButtonPressed += () => {
        ShowHowToPlay();
      };

    }

    public void ShowHowToPlay() {
      // this is our first face enrollment so show the how to play first
      ContextManager.Instance.AppFlash(playChime: true);
      _StateMachine.SetNextState(new FaceEnrollmentHowToPlayState());
    }

    protected override void CleanUpOnDestroy() {
      
    }

  }

}
