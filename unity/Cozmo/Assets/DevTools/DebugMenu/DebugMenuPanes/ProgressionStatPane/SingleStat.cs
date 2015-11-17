using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SingleStat : MonoBehaviour {

  private Anki.Cozmo.ProgressionStatType _StatEnum;

  [SerializeField]
  private InputField _StatInputField;

  [SerializeField]
  private Text _StatLabel;

  public void Init(string lbl, uint progress, Anki.Cozmo.ProgressionStatType statEnum) {
    _StatLabel.text = lbl;
    _StatInputField.text = progress.ToString("D");
    _StatEnum = statEnum;

    _StatInputField.onValueChange.AddListener(HandleValueChanged);
  }

  private void HandleValueChanged(string newValue) {
    //DAS.Info("Single Stat " + _StatEnum, "HandleValueChanged: " + newValue);
    Robot robot = RobotEngineManager.Instance.CurrentRobot;
    uint newStat;

    if (robot != null && uint.TryParse(newValue, out newStat)) {
      robot.SetProgressionStat(_StatEnum, newStat);
    }
  }
}
