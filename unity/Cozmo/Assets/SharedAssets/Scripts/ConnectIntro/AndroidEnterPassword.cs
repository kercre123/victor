using Anki.UI;
using Cozmo.UI;
using UnityEngine;
using UnityEngine.UI;

// Screen for entering the password for the user's Cozmo
public class AndroidEnterPassword : AndroidConnectionFlowStage {

  [SerializeField]
  private AnkiTextLabel _InstructionsLabel;

  [SerializeField]
  private InputField _PasswordField;

  [SerializeField]
  private AnkiTextLabel _ErrorLabel;

  [SerializeField]
  private CozmoButton _ContinueButton;

  [SerializeField]
  private AnkiButton _WrongCozmoButton;

  [SerializeField]
  private AnkiTextLabel _WrongPasswordLabel;

  public bool WrongPassword { set { _WrongPasswordLabel.gameObject.SetActive(value); } }

  private void Start() {
    _PasswordField.onValueChanged.AddListener(HandlePasswordChanged);
    _InstructionsLabel.FormattingArgs = new object[] { AndroidConnectionFlow.Instance.SelectedSSID };

    _ContinueButton.Initialize(HandleContinueButton, "continue_button", "android_enter_password");
    _ContinueButton.Interactable = false;

    _WrongCozmoButton.Initialize(AndroidConnectionFlow.Instance.WrongCozmoSelected, "wrong_cozmo", "android_enter_password");
  }

  private void HandlePasswordChanged(string password) {
    bool validPassword = password.Length == 12;
    if (validPassword) {
      // see if characters are valid
      foreach (char c in password) {
        if (!char.IsLetterOrDigit(c)) {
          validPassword = false;
          break;
        }
      }
    }
    _ErrorLabel.gameObject.SetActive(!validPassword);
    _ContinueButton.Interactable = validPassword;
    if (validPassword) {
      AndroidConnectionFlow.Instance.Password = password.ToUpper();
    }
  }

  private void HandleContinueButton() {
    var SSID = AndroidConnectionFlow.Instance.SelectedSSID;
    var password = AndroidConnectionFlow.Instance.Password;
    bool result = AndroidConnectionFlow.CallJava<bool>("connect", SSID, password, AndroidConnectionFlow.kTimeoutMs);
    DAS.Info("AndroidEnterPassword.Continue", "Connecting to network " + SSID + ", result " + result);
    OnStageComplete();
  }
}
