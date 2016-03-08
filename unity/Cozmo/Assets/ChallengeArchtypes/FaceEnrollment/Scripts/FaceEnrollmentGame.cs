using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    public int _ReactToFaceID = 0;
    public Dictionary<int, string> _FaceNameDictionary = new Dictionary<int, string>();

    [SerializeField]
    private Button _EnrollNewFace;

    [SerializeField]
    private Button _EnrollButton;

    [SerializeField]
    private InputField _NameInputField;

    private int _NewSeenFaceID;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      LookForNewFaceToEnroll();
      RobotEngineManager.Instance.RobotObservedNewFace += HandleObservedNewFace;
      _EnrollButton.onClick.AddListener(EnrollFace);
    }

    private void LookForNewFaceToEnroll() {
      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(0.5f);
      CurrentRobot.EnableNewFaceEnrollment();
    }

    private void EnrollFace() {
      _FaceNameDictionary.Add(_NewSeenFaceID, _NameInputField.text);
    }

    private void HandleObservedNewFace(uint robotID, int faceID, Vector3 facePos, Quaternion faceRot) {
      // found a new face. let's try to enroll
      _NewSeenFaceID = faceID;
    }

    protected override void CleanUpOnDestroy() {
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
    }

  }

}
