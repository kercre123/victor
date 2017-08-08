using Anki.UI;
using Cozmo.UI;
using UnityEngine;
using UnityEngine.UI;

// Screen for entering the password for the user's Cozmo
public class AndroidEnterPassword : AndroidConnectionFlowStage {

  [SerializeField]
  private CozmoText _InstructionsLabel;

  [SerializeField]
  private InputField _PasswordField;

  [SerializeField]
  private CozmoText _ErrorLabel;

  [SerializeField]
  private CozmoButton _ContinueButton;

  [SerializeField]
  private CozmoButton _WrongCozmoButton;

  [SerializeField]
  private CozmoText _WrongPasswordLabel;

  protected bool _WrongPassword = false;
  public bool WrongPassword {
    set {
      _WrongPassword = value;
      _WrongPasswordLabel.gameObject.SetActive(value);
    }
  }

  private void Start() {
    _PasswordField.onValueChanged.AddListener(HandlePasswordChanged);
    _PasswordField.onValidateInput = ValidateCharacter;
    _InstructionsLabel.FormattingArgs = new object[] { AndroidConnectionFlow.Instance.SelectedSSID };
    _ErrorLabel.gameObject.SetActive(false);
    _WrongPasswordLabel.gameObject.SetActive(_WrongPassword);

    _ContinueButton.Initialize(HandleContinueButton, "continue_button", "android_enter_password");
    _ContinueButton.Interactable = false;

    _WrongCozmoButton.Initialize(AndroidConnectionFlow.Instance.WrongCozmoSelected, "wrong_cozmo", "android_enter_password");
  }

  private void HandlePasswordChanged(string password) {
    bool validPassword = (password.Length == 12) || (password.Length == 17);
    if (validPassword) {
      // see if characters are valid
      foreach (char c in password) {
        if (!(char.IsLetterOrDigit(c) || c.Equals('-'))) {
          validPassword = false;
          break;
        }
      }
    }
    password = password.ToUpper();
    _PasswordField.text = password;
    _ErrorLabel.gameObject.SetActive(!validPassword);
    _ContinueButton.Interactable = validPassword;
    if (validPassword) {
      AndroidConnectionFlow.Instance.Password = password;
    }
  }

  private char ValidateCharacter(string text, int index, char inputChar) {
    return char.ToUpper(inputChar);
  }

  private void HandleContinueButton() {
    var instance = AndroidConnectionFlow.Instance;
    var SSID = instance.SelectedSSID;
    var password = instance.Password;
    bool result = instance.Connect(SSID, password);
    DAS.Info("AndroidEnterPassword.Continue", "Connecting to network " + SSID + ", result " + result);
    OnStageComplete();
  }
}
