using UnityEngine;
using Anki.Debug;


public class ExceptionReportManager : MonoBehaviour {
  private void HandleDebugConsoleCrashFromUnityButton(string str) {
    DAS.Event("ExceptionReportManager.ForceDebugCrash", "ExceptionReportManager.ForceDebugCrash");
    // Apparently dividing by 0 only forces an exception on mac not iOS. So just throw.
    if (str != "exception") {
      throw new UnityException("ForcedExceptionTest");
    }
    DAS.Info("test.crash", "test.crash");
  }

  void OnEnable() {
    System.AppDomain.CurrentDomain.UnhandledException += OnHandleUnresolvedException;
    Application.logMessageReceived += OnHandleLogCallback;
    DebugConsoleData.Instance.AddConsoleFunction("Unity Exception", "Debug", HandleDebugConsoleCrashFromUnityButton);
  }

  void OnDisable() {
    System.AppDomain.CurrentDomain.UnhandledException -= OnHandleUnresolvedException;
    Application.logMessageReceived -= OnHandleLogCallback;
  }

  /// <summary>
  /// Handle a single exception, log to DAS immediately
  /// </summary>
  /// <param name="logString">A string that contains the reason for the exception.</param>
  /// <param name="stackTrace">The stacktrace for the exception.</param>
  private void HandleException(string logString, string stackTrace) {
    // Write the full log to be uploaded to DAS, and will show in print logger
    DAS.Error("unity.exception", logString, null, DASUtil.FormatExtraData(stackTrace));
  }

  /// <summary>
  /// Callback for handling log messages.
  /// </summary>
  /// <param name="logString">A string that contains the reason for the exception.</param>
  /// <param name="stackTrace">The stacktrace for the exception.</param>
  /// <param name="type">The type of the log message.</param>
  private void OnHandleLogCallback(string logString, string stackTrace, LogType type) {
    if (LogType.Assert == type || LogType.Exception == type || LogType.Error == type) {
      HandleException(logString, stackTrace);
    }
  }

  private void OnHandleUnresolvedException(object sender, System.UnhandledExceptionEventArgs args) {
    if (args == null || args.ExceptionObject == null) {
      return;
    }

    if (args.ExceptionObject.GetType() == typeof(System.Exception)) {
      System.Exception e = (System.Exception)args.ExceptionObject;
      HandleException(e.Source, e.StackTrace);
    }
  }
}
