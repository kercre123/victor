using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using System.IO;
using System.Runtime.InteropServices;
using Anki.Debug;


public class HockeyAppManager : MonoBehaviour {

  protected const string HOCKEYAPP_BASEURL = "https://rink.hockeyapp.net/";
  protected const string HOCKEYAPP_CRASHESPATH = "api/2/apps/[APPID]/crashes/upload";
  protected const string LOG_FILE_DIR = "/unity_crash_logs/";
  protected const int MAX_CHARS = 199800;

  public enum AuthenticatorType {
    Anonymous,
    Device,
    HockeyAppUser,
    HockeyAppEmail,
    WebAuth
  }

  public string appID = "9ddf59a1bfc9487e9586842a82e32d9d";
  public AuthenticatorType authenticatorType;
  // only useful for authenticator types...
  public string secret = "";
  
  public string serverURL = "";

  public bool autoUpload = true;
  public bool exceptionLogging = true;
  public bool updateManager = false;

  #if (UNITY_IPHONE && !UNITY_EDITOR)
  [DllImport("__Internal")]
  private static extern void HockeyApp_StartHockeyManager(string appID, string serverURL, string authType, string secret, bool updateManagerEnabled, bool autoSendEnabled);
  [DllImport("__Internal")]
  private static extern string HockeyApp_GetVersionCode();
  [DllImport("__Internal")]
  private static extern string HockeyApp_GetVersionName();
  [DllImport("__Internal")]
  private static extern string HockeyApp_GetBundleIdentifier();
  [DllImport("__Internal")]
  private static extern string HockeyApp_GetSdkVersion();
  [DllImport("__Internal")]
  private static extern string HockeyApp_GetSdkName();
  #endif
  
  void Awake() {
    
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    DontDestroyOnLoad(gameObject);

    if(exceptionLogging == true && IsConnected() == true)
    {
      List<string> logFileDirs = GetLogFiles();
      if ( logFileDirs.Count > 0)
      {
        Debug.Log("Found files: " + logFileDirs.Count);
        StartCoroutine(SendLogs(logFileDirs));
      }
    }
    #endif
  }

  void HandleDebugConsoleCrashFromUnityButton(System.Object setvar) {
    DAS.Event("HockeAppManager.ForceDebugCrash", "HockeAppManager.ForceDebugCrash");
    if (setvar.ToString() != "exception") {
      throw new UnityException("ForcedExceptionTest");
    }
    DAS.Info("test.crash.impossible", "test.crash.impossible");
  }

  void OnEnable() {
    
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    if(exceptionLogging == true){
      System.AppDomain.CurrentDomain.UnhandledException += OnHandleUnresolvedException;
      Application.logMessageReceived += OnHandleLogCallback;
    }
    #endif
// Crashing is useful in all platforms.
    DAS.Info("HockeAppManager.OnEnable", "HockeAppManager.OnEnable");
    Anki.Cozmo.ExternalInterface.DebugConsoleVar consoleVar = new Anki.Cozmo.ExternalInterface.DebugConsoleVar();
    consoleVar.category = "Debug";
    consoleVar.varName = "Crash From Unity";
    consoleVar.varValue.varFunction = "CrashFromUnityFunc";
    DebugConsoleData.Instance.AddConsoleVar(consoleVar, this.HandleDebugConsoleCrashFromUnityButton);
  }

  void OnDisable() {
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    if(exceptionLogging == true){
      System.AppDomain.CurrentDomain.UnhandledException -= OnHandleUnresolvedException;
      Application.logMessageReceived -= OnHandleLogCallback;
    }
    #endif
  }

  void GameViewLoaded(string message) { 

    #if (UNITY_IPHONE && !UNITY_EDITOR)
    string urlString = GetBaseURL();
    string authTypeString = GetAuthenticatorTypeString();
    HockeyApp_StartHockeyManager(appID, urlString, authTypeString, secret, updateManager, autoUpload);
    #endif
  }

