using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using Anki.Debug;


public class HockeyAppManager : MonoBehaviour {

  protected const string kHockeyAppBaseUrl = "https://rink.hockeyapp.net/";
  protected const string kHockeyAppCrashesPath = "api/2/apps/[APPID]/crashes/upload";
  protected const string kLogFileDir = "/unity_crash_logs/";
  protected const int kMaxChars = 199800;

  private string _AppID = null;
  #if (UNITY_IPHONE && !UNITY_EDITOR)
  private string _BundleIdentifier;
  private string _VersionCode;
  private string _VersionName;
  private string _SDKVersion;
  private string _SDKName;
#endif
  void HandleDebugConsoleCrashFromUnityButton(System.Object setvar) {
    DAS.Event("HockeAppManager.ForceDebugCrash", "HockeAppManager.ForceDebugCrash");
    // Apparently dividing by 0 only forces an exception on mac not iOS. So just throw.
    if (setvar.ToString() != "exception") {
      throw new UnityException("ForcedExceptionTest");
    }
    DAS.Info("test.crash", "test.crash");
  }

  void OnEnable() {
    
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    System.AppDomain.CurrentDomain.UnhandledException += OnHandleUnresolvedException;
    Application.logMessageReceived += OnHandleLogCallback;
    #endif

    // Crashing is useful in all platforms.
    Anki.Cozmo.ExternalInterface.DebugConsoleVar consoleVar = new Anki.Cozmo.ExternalInterface.DebugConsoleVar();
    consoleVar.category = "Debug";
    consoleVar.varName = "Crash From Unity";
    consoleVar.varValue.varFunction = "CrashFromUnityFunc";
    DebugConsoleData.Instance.AddConsoleVar(consoleVar, this.HandleDebugConsoleCrashFromUnityButton);
  }

  void OnDisable() {
    #if (UNITY_IPHONE && !UNITY_EDITOR)
      System.AppDomain.CurrentDomain.UnhandledException -= OnHandleUnresolvedException;
      Application.logMessageReceived -= OnHandleLogCallback;
    #endif
  }

  /// <summary>
  /// Collect all header fields for the custom exception report.
  /// </summary>
  /// <returns>A list which contains the header fields for a log file.</returns>
  protected virtual List<string> GetLogHeaders() {
    List<string> list = new List<string>();
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    list.Add("Package: " + _BundleIdentifier);
    list.Add("Version Code: " + _VersionCode);
    list.Add("Version Name: " + _VersionName);

    string osVersion = "OS: " + SystemInfo.operatingSystem.Replace("iPhone OS ", "");
    list.Add (osVersion);
    
    list.Add("Model: " + SystemInfo.deviceModel);

    list.Add("Date: " + System.DateTime.UtcNow.ToString("ddd MMM dd HH:mm:ss {}zzzz yyyy").Replace("{}", "GMT").ToString());
    #endif
    
    return list;
  }

  /// <summary>
  /// Create the form data for a single exception report.
  /// </summary>
  /// <param name="log">A string that contains information about the exception.</param>
  /// <returns>The form data for the current exception report.</returns>
  protected virtual WWWForm CreateForm(string log) {
    
    WWWForm form = new WWWForm();
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    byte[] bytes = null;
    using (FileStream fs = File.OpenRead(log)) {
      if (fs.Length > kMaxChars) {
        string resizedLog = null;

        using (StreamReader reader = new StreamReader(fs)) {

          reader.BaseStream.Seek(fs.Length - kMaxChars, SeekOrigin.Begin);
          resizedLog = reader.ReadToEnd();
        }
        List<string> logHeaders = GetLogHeaders();
        string logHeader = "";
          
        foreach (string header in logHeaders) {
          logHeader += header + "\n";
        }
          
        resizedLog = logHeader + "\n" + "[...]" + resizedLog;

        try {
          bytes = System.Text.Encoding.Default.GetBytes(resizedLog);
        }
        catch (System.Exception ae) {
          if (Debug.isDebugBuild)
            Debug.Log("Failed to read bytes of log file: " + ae);
        }
      }
      else {
        try {
          bytes = File.ReadAllBytes(log);
        }
        catch (System.Exception se) {
          if (Debug.isDebugBuild) {
            Debug.Log("Failed to read bytes of log file: " + se);
          }
        }
      }
    }

    if (bytes != null) {
      form.AddBinaryData("log", bytes, log, "text/plain");
    }

    #endif
    
    return form;
  }

