using UnityEngine;
using System.Collections;
using System.IO;
using Anki.Debug;


public class HockeyAppManager : MonoBehaviour {

  protected const string HOCKEYAPP_BASEURL = "https://rink.hockeyapp.net/";
  protected const string HOCKEYAPP_CRASHESPATH = "api/2/apps/[APPID]/crashes/upload";
  protected const string LOG_FILE_DIR = "/unity_crash_logs/";
  protected const int MAX_CHARS = 199800;

  void HandleDebugConsoleCrashFromUnityButton(System.Object setvar) {
    DAS.Event("HockeAppManager.ForceDebugCrash", "HockeAppManager.ForceDebugCrash");
    if (setvar.ToString() != "exception") {
      throw new UnityException("ForcedExceptionTest");
    }
    DAS.Info("test.crash.impossible", "test.crash.impossible");
  }

  void Awake() {
    DAS.Info("HockeAppManager.Awake", "HockeAppManager.Awake");
    #if (UNITY_IPHONE && !UNITY_EDITOR)
   
  // re-enable.
    /*if(exceptionLogging == true && IsConnected() == true)
    {
      List<string> logFileDirs = GetLogFiles();
      if ( logFileDirs.Count > 0)
      {
        Debug.Log("Found files: " + logFileDirs.Count);
        StartCoroutine(SendLogs(logFileDirs));
      }
    }*/
    #endif
  }

  void OnEnable() {
    
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    System.AppDomain.CurrentDomain.UnhandledException += OnHandleUnresolvedException;
    Application.logMessageReceived += OnHandleLogCallback;
    #endif

    // Crashing is useful in all platforms.
    DAS.Info("HockeAppManager.OnEnable", "HockeAppManager.OnEnable");
    Anki.Cozmo.ExternalInterface.DebugConsoleVar consoleVar = new Anki.Cozmo.ExternalInterface.DebugConsoleVar();
    consoleVar.category = "Crash";
    consoleVar.varName = "Crash From Unity";
    consoleVar.varValue.varFunction = "CrashFromUnityFunc";
    DebugConsoleData.Instance.AddConsoleVar(consoleVar, this.HandleDebugConsoleCrashFromUnityButton);
  }



  void OnDisable() {
    DAS.Info("HockeAppManager.OnDisable", "HockeAppManager.OnDisable");
    #if (UNITY_IPHONE && !UNITY_EDITOR)
      System.AppDomain.CurrentDomain.UnhandledException -= OnHandleUnresolvedException;
      Application.logMessageReceived -= OnHandleLogCallback;
    #endif
  }

  /// <summary>
  /// Write a single exception report to disk.
  /// </summary>
  /// <param name="logString">A string that contains the reason for the exception.</param>
  /// <param name="stackTrace">The stacktrace for the exception.</param>
  protected virtual void WriteLogToDisk(string logString, string stackTrace) {
    
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    string logSession = System.DateTime.Now.ToString("yyyy-MM-dd-HH_mm_ss_fff");
    string log = logString.Replace("\n", " ");
    string[]stacktraceLines = stackTrace.Split('\n');
    
    log = "\n" + log + "\n";
    foreach (string line in stacktraceLines)
    {
      if(line.Length > 0)
      {
        log +="  at " + line + "\n";
      }
    }
    // Temp Start
    StreamWriter file = new StreamWriter(Application.persistentDataPath + LOG_FILE_DIR + "LogFile_" + logSession + ".log", true);
    file.WriteLine(log);
    // Temp End
    
    /*List<string> logHeaders = GetLogHeaders();
    using (StreamWriter file = new StreamWriter(Application.persistentDataPath + LOG_FILE_DIR + "LogFile_" + logSession + ".log", true))
    {
      foreach (string header in logHeaders)
      {
        file.WriteLine(header);
      }
      file.WriteLine(log);
    }*/
    #endif
  }

  /// <summary>
  /// Handle a single exception. By default the exception and its stacktrace gets written to disk.
  /// </summary>
  /// <param name="logString">A string that contains the reason for the exception.</param>
  /// <param name="stackTrace">The stacktrace for the exception.</param>
  protected virtual void HandleException(string logString, string stackTrace) {
    DAS.Event("HockeAppManager.HandleException", "HockeAppManager.HandleException");
    #if (UNITY_IPHONE && !UNITY_EDITOR)
      WriteLogToDisk(logString, stackTrace);
    #endif
  }

  /// <summary>
  /// Callback for handling log messages.
  /// </summary>
  /// <param name="logString">A string that contains the reason for the exception.</param>
  /// <param name="stackTrace">The stacktrace for the exception.</param>
  /// <param name="type">The type of the log message.</param>
  public void OnHandleLogCallback(string logString, string stackTrace, LogType type) {
    DAS.Event("HockeAppManager.OnHandleLogCallback", "HockeAppManager.OnHandleLogCallback");
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    if (LogType.Assert == type || LogType.Exception == type || LogType.Error == type) { 
      HandleException(logString, stackTrace);
    }   
    #endif
  }

  public void OnHandleUnresolvedException(object sender, System.UnhandledExceptionEventArgs args) {
    DAS.Event("HockeAppManager.OnHandleUnresolvedException", "HockeAppManager.OnHandleUnresolvedException");
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    if (args == null || args.ExceptionObject == null) { 
      return; 
    }

    if (args.ExceptionObject.GetType() == typeof(System.Exception)) { 
      System.Exception e = (System.Exception)args.ExceptionObject;
      HandleException(e.Source, e.StackTrace);
    }
    #endif
  }
}
