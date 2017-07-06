using UnityEngine;
using System.Collections;
using Cozmo.UI;

public class FaceMultipleFacesErrorShelfContent : MonoBehaviour {

  public System.Action OnRetryButton;

  [SerializeField]
  private CozmoButton _RetryButton;

  [SerializeField]
  private CozmoText _NameLabel;

  public void SetNameLabel(string name) {
    _NameLabel.FormattingArgs = new string[] { name };
  }

  private void Start() {
    _RetryButton.Initialize(() => {
      if (OnRetryButton != null) {
        OnRetryButton();
      }
    }, "retry_button", "multiple_faces_error_shelf");
  }

}
