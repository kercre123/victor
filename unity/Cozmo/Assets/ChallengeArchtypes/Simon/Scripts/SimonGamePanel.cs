using UnityEngine;
using System.Collections;
using Cozmo.UI;

namespace Simon {
  public class SimonGamePanel : BaseView {

    [SerializeField]
    private Anki.UI.AnkiButton _ContinueButton;

    public delegate void SimonButtonPressedHandler();

    public SimonButtonPressedHandler OnContinueButtonPressed;

    void Start() {
      _ContinueButton.onClick.AddListener(HandleOnContinueButton);
    }

    public void EnableContinueButton(bool enabled) {
      _ContinueButton.Interactable = enabled;
    }

    void HandleOnContinueButton() {
      if (OnContinueButtonPressed != null) {
        OnContinueButtonPressed();
      }
    }

    protected override void CleanUp() {

    }
  }

}
