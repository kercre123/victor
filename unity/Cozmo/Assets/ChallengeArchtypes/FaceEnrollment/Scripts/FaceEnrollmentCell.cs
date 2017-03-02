using UnityEngine;
using Cozmo.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentCell : MonoBehaviour {

    public System.Action<int, string> OnDetailsViewRequested;

    private string _NameForFace;
    private int _FaceID;

    [SerializeField]
    private Anki.UI.AnkiTextLegacy _NameLabel;

    [SerializeField]
    private CozmoButtonLegacy _FaceCellButton;

    public void Initialize(int faceID, string nameForFace) {
      _NameForFace = nameForFace;
      _FaceID = faceID;
      _NameLabel.text = _NameForFace;
      _FaceCellButton.Initialize(HandleFaceCellButton, "face_cell_button", "face_enrollment_cell");
    }

    private void HandleFaceCellButton() {
      if (OnDetailsViewRequested != null) {
        OnDetailsViewRequested(_FaceID, _NameForFace);
      }
    }

  }
}