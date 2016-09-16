using System;
using System.Collections.Generic;

public class DasTracker {

  private DateTime _LastBackgroundUtcTime;
  private DateTime _LastResumeUtcTime;
  private DateTime _LastTimerUpdateUtcTime;
  private DateTime _CurrentSessionStartUtcTime;
  private DateTime _CurrentRobotStartUtcTime;
  private DateTime _AppStartupUtcTime;
  private DateTime _ConnectFlowStartUtcTime;
  private DateTime _WifiFlowStartUtcTime;
  private bool _FirstTimeConnectActive;
  private bool _ConnectSessionIsFirstTime;
  private double _RunningApprunTime;
  private double _RunningSessionTime;
  private double? _RunningRobotTime = null;

  public void OnAppBackgrounded() {
    UpdateRunningTimers();
    _LastBackgroundUtcTime = DateTime.UtcNow;

    // app.entered_background - no extra data
    DAS.Event("app.entered_background", "");

    HandleSessionEnd();
  }

  public void OnAppResumed() {
    _LastResumeUtcTime = DateTime.UtcNow;
    _LastTimerUpdateUtcTime = DateTime.UtcNow;
    {
      // app.became_active - include timestamp of last backgrounding (ms since epoch)
      var epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
      DAS.Event("app.became_active", Convert.ToUInt64((_LastBackgroundUtcTime - epoch).TotalMilliseconds).ToString());
    }

    HandleSessionStart();

    if ((DateTime.UtcNow - _LastBackgroundUtcTime).TotalMinutes < 10.0) {
      // app.session.resume - if backgrounding lasted <10 minutes
      DAS.Event("app.session.resume", "");
    }
  }

  public void OnAppStartup() {
    _AppStartupUtcTime = DateTime.UtcNow;
    _LastTimerUpdateUtcTime = DateTime.UtcNow;
    HandleSessionStart();
  }

  public void OnAppQuit() {
    UpdateRunningTimers();
    if (_LastBackgroundUtcTime > _AppStartupUtcTime && _LastBackgroundUtcTime > _LastResumeUtcTime) {
      // if app has been backgrounded more recently than resumed, session has already ended
      HandleSessionEnd();
    }

    // app.terminated - no extra data
    DAS.Event("app.terminated", "");
    // app.apprun.length - time spent in app not including backgrounding
    DAS.Event("app.apprun.length", Convert.ToUInt32(_RunningApprunTime).ToString());
  }

  public void OnRobotConnected() {
    UpdateRunningTimers(); // otherwise pre-connect time will get included in running robot timer
    _RunningRobotTime = 0.0;
    _CurrentRobotStartUtcTime = DateTime.UtcNow;

    // app.connected_session.start - no extra data
    DAS.Event("app.connected_session.start", "");
  }

  public void OnRobotDisconnected() {
    if (_RunningRobotTime == null) {
      return;
    }
    UpdateRunningTimers();
    uint totalConnectedTime = Convert.ToUInt32((DateTime.UtcNow - _CurrentRobotStartUtcTime).TotalSeconds);
    var dataDict = GetDataDictionary("$data", totalConnectedTime.ToString());

    // app.connected_session.end - include:
    //   - time spent in session (w/o backgrounding)
    //   - $data = time spent in session including backgrounding
    DAS.Event("app.connected_session.end", Convert.ToUInt32(_RunningRobotTime).ToString(), null, dataDict);

    _RunningRobotTime = null;
  }

  public void OnFirstTimeConnectStarted() {
    _FirstTimeConnectActive = true;
  }

  public void OnFirstTimeConnectEnded() {
    _FirstTimeConnectActive = false;
  }

  public void OnConnectFlowStarted() {
    // app.connect.start - 0/1 if in out of box experience
    DAS.Event("app.connect.start", _FirstTimeConnectActive ? "1" : "0");

    _ConnectFlowStartUtcTime = DateTime.UtcNow;
    _ConnectSessionIsFirstTime = _FirstTimeConnectActive; // in case this is reset before OnFlowEnded()
  }

