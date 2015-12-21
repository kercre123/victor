using UnityEngine;
using System.Collections;
using Anki.UI;

namespace CodeBreaker {
  public delegate void ReadyButtonClickedHandler();

  public class CodeBreakerReadySlide : MonoBehaviour {

    [SerializeField]
    private AnkiButton _ReadyButton;

    public event ReadyButtonClickedHandler OnReadyButtonClicked;

    private void Start() {
      _ReadyButton.onClick.AddListener(HandleReadyButtonClicked);
    }

    public void SetButtonText(string text) {
      _ReadyButton.Text = text;
    }

    public void EnableButton(bool enable) {
      _ReadyButton.interactable = enable;
    }

    private void HandleReadyButtonClicked() {
      if (OnReadyButtonClicked != null) {
        OnReadyButtonClicked();
      }
    }
  }
}