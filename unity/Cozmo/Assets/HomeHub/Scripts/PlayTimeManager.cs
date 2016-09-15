using UnityEngine;
using System.Collections;

public class PlayTimeManager : MonoBehaviour {

  private static PlayTimeManager _Instance;

  public static PlayTimeManager Instance {
    get {
      return _Instance;
    }
  }

  private float _LastStartedTime;

  private void Awake() {
    _Instance = this;
  }

  private bool _RobotConnected = false;

  public void RobotConnected(bool connected) {
    _RobotConnected = connected;
    if (connected) {
      _LastStartedTime = Time.time;
    }
    else {
      ComputeAndSetPlayTime();
    }
  }

  public void ComputeAndSetPlayTime() {
    if (DataPersistence.DataPersistenceManager.Instance.CurrentSession != null) {
      DataPersistence.DataPersistenceManager.Instance.CurrentSession.PlayTime += (Time.time - _LastStartedTime);
    }
    DataPersistence.DataPersistenceManager.Instance.Save();
    _LastStartedTime = Time.time;
  }

  private void OnApplicationPause(bool isPaused) {
    if (isPaused && _RobotConnected) {
      // we are pausing and robot is still connected
      ComputeAndSetPlayTime();
    }

    if (!isPaused && _RobotConnected) {
      _LastStartedTime = Time.time;
    }
  }

}
