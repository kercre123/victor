using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class RobotRestorePane : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Button _RestoreButton;

  [SerializeField]
  private UnityEngine.UI.Button _WipeButton;

  [SerializeField]
  private UnityEngine.UI.Button _WipeAndResetFirmwareButton;

  [SerializeField]
  private UnityEngine.UI.Dropdown _BackupDropdown;

  [SerializeField]
  private UnityEngine.UI.Text _LblStatus;

  private void Start() {
    RobotEngineManager rem = RobotEngineManager.Instance;
    if (rem != null) {
      rem.AddCallback<Anki.Cozmo.ExternalInterface.RestoreRobotOptions>(HandleRestoreRobotOptions);
      rem.AddCallback<Anki.Cozmo.ExternalInterface.RestoreRobotStatus>(HandleRestoreStatus);
      _RestoreButton.onClick.AddListener(OnHandleRestoreButtonClicked);
      _WipeButton.onClick.AddListener(OnHandleWipeButtonClicked);
      _WipeAndResetFirmwareButton.onClick.AddListener(OnHandleWipeAndResetFirmwareButtonClicked);
      if (rem.CurrentRobot != null) {
        rem.CurrentRobot.RequestRobotRestoreData();
        _LblStatus.text = "";
      }
      else {
        SetStatusText("Error: Disconnected from robot! Can't set anything", Color.red);
      }
    }
  }
  private void OnDestroy() {
    RobotEngineManager rem = RobotEngineManager.Instance;
    if (rem != null) {
      rem.RemoveCallback<Anki.Cozmo.ExternalInterface.RestoreRobotOptions>(HandleRestoreRobotOptions);
      rem.RemoveCallback<Anki.Cozmo.ExternalInterface.RestoreRobotStatus>(HandleRestoreStatus);
    }
  }

  private void HandleRestoreRobotOptions(Anki.Cozmo.ExternalInterface.RestoreRobotOptions message) {
    _BackupDropdown.ClearOptions();

    // Adds zero as the first option in order to support being able to have engine automatically select which 
    // restore file to use
    List<UnityEngine.UI.Dropdown.OptionData> options = new List<UnityEngine.UI.Dropdown.OptionData>();
    UnityEngine.UI.Dropdown.OptionData option = new UnityEngine.UI.Dropdown.OptionData();
    option.text = "0";
    options.Add(option);

    foreach (uint i in message.robotsWithBackupData) {
      option = new UnityEngine.UI.Dropdown.OptionData();
      option.text = i.ToString();
      options.Add(option);
    }

    _BackupDropdown.AddOptions(options);
  }

  private void HandleRestoreStatus(Anki.Cozmo.ExternalInterface.RestoreRobotStatus status) {
    if (status.isWipe) {
      if (status.success) {
        SetStatusText("Success! Robot Data cleared", Color.green);
      }
      else {
        SetStatusText("Error: Robot Data clear failed!", Color.red);
      }
    }
  }

  private void SetStatusText(string txt, Color color) {
    _LblStatus.text = txt;
    _LblStatus.color = color;
  }

  private void OnHandleRestoreButtonClicked() {
    RobotEngineManager.Instance.CurrentRobot.RestoreRobotFromBackup(uint.Parse(_BackupDropdown.options[_BackupDropdown.value].text));
  }

  private void OnHandleWipeButtonClicked() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      SetStatusText("Waiting... ", Color.blue);
      RobotEngineManager.Instance.CurrentRobot.WipeRobotGameData();
      // Setting this flag back to true since the robot will report it hasn't been through onboarding due to the tag being erased
      // which will cause it not to save backups so we need to go back through the first time flow to set the onboarding tag on the robot
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.FirstTimeUserFlow = true;
      DataPersistence.DataPersistenceManager.Instance.Save();
    }
    else {
      SetStatusText("Error: Tried to wipe when disconnected, not cleared", Color.red);
    }
  }

  private void OnHandleWipeAndResetFirmwareButtonClicked() {
    OnHandleWipeButtonClicked();
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.ResetFirmware();
    }
  }
}
