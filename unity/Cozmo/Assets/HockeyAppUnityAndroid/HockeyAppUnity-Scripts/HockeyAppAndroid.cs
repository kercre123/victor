using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using System.IO;
using System.Runtime.InteropServices;

public class HockeyAppAndroid : MonoBehaviour
{
  
  protected const string HOCKEYAPP_BASEURL = "https://rink.hockeyapp.net/";
  protected const string HOCKEYAPP_CRASHESPATH = "api/2/apps/[APPID]/crashes/upload";
  protected const int MAX_CHARS = 199800;
  protected const string LOG_FILE_DIR = "/logs/";
  private static HockeyAppAndroid instance;

  public enum AuthenticatorType
  {
    Anonymous,
    HockeyAppEmail,
    HockeyAppUser,
    Validate
  }

  [Header("HockeyApp Setup")]
  public string packageID = "your-package-identifier";
  public string serverURL = "your-custom-server-url";

  [Header("Authentication")]
  public AuthenticatorType authenticatorType;
  public string secret = "your-hockey-app-secret";

  [Header("Crashes & Exceptions")]
  public bool autoUploadCrashes = false;
  public bool exceptionLogging = true;

  [Header("Metrics")]
  public bool userMetrics = true;

  [Header("Version Updates")]
  public bool updateAlert = true;

  private string hockeyAppId = null;

  void Awake ()
  {

    #if (UNITY_ANDROID && !UNITY_EDITOR)
    if (instance != null) {
      return;
    }

    DontDestroyOnLoad(gameObject);
    CreateLogDirectory();

    hockeyAppId = GetManifestProperty(CozmoBinding.GetCurrentActivity(), "HOCKEYAPP_APP_ID");

    if(exceptionLogging == true  && IsConnected() == true) {
      List<string> logFileDirs = GetLogFiles();
      if(logFileDirs.Count > 0) {
        StartCoroutine(SendLogs(logFileDirs));
      }
    }
    serverURL = GetBaseURL();
    int authType = (int)authenticatorType;
    StartCrashManager(serverURL, secret, authType, updateAlert, userMetrics, autoUploadCrashes);
    #endif
  }
  
  void OnEnable ()
  {
    
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    if(exceptionLogging == true) {
      System.AppDomain.CurrentDomain.UnhandledException += OnHandleUnresolvedException;
      Application.logMessageReceived += OnHandleLogCallback;
    }
    #endif
  }
  
  void OnDisable ()
  {
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    if (exceptionLogging == true) {
      System.AppDomain.CurrentDomain.UnhandledException -= OnHandleUnresolvedException;
      Application.logMessageReceived -= OnHandleLogCallback;
    }
    #endif
  }

  /// <summary>
  /// Start HockeyApp for Unity.
  /// </summary>
  /// <param name="urlString">The url of the endpoint used for sending data.</param>
  /// <param name="secret">The app secret used for authenticating users.</param>
  /// <param name="authType">Auth type used for authentication: Anonymous, email, email& password, or check if user was explicitly added to use this app.</param>
  /// <param name="updateManagerEnabled">True, if user should be notified about newer versions of the app.</param>
  /// <param name="userMetricsEnabled">True, app should send user and session information.</param>
  /// <param name="autoSendEnabled">True, if crashes should be sent without asking the user for approval.</param>
  protected void StartCrashManager (string urlString, string secret, int authType, bool updateManagerEnabled, bool userMetricsEnabled, bool autoSendEnabled)
  {
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    // get Unity activity
    AndroidJavaObject currentActivity = CozmoBinding.GetCurrentActivity();

    // get DAS (for device ID)
    AndroidJavaClass dasClass = new AndroidJavaClass("com.anki.daslib.DAS");
    string deviceId = dasClass.CallStatic<string>("getDeviceID", currentActivity);

    // if hockeyapp ID is empty, this is a debug/local build and we don't care about crash reporting
    if (hockeyAppId.Length == 0) {
      return;
    }

    // initialize HockeyApp.Constants
    AndroidJavaClass hockeyAppConstants = new AndroidJavaClass("net.hockeyapp.android.Constants");
    hockeyAppConstants.CallStatic("loadFromContext", currentActivity);

    // install google breakpad
    AndroidJavaClass nativeCrashManagerClass = new AndroidJavaClass("com.anki.hockeyappandroid.NativeCrashManager");
    string dumpsDirectory = nativeCrashManagerClass.CallStatic<string>("getDumpsDirectory", currentActivity);
    string dumpsExtension = nativeCrashManagerClass.GetStatic<string>("DMP_EXTENSION");
    string appRunId = CozmoBinding.AppRunId.ToString();
    dumpsDirectory += "/" + appRunId + dumpsExtension;
    CozmoBinding.cozmo_install_google_breakpad(dumpsDirectory);

    // initialize NativeCrashManager
    AndroidJavaObject nativeCrashManager = nativeCrashManagerClass.CallStatic<AndroidJavaObject>("getInstance",
      currentActivity,
      appRunId,
      hockeyAppId,
      deviceId);
    nativeCrashManager.Call("updateDescriptionFile", getCrashDescriptionJSON(appRunId, deviceId));

    AndroidJavaClass pluginClass = new AndroidJavaClass("net.hockeyapp.unity.HockeyUnityPlugin");
    pluginClass.CallStatic("startHockeyAppManager", currentActivity, urlString, hockeyAppId, secret, authType, updateManagerEnabled, userMetricsEnabled, autoSendEnabled);
    instance = this;

    #endif
  }

