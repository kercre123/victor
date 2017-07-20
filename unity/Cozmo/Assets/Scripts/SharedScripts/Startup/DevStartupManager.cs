using UnityEngine;
using System.Collections;
using System;

public class DevStartupManager : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Button _RobotButton;

  [SerializeField]
  private UnityEngine.UI.Button _SimButton;

  [SerializeField]
  private UnityEngine.UI.Button _MockButton;

  [SerializeField]
  private StartupManager _StartupManager;

  private void Start() {

#if !UNITY_EDITOR
    GameObject.Destroy(gameObject);
    RobotEngineManager.Instance.RobotConnectionType = RobotEngineManager.ConnectionType.Robot;
    _StartupManager.StartLoadAsync();
#endif
#if UNITY_EDITOR
    if(GetFlag("-smoke")) {
      RobotEngineManager.Instance.RobotConnectionType = RobotEngineManager.ConnectionType.Mock;
      _StartupManager.StartLoadAsync();
    }

    _RobotButton.onClick.AddListener(() => {
      RobotEngineManager.Instance.RobotConnectionType = RobotEngineManager.ConnectionType.Robot;
      GameObject.Destroy(gameObject);
      _StartupManager.StartLoadAsync();
    });

    _SimButton.onClick.AddListener(() => {
      RobotEngineManager.Instance.RobotConnectionType = RobotEngineManager.ConnectionType.Sim;
      GameObject.Destroy(gameObject);
      _StartupManager.StartLoadAsync();
    });

    _MockButton.onClick.AddListener(() => {
      RobotEngineManager.Instance.RobotConnectionType = RobotEngineManager.ConnectionType.Mock;
      GameObject.Destroy(gameObject);
      _StartupManager.StartLoadAsync();
    });
#endif    
  }

  private static bool GetFlag(string name) {
      var args = System.Environment.GetCommandLineArgs();
      if (args==null){
        return false;
      }
      foreach (String item in args) {
        if (item == name) {
          return true;
        }
      }
      return false;
  }

}
