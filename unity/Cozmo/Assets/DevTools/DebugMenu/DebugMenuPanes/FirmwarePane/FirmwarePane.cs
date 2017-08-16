using Anki.Cozmo;
using UnityEngine;
using UnityEngine.UI;
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Text.RegularExpressions;

public class FirmwarePane : MonoBehaviour {

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _OutputText;
  [SerializeField]
  private Anki.UI.AnkiButtonLegacy _UpgradeButton;
  [SerializeField]
  private Anki.UI.AnkiButtonLegacy _PrepareButton;

  [SerializeField]
  private Dropdown _FirmwareDropdown;
  private const string _kFWSelectMessage = "Select Firmware to Load";
  private List<int> _FirmwareVersions = new List<int>();


  public static string GetFirmwareDir() {
#if UNITY_EDITOR
    return Application.dataPath + "/../../../resources/config/basestation";
#elif UNITY_IOS || UNITY_ANDROID
    return PlatformUtil.GetResourcesFolder() + "/config/basestation";
#else
    return null;
#endif
  }


  void Awake() {
    _UpgradeButton.Initialize(() => RobotEngineManager.Instance.UpdateFirmware(FirmwareType.Current, 0),
                              "debug_upgrade_firmware_button",
                              "debug_firmware_view");
    _PrepareButton.Initialize (() => RobotEngineManager.Instance.TurnOffReactionaryBehavior(),
                               "debug_disable_reactionary_behavior_button",
                               "debug_disable_reactionary_behavior_view");

    _FirmwareDropdown.onValueChanged.AddListener(HandleFirmwareSelected);

    if (RobotEngineManager.Instance != null) {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(OnFirmwareUpdateProgress);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(OnFirmwareUpdateComplete);
    }
  }

  void Start() {
    Regex versionOnly = new Regex(".*firmware_");

    _FirmwareDropdown.options.Clear();
    //update dropdown of recordings
    Dropdown.OptionData fileMessage = new Dropdown.OptionData();
    fileMessage.text = _kFWSelectMessage;
    _FirmwareVersions.Add(0);
    _FirmwareDropdown.options.Add(fileMessage);
    foreach (string file in Directory.GetDirectories(GetFirmwareDir(), "firmware_*")) {
      Dropdown.OptionData TextOptionData = new Dropdown.OptionData();
      string version = versionOnly.Replace(file, String.Empty);
      TextOptionData.text = version;
      _FirmwareVersions.Add(Int32.Parse(version));
      _FirmwareDropdown.options.Add(TextOptionData);
    }
    _FirmwareDropdown.RefreshShownValue();

  }

  public void HandleFirmwareSelected(int i) {
    if (_FirmwareDropdown.captionText.text.Equals(_kFWSelectMessage)) {
      return;
    }

    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.UpdateFirmware(FirmwareType.Old, _FirmwareVersions[i]);
    }
  }

  public void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress>(OnFirmwareUpdateProgress);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete>(OnFirmwareUpdateComplete);
  }


  private void OnFirmwareUpdateProgress(Anki.Cozmo.ExternalInterface.FirmwareUpdateProgress message) {
    _OutputText.text = "InProgress: Robot " + message.robotID + " Stage: " + message.stage + ":" + message.subStage + " " + message.percentComplete + "%"
    + "\nFwSig = " + message.fwSig;
  }

  private void OnFirmwareUpdateComplete(Anki.Cozmo.ExternalInterface.FirmwareUpdateComplete message) {
    _OutputText.text = "Complete: Robot " + message.robotID + " Result: " + message.result
    + "\nFwSig = " + message.fwSig;
  }


}
