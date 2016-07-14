using UnityEngine;
using System.Collections;

public class PingStatus : MonoBehaviour {

  private const float kWaitBetweenSuccesfulPings = 3.0f;
  private const float kWaitBetweenFailedPings = 2.0f;
  private const float kKillPingTime = 10.0f;
  private const float kLogTime = 2.0f;

  private UnityEngine.Ping _Ping;
  private Coroutine _coroutine;
  private bool _PingCompleted = true;
  private bool _PingSuccessful = false;
  //  private float _LastLogTime = 0.0f;
  private float _LastPingTime = 0.0f;

  public bool GetPingStatus() {
    return _PingSuccessful;
  }

  void Start() {
    _coroutine = StartCoroutine(PingCoroutine());
  }

  void OnDestroy() {
    if (_coroutine != null) {
      DAS.Debug(this, "Stopping ping coroutine");
      StopCoroutine(_coroutine);
      _coroutine = null;
    }
  }

  void Update() {
    if (_PingCompleted) {
      if (!_PingSuccessful) {
        if (Time.time - _LastPingTime > kWaitBetweenFailedPings) {
          _coroutine = StartCoroutine(PingCoroutine());
          _LastPingTime = Time.time;
        }
      }
      else {
        if (Time.time - _LastPingTime > kWaitBetweenSuccesfulPings) {
          _coroutine = StartCoroutine(PingCoroutine());
          _LastPingTime = Time.time;
        }
      }
    }
  }

  private IEnumerator PingCoroutine() {
    //    DAS.Debug(this, "Pinging " + RobotEngineManager.kRobotIP);
    //    _Ping = new Ping(RobotEngineManager.kRobotIP);
    //    _PingCompleted = false;
    //
    //    float startTime = Time.time;
    //    _LastLogTime = Time.time;
    //
    //    // Cancel the ping if it is not comleted in a certain amount of time to make sure it doesn't keep running forever
    //    while (!_Ping.isDone && ((Time.time - startTime) < kKillPingTime)) {
    //      if ((Time.time - _LastLogTime) > kLogTime) {
    //        DAS.Debug(this, "Still pinging " + RobotEngineManager.kRobotIP);
    //        _LastLogTime = Time.time;
    //      }
    //      yield return null;
    //    }
    //
    //    if (_Ping.isDone) {
    //      DAS.Debug(this, "Ping completed. Time is = " + _Ping.time);
    //    } else {
    //      DAS.Debug(this, "Ping failed");
    //    }
    //
    //    _PingSuccessful = _Ping.time > 0;
    //    _Ping.DestroyPing();
    //    _Ping = null;
    //    _PingCompleted = true;

    // Disabling using Ping since it is leaking threads on device which
    // is causing multiple issues especially in the factory test app.
    // See https://fogbugz.unity3d.com/default.asp?804440_vdqj6tngkifda521
    // While we wait for this to be fixed in Unity, we have decided to use the
    // WiFi address as an indication of whether we are connected to the right 
    // network.
    System.Net.IPAddress ipAddress = RobotUdpChannel.GetLocalIPv4();
    _PingSuccessful = (ipAddress != null) && ipAddress.ToString().StartsWith("172.31");
    _PingCompleted = true;
    yield break;
  }
}
