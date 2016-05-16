using UnityEngine;
using System.Collections;

public class FaceEnrollmentPane : MonoBehaviour {
  [SerializeField]
  UnityEngine.UI.Button _EraseAllEnrolledFacesButton;

  private void Start() {
    _EraseAllEnrolledFacesButton.onClick.AddListener(() => RobotEngineManager.Instance.CurrentRobot.EraseAllEnrolledFaces());
  }

}
