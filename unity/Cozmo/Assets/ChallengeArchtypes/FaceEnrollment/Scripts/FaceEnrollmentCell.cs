using UnityEngine;
using System.Collections;

public class FaceEnrollmentCell : MonoBehaviour {

  public System.Action<int, string> OnEditNameRequested;
  public System.Action<int> OnDeleteNameRequested;

  private string _FaceName;
  private int _FaceID;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _NameLabel;

  [SerializeField]
  private Cozmo.UI.CozmoButton _EditButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _DeleteButton;

  public void Initialize(int faceID, string faceName) {
    _FaceName = faceName;
    _FaceID = faceID;
    _NameLabel.text = _FaceName;

    _EditButton.Initialize(HandleEditNameClicked, "edit_button", "face_enrollment_cell");
    _DeleteButton.Initialize(HandleDeleteNameClicked, "delete_button", "face_enrollment_cell");
  }

  private void HandleEditNameClicked() {
    if (OnEditNameRequested != null) {
      OnEditNameRequested(_FaceID, _FaceName);
    }
  }

  private void HandleDeleteNameClicked() {
    if (OnDeleteNameRequested != null) {
      OnDeleteNameRequested(_FaceID);
    }
  }
}
