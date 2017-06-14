using Cozmo.UI;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

namespace Cozmo.Needs.UI {
  public class NeedsMeter : MonoBehaviour {
    [SerializeField]
    private CozmoButton _MeterButton;

    [SerializeField]
    private ProgressBar _ProgressBar;
    public ProgressBar ProgressBar { get { return _ProgressBar; } }

    public bool AllowInput {
      get {
        return _MeterButton.enabled;
      }
      set {
        // If we are disabling input, we still want to show the button in an "unpressed, enabled" state
        // Potentially in the future (art TBD) we would show a static image instead of using the button
        if (!value) {
          ButtonEnabled = true;
        }
        _MeterButton.enabled = value;
      }
    }

    public CozmoButton Button {
      get {
        return _MeterButton;
      }
    }

    public bool ButtonEnabled {
      get {
        return _MeterButton.Interactable;
      }
      set {
        // If we are disabling input, we still want to show the button in an "unpressed, enabled" state all the time
        if (AllowInput) {
          _MeterButton.Interactable = value;
        }
        else {
          _MeterButton.Interactable = true;
        }
      }
    }

    public void Initialize(bool allowInput, UnityAction buttonClickCallback,
                           string dasButtonName, string dasParentDialogName) {
      AllowInput = allowInput;
      _MeterButton.Initialize(buttonClickCallback, dasButtonName, dasParentDialogName);
    }
  }
}