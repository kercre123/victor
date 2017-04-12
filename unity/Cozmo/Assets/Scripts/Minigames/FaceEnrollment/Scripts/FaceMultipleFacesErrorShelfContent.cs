using UnityEngine;
using System.Collections;

public class FaceMultipleFacesErrorShelfContent : MonoBehaviour {

  public System.Action OnRetryButton;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _RetryButton;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _NameLabel;

  public void SetNameLabel(string name) {
    _NameLabel.text = name;
  }

  private void Start() {
    _RetryButton.Initialize(() => {
      if (OnRetryButton != null) {
        OnRetryButton();
      }
    }, "retry_button", "multiple_faces_error_shelf");
  }

}
