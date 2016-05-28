using UnityEngine;
using System.Collections;

public class PingStatus : MonoBehaviour {

  private UnityEngine.Ping _Ping;
  private bool _PingSuccess = true;
  private float _LastPingTime = 0.0f;

  public bool GetPingStatus() {
    return _PingSuccess;
  }

  // Use this for initialization
  void Start() {
    //SendPing();
  }
	
  // Update is called once per frame
  void Update() {
    /*if (Time.time - _LastPingTime > 2.0f) {
      // it's been 2 seconds since we've heard anything. try again.
      // this handles the edge case of Ping throwing a routing exception
      // unity is dumb and doesn't mark isDone as true if it throws an exception so there's
      // no other way for us to tell
      SendPing();
    }
    if (_Ping.isDone) {
      if (_Ping.time >= 0) {
        _PingSuccess = true;
      }
      else {
        _PingSuccess = false;
      }
      try {
        SendPing();
      }
      catch {
        _PingSuccess = false;
        SendPing();
      }

    }*/
  }

  private void SendPing() {
    _LastPingTime = Time.time;
    _Ping = new Ping(RobotEngineManager.kRobotIP);
  }
}
