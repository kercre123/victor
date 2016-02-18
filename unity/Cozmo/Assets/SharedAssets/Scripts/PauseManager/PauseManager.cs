using UnityEngine;
using System.Collections;

public class PauseManager : MonoBehaviour {
  private void OnApplicationFocus(bool focusStatus) {
    DAS.Info(this, "App Focussed: " + focusStatus);
  }
}
