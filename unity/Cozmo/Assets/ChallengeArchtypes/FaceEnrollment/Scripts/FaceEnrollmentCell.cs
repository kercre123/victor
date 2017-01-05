using UnityEngine;
using Cozmo.UI;
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
  private CozmoButton _EditButton;

  [SerializeField]
  private CozmoButton _DeleteButton;

  [SerializeField]
  private CozmoButton _ReEnrollFace;

  [SerializeField]
  private UnityEngine.UI.Image _UpdateImage;

  private AlertModal _ReEnrollAlertModal = null;

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

  private void OnDestroy() {
    if (_ReEnrollAlertModal != null) {
      _ReEnrollAlertModal.CloseDialogImmediately();
    }
  }

  private void HandleReEnrollFaceClicked() {
    if (OnReEnrollFaceRequested != null) {
      AlertModalData reenrollAlertData = new AlertModalData("reenroll_face_alert",
                                                                              LocalizationKeys.kFaceEnrollmentReenrollmentAlertTitle,
                                                                              LocalizationKeys.kFaceEnrollmentReenrollmentAlertDescription,
                                                                              new AlertModalButtonData("confirm_button", LocalizationKeys.kFaceEnrollmentReenrollmentAlertConfirmButton, HandleReEnrollConfirm),
                                                                              new AlertModalButtonData("cancel_button", LocalizationKeys.kButtonCancel),
                                                                              titleLocArgs: new object[] { _FaceName },
                                                                              primaryButtonLocArgs: new object[] { _FaceName });
      UIManager.OpenAlert(reenrollAlertData, new ModalPriorityData(), HandleReEnrollAlertCreated);
    }
  }

  private void HandleReEnrollAlertCreated(AlertModal alertModal) {
    _ReEnrollAlertModal = alertModal;
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
