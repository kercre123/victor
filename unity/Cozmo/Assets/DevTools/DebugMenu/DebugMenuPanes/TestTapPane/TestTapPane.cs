using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;

public class TestTapPane : MonoBehaviour {

  [SerializeField]
  private Button _IntensityButton;

  [SerializeField]
  private InputField _IntensityInput;

  [SerializeField]
  private Toggle _FilterEnabledCheckbox;

  // Shortcut for 
  //CONSOLE_VAR(int16_t, TapIntensityMin, "TapFilter.IntesityMin", 55);
  private const string _kTapIntensityMinKey = "TapIntensityMin";

  // CubeID -> TimesTapped
  private Dictionary<int, int> _CubeTappedAmount = new Dictionary<int, int>();

  private Color[] _TestColors = new Color[] { Color.black, Color.red, Color.green, Color.blue };

  // Use this for initialization
  void Start() {
    _IntensityButton.onClick.AddListener(OnIntensityButton);

    _FilterEnabledCheckbox.onValueChanged.AddListener(HandleEnableChanged);

    IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
    // Turn off the light components
    if (CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayLightStates(false);

      // Listen to light changes
      LightCube.TappedAction += OnBlockTapped;

      //  DebugConsole Intensity changes...
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleInitIntensity);
      RobotEngineManager.Instance.Message.GetDebugConsoleVarMessage = new Anki.Cozmo.ExternalInterface.GetDebugConsoleVarMessage(_kTapIntensityMinKey);
      RobotEngineManager.Instance.SendMessage();

      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.BlockTapFilterStatus>(HandleInitEnabled);
      RobotEngineManager.Instance.Message.GetBlockTapFilterStatus = new Anki.Cozmo.ExternalInterface.GetBlockTapFilterStatus();
      RobotEngineManager.Instance.SendMessage();

      foreach (KeyValuePair<int, LightCube> lightCube in CurrentRobot.LightCubes) {
        CurrentRobot.LightCubes[lightCube.Key].SetLEDs(Color.yellow);
        _CubeTappedAmount.Add(lightCube.Key, 0);
      }
    }
  }

  void OnDestroy() {
    IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
    if (CurrentRobot != null) {
      CurrentRobot.SetEnableFreeplayLightStates(true);

      foreach (KeyValuePair<int, LightCube> lightCube in CurrentRobot.LightCubes) {
        CurrentRobot.LightCubes[lightCube.Key].SetLEDs(Color.black);
      }
    }

    LightCube.TappedAction -= OnBlockTapped;

    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.BlockTapFilterStatus>(HandleInitEnabled);
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage>(HandleInitIntensity);
  }

  private void HandleInitIntensity(Anki.Cozmo.ExternalInterface.VerifyDebugConsoleVarMessage message) {
    if (message.varName == _kTapIntensityMinKey && message.success) {
      _IntensityInput.text = message.varValue.varInt.ToString();
    }
  }

  private void OnBlockTapped(int id, int times, float timeStamp) {
    // Every tap we go between Black, R, G, B
    IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
    if (CurrentRobot != null) {
      if (_CubeTappedAmount.ContainsKey(id)) {
        _CubeTappedAmount[id]++;
      }
      else {
        _CubeTappedAmount.Add(id, 1);
        DAS.Info("TestTapPane.OnBlockTapped.NewID", "New ID " + id);
      }
      DAS.Info("TestTapPane.OnBlockTapped.Info", id + " @ " + timeStamp + " unityT: " + Time.time);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Placeholder);
      CurrentRobot.LightCubes[id].SetLEDs(_TestColors[_CubeTappedAmount[id] % _TestColors.Length]);
    }
  }


  private void OnIntensityButton() {
    RobotEngineManager.Instance.SetDebugConsoleVar(_kTapIntensityMinKey, _IntensityInput.text);
  }

  private void HandleInitEnabled(Anki.Cozmo.ExternalInterface.BlockTapFilterStatus msg) {
    _FilterEnabledCheckbox.isOn = msg.enabled;
  }
  private void HandleEnableChanged(bool isOn) {
    RobotEngineManager.Instance.Message.EnableBlockTapFilter = Singleton<Anki.Cozmo.ExternalInterface.EnableBlockTapFilter>.Instance.Initialize(isOn);
    RobotEngineManager.Instance.SendMessage();
  }

}
