using UnityEngine;
using System.Collections;

public class PingStatus : MonoBehaviour {

  private UnityEngine.Ping _Ping;
  private bool _PingSuccess = false;

  public bool GetPingStatus() {
    return _PingSuccess;
  }

  // Use this for initialization
  void Start() {
    SendPing();
  }
	
  // Update is called once per frame
  void Update() {
    if (_Ping.isDone) {
      if (_Ping.time >= 0) {
        _PingSuccess = true;
      }
      else {
        _PingSuccess = false;
      }
      SendPing();
    }
  }

  private void SendPing() {
    _Ping = new Ping("172.31.1.1");
  }
}
