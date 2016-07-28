using UnityEngine;
using System.Collections;

public class PinSecurityView : Cozmo.UI.BaseView {

  public System.Action<string> OnPinEntered;
  public System.Action OnQuitButton;

  private int[] _PinCode = new int[4];
  private int _PinIndex = 0;
  private bool _PinFilled = false;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _PinFeedbackLabel;

  [SerializeField]
  private Cozmo.UI.CozmoButton[] _NumPadButtons;

  private void Start() {
    for (int i = 0; i < _NumPadButtons.Length; ++i) {
      int numPadValue = i;
      _NumPadButtons[i].Initialize(null, "num_pad_" + i, this.DASEventViewName);
      _NumPadButtons[i].onClick.AddListener(() => HandlePinButton(numPadValue));
    }
  }

  private void HandlePinButton(int pinValue) {
    if (_PinFilled) return;

    _PinCode[_PinIndex] = pinValue;
    _PinIndex++;

    _PinFeedbackLabel.text = PinToString();

    if (_PinIndex >= _PinCode.Length) {
      PinComplete();
    }
  }

  private string PinToString() {
    string pin = "";
    for (int i = 0; i < _PinIndex; ++i) {
      pin += _PinCode[i].ToString();
    }
    return pin;
  }

  private void PinComplete() {
    _PinFilled = true;

    if (OnPinEntered != null) {
      OnPinEntered(PinToString());
    }
  }

  protected override void CleanUp() {

  }
}