  /// <summary>
  /// Collect all header fields for the custom exception report.
  /// </summary>
  /// <returns>A list which contains the header fields for a log file.</returns>
  protected virtual List<string> GetLogHeaders() {
    List<string> list = new List<string>();
    
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    string bundleID = HockeyApp_GetBundleIdentifier();
    list.Add("Package: " + bundleID);
    
    string versionCode = HockeyApp_GetVersionCode();
    list.Add("Version Code: " + versionCode);

    string versionName = HockeyApp_GetVersionName();
    list.Add("Version Name: " + versionName);

    string osVersion = "OS: " + SystemInfo.operatingSystem.Replace("iPhone OS ", "");
    list.Add (osVersion);
    
    list.Add("Model: " + SystemInfo.deviceModel);

    list.Add("Date: " + DateTime.UtcNow.ToString("ddd MMM dd HH:mm:ss {}zzzz yyyy").Replace("{}", "GMT"));
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
    using(FileStream fs = File.OpenRead(log)){

      if (fs.Length > MAX_CHARS)
      {
        string resizedLog = null;

        using(StreamReader reader = new StreamReader(fs)){

          reader.BaseStream.Seek( fs.Length - MAX_CHARS, SeekOrigin.Begin );
          resizedLog = reader.ReadToEnd();
        }

        List<string> logHeaders = GetLogHeaders();
        string logHeader = "";
          
        foreach (string header in logHeaders)
        {
          logHeader += header + "\n";
        }
          
        resizedLog = logHeader + "\n" + "[...]" + resizedLog;

        try
        {
          bytes = System.Text.Encoding.Default.GetBytes(resizedLog);
        }
        catch(ArgumentException ae)
        {
          if (Debug.isDebugBuild) Debug.Log("Failed to read bytes of log file: " + ae);
        }
      }
      else
      {
        try
        {
          bytes = File.ReadAllBytes(log);
        }
        catch(SystemException se)
        {
          if (Debug.isDebugBuild) 
          {
            Debug.Log("Failed to read bytes of log file: " + se);
          }
        }

      }
    }

    if(bytes != null)
    {
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
    string logsDirectoryPath = Application.persistentDataPath + LOG_FILE_DIR;

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
    catch(Exception e)
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

    string crashPath = HOCKEYAPP_CRASHESPATH;
    string url = GetBaseURL() + crashPath.Replace("[APPID]", appID);

    #if (UNITY_IPHONE && !UNITY_EDITOR)
    string sdkVersion = HockeyApp_GetSdkVersion ();
    string sdkName = HockeyApp_GetSdkName ();
    if (sdkName != null && sdkVersion != null) {
      url += "?sdk=" + WWW.EscapeURL(sdkName) + "&sdk_version=" + sdkVersion;
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

      if (String.IsNullOrEmpty(www.error)) {
        try {
          File.Delete(log);
        }
        catch (Exception e) {
          if (Debug.isDebugBuild)
            Debug.Log("Failed to delete exception log: " + e);
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
    string logSession = DateTime.Now.ToString("yyyy-MM-dd-HH_mm_ss_fff");
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
    using (StreamWriter file = new StreamWriter(Application.persistentDataPath + LOG_FILE_DIR + "LogFile_" + logSession + ".log", true))
    {
      foreach (string header in logHeaders)
      {
        file.WriteLine(header);
      }
      file.WriteLine(log);
    }
    #endif
  }

  /// <summary>
  /// Get the base url used for custom exception reports.
  /// </summary>
  /// <returns>A formatted base url.</returns>
  protected virtual string GetBaseURL() {
    
    string baseURL = "";
    
    #if (UNITY_IPHONE && !UNITY_EDITOR)

    string urlString = serverURL.Trim();
    
    if(urlString.Length > 0)
    {
      baseURL = urlString;
      
      if(baseURL[baseURL.Length -1].Equals("/") != true){
        baseURL += "/";
      }
    }
    else
    {
      baseURL = HOCKEYAPP_BASEURL;
    }
    #endif

    return baseURL;
  }

  /// <summary>
  /// Get the base url used for custom exception reports.
  /// </summary>
  /// <returns>A formatted base url.</returns>
  protected virtual string GetAuthenticatorTypeString() {

    string authType = "";

    #if (UNITY_IPHONE && !UNITY_EDITOR)
    switch (authenticatorType)
    {
    case AuthenticatorType.Device:
      authType = "BITAuthenticatorIdentificationTypeDevice";
      break;
    case AuthenticatorType.HockeyAppUser:
      authType = "BITAuthenticatorIdentificationTypeHockeyAppUser";
      break;
    case AuthenticatorType.HockeyAppEmail:
      authType = "BITAuthenticatorIdentificationTypeHockeyAppEmail";
      break;
    case AuthenticatorType.WebAuth:
      authType = "BITAuthenticatorIdentificationTypeWebAuth";
      break;
    default:
      authType = "BITAuthenticatorIdentificationTypeAnonymous";
      break;
    }
    #endif

    return authType;
  }

  /// <summary>
  /// Checks whether internet is reachable
  /// </summary>
  protected virtual bool IsConnected() {

    bool connected = false;
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    
    if  (Application.internetReachability == NetworkReachability.ReachableViaLocalAreaNetwork || 
         (Application.internetReachability == NetworkReachability.ReachableViaCarrierDataNetwork))
    {
      connected = true;
    }
  
    #endif

    return connected;
  }

  /// <summary>
  /// Handle a single exception. By default the exception and its stacktrace gets written to disk.
  /// </summary>
  /// <param name="logString">A string that contains the reason for the exception.</param>
  /// <param name="stackTrace">The stacktrace for the exception.</param>
  protected virtual void HandleException(string logString, string stackTrace) {

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
    
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    if(LogType.Assert == type || LogType.Exception == type || LogType.Error == type)  
    { 
      HandleException(logString, stackTrace);
    }   
    #endif
  }

  public void OnHandleUnresolvedException(object sender, System.UnhandledExceptionEventArgs args) {
    
    #if (UNITY_IPHONE && !UNITY_EDITOR)
    if(args == null || args.ExceptionObject == null)
    { 
      return; 
    }

    if(args.ExceptionObject.GetType() == typeof(System.Exception))
    { 
      System.Exception e  = (System.Exception)args.ExceptionObject;
      HandleException(e.Source, e.StackTrace);
    }
    #endif
  }
}