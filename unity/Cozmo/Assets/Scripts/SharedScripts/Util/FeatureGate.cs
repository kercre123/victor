using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;

public class FeatureGate {
  private static FeatureGate _sInstance;

  private Dictionary<string, bool> _FeatureMap = new Dictionary<string, bool>();

  public Dictionary<string, bool> FeatureMap { get { return _FeatureMap; } }

  private bool _ReceivedData = false;

  public void Initialize() {
    // message engine and ask for our data
    RobotEngineManager.Instance.ConnectedToClient += (s) => RequestData();
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FeatureToggles>(HandleFeatureToggles);
  }

  private void RequestData() {
    _ReceivedData = false;
    _FeatureMap.Clear();
    RobotEngineManager.Instance.Message.RequestFeatureToggles = new RequestFeatureToggles();
    RobotEngineManager.Instance.SendMessage();
  }

  public bool IsFeatureEnabled(FeatureType feature) {
    return IsFeatureEnabled(feature.ToString());
  }

  public bool IsFeatureEnabled(string feature) {
    if (!_ReceivedData) {
      DAS.Error("FeatureGate.IsFeatureEnabled", "requesting feature flag when we haven't received data yet!");
      return false;
    }

    bool result;
    if (_FeatureMap.TryGetValue(feature.ToLower(), out result)) {
      return result;
    }
    else {
      return false;
    }
  }

  public void SetFeatureEnabled(FeatureType feature, bool enabled) {
    if (!_ReceivedData) {
      DAS.Error("FeatureGate.SetFeatureEnabled", "requesting feature flag when we haven't received data yet!");
    }

    _FeatureMap[feature.ToString().ToLower()] = enabled;
    RobotEngineManager.Instance.Message.SetFeatureToggle = new SetFeatureToggle(feature, enabled);
    RobotEngineManager.Instance.SendMessage();
  }

  private void HandleFeatureToggles(FeatureToggles featureToggles) {
    string[] enumNames = System.Enum.GetNames(typeof(FeatureType));
    for (int i = 0; i < enumNames.Length; i++) {
      enumNames[i] = enumNames[i].ToLower();
    }

    string resultString = "";
    string eventString = "";
    int numEnabledEvents = 0;
    foreach (FeatureToggle toggle in featureToggles.features) {
      _FeatureMap[toggle.name] = toggle.enabled;
      resultString += "{ " + toggle.name + ": " + toggle.enabled + " }";

      // add to das event string if enabled
      if (toggle.enabled) {
        if (numEnabledEvents > 0) {
          eventString += ",";
        }
        eventString += toggle.name;
        numEnabledEvents++;
      }

      // verify that each feature we have data for matches a name in our enum
      bool foundMatch = false;
      foreach (string validName in enumNames) {
        if (toggle.name.Equals(validName)) {
          foundMatch = true;
          break;
        }
      }
      if (!foundMatch) {
        DAS.Error("FeatureGate.HandleFeatureToggles", "received feature name " + toggle.name + " that is unknown to us");
      }
    }
    DAS.Info("FeatureGate.ReceivedFeatureToggles", resultString);
    DAS.Event("app.features_enabled", eventString);
    _ReceivedData = true;
  }

  public static FeatureGate Instance {
    get {
      if (_sInstance == null) {
        _sInstance = new FeatureGate();
      }
      return _sInstance;
    }
  }

}
