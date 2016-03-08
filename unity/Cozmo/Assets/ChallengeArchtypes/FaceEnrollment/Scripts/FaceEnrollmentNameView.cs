using UnityEngine;
using System.Collections;
using Cozmo.UI;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentNameView : BaseView {

    public System.Action<string> OnSubmitButton;

    [SerializeField]
    public Button _SubmitNameButton;

    [SerializeField]
    private InputField _NameField;

    void Start() {
      _SubmitNameButton.onClick.AddListener(HandleButtonPress);
    }

    private void HandleButtonPress() {
      OnSubmitButton(_NameField.text);
    }

    protected override void CleanUp() {
    }
  }

}
