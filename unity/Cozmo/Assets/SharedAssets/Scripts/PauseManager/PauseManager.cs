using UnityEngine;
using System.Collections;

public class PauseManager : MonoBehaviour {
  private void OnApplicationPause(bool pauseStatus) {
    DAS.Info(this, "Pause Triggered: " + pauseStatus);
  }
}
