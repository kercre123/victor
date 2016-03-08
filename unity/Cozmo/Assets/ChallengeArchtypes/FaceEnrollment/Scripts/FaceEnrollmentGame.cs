using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    public int _ReactToFaceID = 0;
    public Dictionary<int, string> _FaceNameDictionary = new Dictionary<int, string>();
    public Dictionary<int, string> _FaceIDToReaction = new Dictionary<int, string>();

    private string[] _ReactionBank = {
      AnimationName.kSpeedTap_loseHand_01,
      AnimationName.kSpeedTap_winHand_01,
      AnimationName.kSpeedTap_winSession_01,
      AnimationName.kSpeedTap_loseSession_03
    };

    private int _ReactionIndex = 0;

    private float _LastPlayedReaction = 0.0f;

    [SerializeField]
    private FaceEnrollmentDataView _FaceEnrollmentDataViewPrefab;
    private FaceEnrollmentDataView _FaceEnrollmentDataView;

    [SerializeField]
    private FaceEnrollmentNameView _FaceEnrollmentViewPrefab;
    private FaceEnrollmentNameView _FaceEnrollmentView;

    private int _NewSeenFaceID = 0;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(0.5f);
      RobotEngineManager.Instance.RobotObservedNewFace += HandleObservedNewFace;

      _FaceEnrollmentDataView = 
        SharedMinigameView.ShowWideGameStateSlide(_FaceEnrollmentDataViewPrefab.gameObject, 
        "face_enrollment_data_view_panel").GetComponent<FaceEnrollmentDataView>();
      
      _FaceEnrollmentDataView.OnEnrollNewFace += AnimateFaceEnroll;
    }

    private void AnimateFaceEnroll() {
      CurrentRobot.SendAnimation(AnimationName.kSpeedTap_winHand_01, LookForNewFaceToEnroll);
    }

    private void LookForNewFaceToEnroll(bool success) {
      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(0.5f);
      CurrentRobot.EnableNewFaceEnrollment();
    }

    public void EnrollFace(string nameForFace) {
      _FaceNameDictionary.Add(_NewSeenFaceID, nameForFace);
      CurrentRobot.AssignNameToFace(_NewSeenFaceID, nameForFace);

      _FaceIDToReaction.Add(_NewSeenFaceID, _ReactionBank[_ReactionIndex]);
      _ReactionIndex = (_ReactionIndex + 1) % _ReactionBank.Length;
    }

    private void HandleObservedNewFace(int faceID, Vector3 facePos, Quaternion faceRot) {
      if (_FaceEnrollmentView != null) {
        return; // we already have a view open to ask for a name.
      }
      // I found a new face! let's get the name from the user.
      _FaceEnrollmentView = UIManager.OpenView<FaceEnrollmentNameView>(_FaceEnrollmentViewPrefab);
      _FaceEnrollmentView.OnSubmitButton += HandleSubmitButton;
      _NewSeenFaceID = faceID;
    }

    private void HandleSubmitButton(string name) {
      EnrollFace(name);
      _NewSeenFaceID = 0;
      UIManager.CloseView(_FaceEnrollmentView);
      _FaceEnrollmentView = null;
    }

    private void HandleOnAnyFaceSeen(int faceID, string name, Vector3 pos, Quaternion rot) {
      // check if we have an active face enroll view open
      if (_FaceEnrollmentView == null) {
        if (faceID > 0 && string.IsNullOrEmpty(name) == false && _FaceIDToReaction.ContainsKey(faceID)) {
          // this is a face we know...
          if (Time.time - _LastPlayedReaction > 8.0f) {
            // been at least 8 seconds since we reacted.
            CurrentRobot.SendAnimation(_FaceIDToReaction[faceID]);
            _LastPlayedReaction = Time.time;
          }
        }
      }
    }

    protected override void CleanUpOnDestroy() {
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
    }


  }

}
