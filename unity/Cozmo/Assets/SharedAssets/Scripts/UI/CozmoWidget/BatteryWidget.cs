using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;

public class BatteryWidget : BaseView {

  [SerializeField]
  Cozmo.UI.ProgressBar _BatteryBar;

  public void Awake() {
    RobotEngineManager.Instance.OnEmotionRecieved += UpdateBatteryLife;
  }

  private void UpdateBatteryLife(Anki.Cozmo.EmotionType index, float value) {
    if (index != Anki.Cozmo.EmotionType.Charged) {
      return;
    }
    _BatteryBar.SetProgress(value);
  }

  protected override void CleanUp() {
    RobotEngineManager.Instance.OnEmotionRecieved -= UpdateBatteryLife;
  }
}