  /// <summary>
  /// Get a list of all existing exception reports.
  /// </summary>
  /// <returns>A list which contains the filenames of the log files.</returns>
  protected virtual List<string> GetLogFiles() {

    List<string> logs = new List<string>();

    #if (UNITY_IPHONE && !UNITY_EDITOR)
    string logsDirectoryPath = Application.persistentDataPath + kLogFileDir;

    try
    {
      if (Directory.Exists(logsDirectoryPath) == false)
      {
        Directory.CreateDirectory(logsDirectoryPath);
      }
    
      DirectoryInfo info = new DirectoryInfo(logsDirectoryPath);
      FileInfo[] files = info.GetFiles();

      if (files.Length > 0)
      {
        foreach (FileInfo file in files)
        {
          if (file.Extension == ".log")
          {
            logs.Add(file.FullName);
          }
          else
          {
            File.Delete(file.FullName);
          }
        }
      }
    }
    catch(System.Exception e)
    {
      if (Debug.isDebugBuild) Debug.Log("Failed to write exception log to file: " + e);
    }
    #endif

    return logs;
  }

  /// <summary>
  /// Upload existing reports to HockeyApp and delete them locally.
  /// </summary>
  protected virtual IEnumerator SendLogs(List<string> logs) {

    string crashPath = kHockeyAppCrashesPath;
    string url = kHockeyAppBaseUrl + crashPath.Replace("[APPID]", _AppID);
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    if (_SDKName != null && _SDKVersion != null) {
      url += "?sdk=" + WWW.EscapeURL(_SDKName) + "&sdk_version=" + _SDKVersion;
    }
    #endif
    foreach (string log in logs) {

      WWWForm postForm = CreateForm(log);
      string lContent = postForm.headers["Content-Type"].ToString();
      lContent = lContent.Replace("\"", "");
      Dictionary<string,string> headers = new Dictionary<string,string>();
      headers.Add("Content-Type", lContent);
      WWW www = new WWW(url, postForm.data, headers);
      yield return www;

      if (System.String.IsNullOrEmpty(www.error)) {
        try {
          File.Delete(log);
        }
        catch (System.Exception e) {
          if (Debug.isDebugBuild)
            DAS.Error(e, "HockeAppManager.LogDeleteFail");
        }
      }
    }
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
    
    List<string> logHeaders = GetLogHeaders();
    using (StreamWriter file = new StreamWriter(Application.persistentDataPath + kLogFileDir + "LogFile_" + logSession + ".log", true))
    {
      foreach (string header in logHeaders)
      {
        file.WriteLine(header);
      }
      file.WriteLine(log);
    }
    #endif
  }

  public void UploadUnityCrashInfo(string hockey_params) {
    // params are appId, versionCode, versionName, budleIdentifier, sdkversion, sdkname
#if (UNITY_IPHONE && !UNITY_EDITOR)
    char[] delimiterChars = { ',' };
    string[] split_params = hockey_params.Split(delimiterChars);
    _AppID = split_params[0];
    _VersionCode = split_params[1];
    _VersionName = split_params[2];
    _BundleIdentifier = split_params[3];
    _SDKVersion = split_params[4];
    _SDKName = split_params[5];
   
    if ((Application.internetReachability == NetworkReachability.ReachableViaLocalAreaNetwork ||
        (Application.internetReachability == NetworkReachability.ReachableViaCarrierDataNetwork))) {
      DAS.Event("HockeAppManager.UploadUnityCrashInfo.Connected", "HockeAppManager.UploadUnityCrashInfo.Connected");
      List<string> logFileDirs = GetLogFiles();
      DAS.Event("HockeAppManager.UploadUnityCrashInfo.LogCount = " + logFileDirs.Count, "HockeAppManager.UploadUnityCrashInfo.Connected " + logFileDirs.Count);
      if (logFileDirs.Count > 0) {
        StartCoroutine(SendLogs(logFileDirs));
      }
    }
#else
    DAS.Event("HockeAppManager.UploadUnityCrashInfoNotIOS " + hockey_params, "HockeyAppManager.UploadUnityCrashInfoNotIOS");
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
