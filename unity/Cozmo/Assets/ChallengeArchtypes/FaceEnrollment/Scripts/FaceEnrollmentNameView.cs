using UnityEngine;
using System.Collections;
using Cozmo.UI;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentNameView : BaseView {

    public System.Action<string> OnSubmitButton;

    [SerializeField]
    public Anki.UI.AnkiButton _SubmitNameButton;

    [SerializeField]
    private InputField _NameField;

    void Start() {
      _SubmitNameButton.DASEventButtonName = "face_enrollment_name_view_submit_button";
      _SubmitNameButton.DASEventViewController = "face_enrollment_name_view";
      _SubmitNameButton.onClick.AddListener(HandleButtonPress);
    }

    private void HandleButtonPress() {
      OnSubmitButton(_NameField.text);
    }

    protected override void CleanUp() {
    }
  }

}
