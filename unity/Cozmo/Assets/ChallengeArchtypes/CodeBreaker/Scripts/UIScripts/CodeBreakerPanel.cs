using UnityEngine;
using System.Collections;
using Cozmo.UI;
using Anki.UI;

namespace CodeBreaker {
  public delegate void SubmitButtonClickedHandler();
  public class CodeBreakerPanel : MonoBehaviour {

    [SerializeField]
    private AnkiButton _SubmitGuessButton;

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
  }
}