  private string getCrashDescriptionJSON(string appRunId, string deviceId) {
    JSONObject json = new JSONObject();
    json.AddField("apprun", appRunId);
    json.AddField("device", deviceId);
    return json.ToString();
  }

  #if (UNITY_ANDROID && !UNITY_EDITOR)
  private string GetManifestProperty(AndroidJavaObject context, string name) {
    int getMetaData = new AndroidJavaClass("android.content.pm.PackageManager").GetStatic<int>("GET_META_DATA");
    AndroidJavaObject packageName = context.Call<AndroidJavaObject>("getPackageName");
    AndroidJavaObject appInfo = context.Call<AndroidJavaObject>("getPackageManager")
      .Call<AndroidJavaObject>("getApplicationInfo", packageName, getMetaData);
    if (appInfo != null) {
      AndroidJavaObject metaData = appInfo.Get<AndroidJavaObject>("metaData");
      if (metaData != null) {
        return metaData.Call<string>("getString", name);
      }
    }
    return "";
  }
  #endif

  /// <summary>
  /// Check for version update and present alert if newer version is available.
  /// </summary>
  public static void CheckForUpdate()
  { 
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    if (instance != null) {
      AndroidJavaClass pluginClass = new AndroidJavaClass("net.hockeyapp.unity.HockeyUnityPlugin"); 
      pluginClass.CallStatic("checkForUpdate", CozmoBinding.GetCurrentActivity(), instance.serverURL, instance.hockeyAppId);
    } else {
      Debug.Log("Failed to check for update. SDK has not been initialized, yet.");
    }
    #endif
  }

  /// <summary>
  /// Display a feedback form.
  /// </summary>
  public static void ShowFeedbackForm()
  { 
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    if (instance != null) {
      AndroidJavaClass pluginClass = new AndroidJavaClass("net.hockeyapp.unity.HockeyUnityPlugin"); 
      pluginClass.CallStatic("startFeedbackForm", CozmoBinding.GetCurrentActivity());
    } else {
      Debug.Log("Failed to present feedback form. SDK has not been initialized, yet.");
    }
    #endif
  }

  /// <summary>
  /// Get the version code of the app.
  /// </summary>
  /// <returns>The version code of the Android app.</returns>
  protected String GetVersionCode ()
  {
    string versionCode = null;

    #if (UNITY_ANDROID && !UNITY_EDITOR)
    AndroidJavaClass jc = new AndroidJavaClass("net.hockeyapp.unity.HockeyUnityPlugin"); 
    versionCode =  jc.CallStatic<string>("getVersionCode");
    #endif

    return versionCode;
  }

  /// <summary>
  /// Get the version name of the app.
  /// </summary>
  /// <returns>The version name of the Android app.</returns>
  protected String GetVersionName ()
  {
    string versionName = null;
    
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    AndroidJavaClass jc = new AndroidJavaClass("net.hockeyapp.unity.HockeyUnityPlugin"); 
    versionName =  jc.CallStatic<string>("getVersionName");
    #endif
    
    return versionName;
  }

