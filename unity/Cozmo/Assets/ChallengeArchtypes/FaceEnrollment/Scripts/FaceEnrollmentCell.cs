using UnityEngine;
using System.Collections;

public class FaceEnrollmentCell : MonoBehaviour {

  public System.Action<int, string> OnEditNameRequested;
  public System.Action<int, string> OnReEnrollFaceRequested;
  public System.Action<int> OnDeleteNameRequested;

  private string _FaceName;
  private int _FaceID;
  private bool _NeedsUpdate;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _NameLabel;

  [SerializeField]
  private Cozmo.UI.CozmoButton _EditButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _DeleteButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ReEnrollFace;

  [SerializeField]
  private UnityEngine.UI.Image _UpdateImage;

  public void Initialize(int faceID, string faceName, bool needsUpdate) {
    _FaceName = faceName;
    _FaceID = faceID;
    _NameLabel.text = _FaceName;

    //Always show the "Update" banner on all enrolled faces until we can make a better user experience around this process.
    _UpdateImage.gameObject.SetActive(true);

    _EditButton.Initialize(HandleEditNameClicked, "edit_button", "face_enrollment_cell");
    _DeleteButton.Initialize(HandleDeleteNameClicked, "delete_button", "face_enrollment_cell");
    _ReEnrollFace.Initialize(HandleReEnrollFaceClicked, "reenroll_face_button", "face_enrollment_cell");
  }

  private void HandleReEnrollFaceClicked() {
    if (OnReEnrollFaceRequested != null) {

      Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab);
      alertView.SetCloseButtonEnabled(false);
      alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentReenrollmentAlertTitle;
      alertView.SetTitleArgs(new object[] { _FaceName });
      alertView.DescriptionLocKey = LocalizationKeys.kFaceEnrollmentReenrollmentAlertDescription;
      alertView.SetPrimaryButton(LocalizationKeys.kFaceEnrollmentReenrollmentAlertConfirmButton, HandleReEnrollConfirm);
      alertView.SetPrimaryButtonArgs(new object[] { _FaceName });
      alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel);

    }
  }

  private void HandleReEnrollConfirm() {
    OnReEnrollFaceRequested(_FaceID, _FaceName);
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
