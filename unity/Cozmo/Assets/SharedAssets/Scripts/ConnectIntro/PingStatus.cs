using UnityEngine;
using System.Collections;

public class PingStatus : MonoBehaviour {

  private const float kWaitBetweenSuccesfulPings = 1.0f;
  private const float kWaitBetweenFailedPings = 1.0f;
  private const float kKillPingTime = 10.0f;
  private const float kLogTime = 2.0f;

  private UnityEngine.Ping _Ping;
  private Coroutine _coroutine;
  private bool _PingCompleted = true;
  private bool _PingSuccessful = false;
  private bool _NetworkCheckSuccessful = false;
  private float _LastLogTime = 0.0f;
  private float _LastPingTime = 0.0f;

  public bool GetPingStatus() {
    return _PingSuccessful;
  }

  public bool GetNetworkStatus() {
    return _NetworkCheckSuccessful;
  }

  void Start() {
    _coroutine = StartCoroutine(PingCoroutine());
  }

  void OnDestroy() {
    if (_coroutine != null) {
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
    // Network check first
    System.Net.IPAddress ipAddress = RobotUdpChannel.GetLocalIPv4();
    _NetworkCheckSuccessful = (ipAddress != null) && ipAddress.ToString().StartsWith("172.31");
    if (!_NetworkCheckSuccessful) {
      _PingSuccessful = false;
      _PingCompleted = true;
      _coroutine = null;
      yield break;
    }

    // Real ping now if we pass the network check
    _Ping = new Ping(RobotEngineManager.kRobotIP);
    _PingCompleted = false;

    float startTime = Time.time;
    _LastLogTime = Time.time;

    // Cancel the ping if it is not completed in a certain amount of time to make sure it doesn't keep running forever
    while (!_Ping.isDone && ((Time.time - startTime) < kKillPingTime)) {
      if ((Time.time - _LastLogTime) > kLogTime) {
        _LastLogTime = Time.time;
      }
      yield return null;
    }

    _PingSuccessful = _Ping.time > 0;
    _Ping.DestroyPing();
    _Ping = null;
    _PingCompleted = true;
    _coroutine = null;
  }
}
