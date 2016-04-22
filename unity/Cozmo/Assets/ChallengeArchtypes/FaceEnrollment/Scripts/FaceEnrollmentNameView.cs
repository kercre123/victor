using UnityEngine;
using System.Collections;
using Cozmo.UI;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentNameView : BaseView {

    public System.Action<string> OnSubmitButton;

    [SerializeField]
    public Cozmo.UI.CozmoButton _SubmitNameButton;

    [SerializeField]
    private InputField _NameField;

    void Start() {
      _SubmitNameButton.Initialize(HandleButtonPress, "save_name_to_face_button", "face_enrollment_name_view");
    }

    private void HandleButtonPress() {
      OnSubmitButton(_NameField.text);
    }

    protected override void CleanUp() {
    }
  }

}
