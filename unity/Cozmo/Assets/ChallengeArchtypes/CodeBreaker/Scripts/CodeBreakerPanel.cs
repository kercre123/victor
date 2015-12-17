using UnityEngine;
using System.Collections;
using Cozmo.UI;
using Anki.UI;

namespace CodeBreaker {
  public class CodeBreakerPanel : MonoBehaviour {

    [SerializeField]
    private AnkiButton _SubmitGuessButton;

    [SerializeField]
    private AnkiTextLabel _GuessesLeftLabel;

  }
}