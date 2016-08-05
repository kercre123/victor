using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class RobotRestorePane : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Button _RestoreButton;

  [SerializeField]
  private UnityEngine.UI.Button _WipeButton;

  [SerializeField]
  private UnityEngine.UI.Dropdown _BackupDropdown;

  private void Start() {
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RestoreRobotOptions>(HandleRestoreRobotOptions);
    _RestoreButton.onClick.AddListener(OnHandleRestoreButtonClicked);
    _WipeButton.onClick.AddListener(OnHandleWipeButtonClicked);

    RobotEngineManager.Instance.CurrentRobot.RequestRobotRestoreData();
  }

  private void HandleRestoreRobotOptions(Anki.Cozmo.ExternalInterface.RestoreRobotOptions message) {
    _BackupDropdown.ClearOptions();

    // Adds zero as the first option in order to support being able to have engine automatically select which 
    // restore file to use
    List<UnityEngine.UI.Dropdown.OptionData> options = new List<UnityEngine.UI.Dropdown.OptionData>();
    UnityEngine.UI.Dropdown.OptionData option = new UnityEngine.UI.Dropdown.OptionData();
    option.text = "0";
    options.Add(option);

    int i = 0;
    while (message.robotsWithBackupData[i] != 0) {
      option = new UnityEngine.UI.Dropdown.OptionData();
      option.text = message.robotsWithBackupData[i].ToString();
      options.Add(option);
      ++i;
    }
	
    _BackupDropdown.AddOptions(options);
  }

  private void OnHandleRestoreButtonClicked() {
    RobotEngineManager.Instance.CurrentRobot.RestoreRobotFromBackup(uint.Parse(_BackupDropdown.options[_BackupDropdown.value].text));
  }

  private void OnHandleWipeButtonClicked() {
    RobotEngineManager.Instance.CurrentRobot.WipeRobotGameData();

    // Setting this flag back to true since the robot will report it hasn't been through onboarding due to the tag being erased
    // which will cause it not to save backups so we need to go back through the first time flow to set the onboarding tag on the robot
    DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow = true;

    DataPersistence.DataPersistenceManager.Instance.Save();
  }
}
