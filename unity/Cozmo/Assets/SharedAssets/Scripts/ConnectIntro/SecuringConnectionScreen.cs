using UnityEngine;
using System.Collections;

public class SecuringConnectionScreen : MonoBehaviour {
  public System.Action<bool> OnScreenComplete;

  private void Start() {
    Invoke("ShowScreenComplete", ConnectionFlow.kConnectionFlowDelay);
  }

  private void ShowScreenComplete() {
    if (OnScreenComplete != null) {
      OnScreenComplete(true);
    }
  }
}
