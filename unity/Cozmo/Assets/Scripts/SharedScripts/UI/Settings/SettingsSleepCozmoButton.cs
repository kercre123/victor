using UnityEngine;
using Cozmo.UI;

namespace Cozmo.Settings {
  public class SettingsSleepCozmoButton : MonoBehaviour {

    [SerializeField]
    private CozmoButton _SleepCozmoButton;

    // Use this for initialization
    private void Awake() {
      _SleepCozmoButton.Initialize(HandleSleepCozmoButtonTapped, "settings_sleep_cozmo_button", "settings_sleep_cozmo_panel");
    }

    private void HandleSleepCozmoButtonTapped() {
      PauseManager.Instance.OpenConfirmSleepCozmoDialog(handleOnChargerSleepCancel: false);
    }
  }
}