  /// <summary>
  /// Get the SDK version.
  /// </summary>
  /// <returns>The SDK version.</returns>
  protected String GetSdkVersion ()
  {
    string sdkVersion = null;
    
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    AndroidJavaClass jc = new AndroidJavaClass("net.hockeyapp.unity.HockeyUnityPlugin"); 
    sdkVersion =  jc.CallStatic<string>("getSdkVersion");
    #endif
    
    return sdkVersion;
  }

  /// <summary>
  /// Get the name of the SDK.
  /// </summary>
  /// <returns>The name of the SDK.</returns>
  protected String GetSdkName ()
  {
    string sdkName = null;
    
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    AndroidJavaClass jc = new AndroidJavaClass("net.hockeyapp.unity.HockeyUnityPlugin"); 
    sdkName =  jc.CallStatic<string>("getSdkName");
    #endif
    
    return sdkName;
  }


  /// <summary>
  /// Collect all header fields for the custom exception report.
  /// </summary>
  /// <returns>A list which contains the header fields for a log file.</returns>
  protected virtual List<string> GetLogHeaders ()
  {
    List<string> list = new List<string> ();

    #if (UNITY_ANDROID && !UNITY_EDITOR)

    list.Add("Package: " + packageID);

    string versionCode = GetVersionCode();
    list.Add("Version Code: " + versionCode);

    string versionName = GetVersionName();
    list.Add("Version Name: " + versionName);

    string[] versionComponents = SystemInfo.operatingSystem.Split('/');
    string osVersion = "Android: " + versionComponents[0].Replace("Android OS ", "");
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
  /// <returns>The form data for the current crash report.</returns>
  protected virtual WWWForm CreateForm (string log)
  {
    WWWForm form = new WWWForm ();

    #if (UNITY_ANDROID && !UNITY_EDITOR)
    byte[] bytes = null;
    using(FileStream fs = File.OpenRead(log)){
      
      if (fs.Length > MAX_CHARS) {
        string resizedLog = null;
        
        using(StreamReader reader = new StreamReader(fs)) {
          
          reader.BaseStream.Seek( fs.Length - MAX_CHARS, SeekOrigin.Begin );
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
        } catch(ArgumentException ae) {
          if (Debug.isDebugBuild) {
            Debug.Log("Failed to read bytes of log file: " + ae);
          }
        }
      } else {
        try {
          bytes = File.ReadAllBytes(log);
        } catch(SystemException se) {
          if (Debug.isDebugBuild) {
            Debug.Log("Failed to read bytes of log file: " + se);
          }
        }
      }
    }
    
    if(bytes != null) {
      form.AddBinaryData("log", bytes, log, "text/plain");
    }
    
    #endif
    
    return form;
  }

  /// <summary>
  /// Create the log directory if needed.
  /// </summary>
  protected virtual void CreateLogDirectory ()
  {
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    string logsDirectoryPath = Application.persistentDataPath + LOG_FILE_DIR;
    
    try {
      Directory.CreateDirectory (logsDirectoryPath);
    } catch (Exception e) {
      if (Debug.isDebugBuild) Debug.Log ("Failed to create log directory at " + logsDirectoryPath + ": " + e);
    }
    #endif
  }

  /// <summary>
  /// Get a list of all existing exception reports.
  /// </summary>
  /// <returns>A list which contains the filenames of the log files.</returns>
  protected virtual List<string> GetLogFiles ()
  {
    List<string> logs = new List<string> ();
    
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    string logsDirectoryPath = Application.persistentDataPath + LOG_FILE_DIR;
    
    try {
      DirectoryInfo info = new DirectoryInfo(logsDirectoryPath);
      FileInfo[] files = info.GetFiles();
      
      if (files.Length > 0) {
        foreach (FileInfo file in files) {
          if (file.Extension == ".log") {
            logs.Add(file.FullName);
          } else {
            File.Delete(file.FullName);
          }
        }
      }
    } catch(Exception e) {
      if (Debug.isDebugBuild) {
        Debug.Log("Failed to write exception log to file: " + e);
      }
    }
    #endif
    
    return logs;
  }

  /// <summary>
  /// Upload existing reports to HockeyApp and delete delete them locally.
  /// </summary>
  protected virtual IEnumerator SendLogs (List<string> logs)
  {
    string crashPath = HOCKEYAPP_CRASHESPATH;
    string url = GetBaseURL () + crashPath.Replace ("[APPID]", hockeyAppId);

    #if (UNITY_ANDROID && !UNITY_EDITOR)
    string sdkName = GetSdkName ();
    if (sdkName != null) {
      url+= "?sdk=" + WWW.EscapeURL(sdkName);
    }
    #endif

    foreach (string log in logs) {    
      WWWForm postForm = CreateForm (log);
      string lContent = postForm.headers ["Content-Type"].ToString ();
      lContent = lContent.Replace ("\"", "");
      Dictionary<string,string> headers = new Dictionary<string,string> ();
      headers.Add ("Content-Type", lContent);
      WWW www = new WWW (url, postForm.data, headers);
      yield return www;

      if (String.IsNullOrEmpty (www.error)) {
        try {
          File.Delete (log);
        } catch (Exception e) {
          if (Debug.isDebugBuild)
            Debug.Log ("Failed to delete exception log: " + e);
        }
      }
    }
  }

  /// <summary>
  /// Write a single exception report to disk.
  /// </summary>
  /// <param name="logString">A string that contains the reason for the exception.</param>
  /// <param name="stackTrace">The stacktrace for the exception.</param>
  protected virtual void WriteLogToDisk (string logString, string stackTrace)
  {
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    string logSession = DateTime.Now.ToString("yyyy-MM-dd-HH_mm_ss_fff");
    string log = logString.Replace("\n", " ");
    string[]stacktraceLines = stackTrace.Split('\n');
    
    log = "\n" + log + "\n";
    foreach (string line in stacktraceLines) {
      if(line.Length > 0) {
        log +="  at " + line + "\n";
      }
    }
    
    List<string> logHeaders = GetLogHeaders();
    using (StreamWriter file = new StreamWriter(Application.persistentDataPath + LOG_FILE_DIR + "LogFile_" + logSession + ".log", true)) {
      foreach (string header in logHeaders) {
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
  protected virtual string GetBaseURL ()
  { 
    string baseURL = "";
    
    #if (UNITY_ANDROID && !UNITY_EDITOR)

    string urlString = serverURL.Trim();
    if(urlString.Length > 0) {
      baseURL = urlString;
      
      if(baseURL[baseURL.Length -1].Equals("/") != true) {
        baseURL += "/";
      }
    } else {
      baseURL = HOCKEYAPP_BASEURL;
    }
    #endif
    
    return baseURL;
  }
  
  /// <summary>
  /// Checks whether internet is reachable
  /// </summary>
  protected virtual bool IsConnected ()
  {
    bool connected = false;

    #if (UNITY_ANDROID && !UNITY_EDITOR)
    
    if  (Application.internetReachability == NetworkReachability.ReachableViaLocalAreaNetwork || 
         (Application.internetReachability == NetworkReachability.ReachableViaCarrierDataNetwork)) {
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
  protected virtual void HandleException (string logString, string stackTrace)
  {
    
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    WriteLogToDisk(logString, stackTrace);
    #endif
  }

  /// <summary>
  /// Callback for handling log messages.
  /// </summary>
  /// <param name="logString">A string that contains the reason for the exception.</param>
  /// <param name="stackTrace">The stacktrace for the exception.</param>
  /// <param name="type">The type of the log message.</param>
  public void OnHandleLogCallback (string logString, string stackTrace, LogType type)
  {
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    if(LogType.Assert == type || LogType.Exception == type || LogType.Error == type) {  
      HandleException(logString, stackTrace);
    } 
    #endif
  }

  /// <summary>
  /// Callback for handling unresolved exceptions.
  /// </summary>
  public void OnHandleUnresolvedException (object sender, System.UnhandledExceptionEventArgs args)
  {
    #if (UNITY_ANDROID && !UNITY_EDITOR)
    if(args == null || args.ExceptionObject == null) {  
      return; 
    }

    if(args.ExceptionObject.GetType() == typeof(System.Exception)) {  
      System.Exception e  = (System.Exception)args.ExceptionObject;
      HandleException(e.Source, e.StackTrace);
    }
    #endif
  }
}
