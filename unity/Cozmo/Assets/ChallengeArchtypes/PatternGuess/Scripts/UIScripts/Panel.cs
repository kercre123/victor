using UnityEngine;
using System.Collections;
using Cozmo.UI;
using Anki.UI;

namespace PatternGuess {
  public delegate void SubmitButtonClickedHandler();
  public class Panel : MonoBehaviour {

    [SerializeField]
    private CozmoButton _SubmitGuessButton;

    [SerializeField]
    private AnkiTextLabel _GuessesLeftLabel;

    public event SubmitButtonClickedHandler OnSubmitButtonClicked;

    private void Start() {
      _SubmitGuessButton.onClick.AddListener(HandleSubmitButtonClicked);
    }

    private void HandleSubmitButtonClicked() {
      if (OnSubmitButtonClicked != null) {
        OnSubmitButtonClicked();
      }
    }

    public bool EnableButton {
      get { return _SubmitGuessButton.Interactable; }
      set { _SubmitGuessButton.Interactable = value; }
    }

    public void SetGuessesLeft(int guessesLeft) {
      _GuessesLeftLabel.text = Localization.GetWithArgs(LocalizationKeys.kPatternGuessTextGuessesLeft, guessesLeft);
    }
  }
}