  public void OnConnectFlowEnded() {
    // we're leaving connect flow - do we have a robot connection active?
    if (_RunningRobotTime != null) {
      HandleConnectSuccess();
    }

    {
      var dataDict = GetDataDictionary("$data", _ConnectSessionIsFirstTime ? "1" : "0");
      uint secondsInFlow = Convert.ToUInt32((DateTime.UtcNow - _ConnectFlowStartUtcTime).TotalSeconds);

      // app.connect.exit - include:
      //   - total time in connect flow (seconds)
      //   - 0/1 if in out of box experience
      DAS.Event("app.connect.exit", secondsInFlow.ToString(), null, dataDict);
    }
  }

  public void OnSearchForCozmoFailed() {
    var dataDict = GetDataDictionary("$data", _ConnectSessionIsFirstTime ? "1" : "0");
    uint secondsInFlow = Convert.ToUInt32((DateTime.UtcNow - _ConnectFlowStartUtcTime).TotalSeconds);

    // app.connect.fail - include:
    //   - total time in connect flow (seconds)
    //   - 0/1 if in out of box experience
    DAS.Event("app.connect.fail", secondsInFlow.ToString(), null, dataDict);
  }

  public void OnWifiInstructionsStarted() {
    // app.connect.wifi.start - 0/1 if in out of box experience
    DAS.Event("app.connect.wifi.start", _ConnectSessionIsFirstTime ? "1" : "0");

    _WifiFlowStartUtcTime = DateTime.UtcNow;
  }

  public void OnWifiInstructionsEnded() {
    var dataDict = GetDataDictionary("$data", _ConnectSessionIsFirstTime ? "1" : "0");
    uint secondsInFlow = Convert.ToUInt32((DateTime.UtcNow - _WifiFlowStartUtcTime).TotalSeconds);

    // app.connect.wifi.complete - include:
    //   - total time in wifi setup portion of flow
    //   - 0/1 if in out of box experience
    DAS.Event("app.connect.wifi.complete", secondsInFlow.ToString(), null, dataDict);
  }

  public void OnWifiInstructionsGetHelp() {
    // app.connect.wifi.get_help - no extra data
    DAS.Event("app.connect.wifi.get_help", "");
  }

  private void HandleConnectSuccess() {
    // connect flow ends when connection is successfully made
    var dataDict = GetDataDictionary("$data", _ConnectSessionIsFirstTime ? "1" : "0");
    uint secondsInFlow = Convert.ToUInt32((DateTime.UtcNow - _ConnectFlowStartUtcTime).TotalSeconds);

    // app.connect.success - include:
    //   - total time in connect flow (seconds)
    //   - 0/1 if in out of box experience
    DAS.Event("app.connect.success", secondsInFlow.ToString(), null, dataDict);
  }

  private void HandleSessionStart() {
    // app.session.start - no extra data
    DAS.Event("app.session.start", "");

    _CurrentSessionStartUtcTime = DateTime.UtcNow;
    _RunningSessionTime = 0.0;
  }

  private void HandleSessionEnd() {
    UpdateRunningTimers();
    uint sessionSeconds = Convert.ToUInt32((DateTime.UtcNow - _CurrentSessionStartUtcTime).TotalSeconds);
    var dataDict = GetDataDictionary("$data", sessionSeconds.ToString());

    // app.session.end - include:
    //   - time spent in session (w/o backgrounding)
    //   - $data = time spent in session including backgrounding
    DAS.Event("app.session.end", Convert.ToUInt32(_RunningSessionTime).ToString(), null, dataDict);

    _RunningSessionTime = 0.0;
  }

  private void UpdateRunningTimers() {
    double secondsSinceLastUpdate = (DateTime.UtcNow - _LastTimerUpdateUtcTime).TotalSeconds;
    _LastTimerUpdateUtcTime = DateTime.UtcNow; // prevents potential double counting

    _RunningApprunTime += secondsSinceLastUpdate;
    _RunningSessionTime += secondsSinceLastUpdate;
    if (_RunningRobotTime != null) {
      _RunningRobotTime += secondsSinceLastUpdate;
    }
  }

  public static Dictionary<string, string> GetDataDictionary(params string[] list) {
    var ret = new Dictionary<string, string>();
    if (list.Length % 2 != 0) {
      throw new ArgumentOutOfRangeException("count", "must send pairs of values");
    }
    for (int i = 0; i + 1 < list.Length; i += 2) {
      ret.Add(list[i], list[i + 1]);
    }
    return ret;
  }

  private static DasTracker _Instance;
  public static DasTracker Instance {
    get {
      if (_Instance == null) {
        _Instance = new DasTracker();
      }
      return _Instance;
    }
  }
}
