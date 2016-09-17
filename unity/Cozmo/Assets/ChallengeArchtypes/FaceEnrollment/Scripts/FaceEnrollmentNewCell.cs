using UnityEngine;
using System.Collections;

public class FaceEnrollmentNewCell : MonoBehaviour {

  public System.Action OnCreateNewButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _CreateNewButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _CreateNewButtonFace;

  [SerializeField]
  private Sprite _DisabledLockSprite;

  [SerializeField]
  private UnityEngine.UI.Image _MainCellImage;

  private void Awake() {
    _CreateNewButtonFace.Initialize(HandleEnrollNewFaceButton, "create_new_face_button", "face_enrollment_new_cell");
    _CreateNewButton.Initialize(HandleEnrollNewFaceButton, "create_new_button", "face_enrollment_new_cell");
  }

  public void EnableCell(bool enable) {
    _CreateNewButtonFace.Interactable = enable;
    _CreateNewButton.gameObject.SetActive(enable);
    if (!enable) {
      _MainCellImage.overrideSprite = _DisabledLockSprite;
    }
    else {
      _MainCellImage.overrideSprite = null;
    }
  }

  private void HandleEnrollNewFaceButton() {
    if (OnCreateNewButton != null) {
      OnCreateNewButton();
    }
  }

}
