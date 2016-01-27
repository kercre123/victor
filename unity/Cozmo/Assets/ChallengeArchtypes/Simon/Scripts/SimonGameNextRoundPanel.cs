using UnityEngine;
using System.Collections;
using Cozmo.UI;
using Anki.UI;

namespace Simon {
  public class SimonGameNextRoundPanel : MonoBehaviour {

    [SerializeField]
    private AnkiTextLabel _NextPlayerLabel;

    [SerializeField]
    private AnkiButton _ContinueButton;

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

    public void SetNextPlayerText(PlayerType player) {
      string locKey;
      if (player == PlayerType.Human) {
        locKey = LocalizationKeys.kSimonGameLabelYourTurn;
      }
      else {
        locKey = LocalizationKeys.kSimonGameLabelCozmoTurn;
      }
      _NextPlayerLabel.text = Localization.Get(locKey);
    }
  }
}
