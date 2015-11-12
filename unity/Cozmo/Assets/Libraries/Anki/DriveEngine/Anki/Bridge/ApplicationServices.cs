using System;
using System.Text;
using System.Runtime.InteropServices;
using UnityEngine;

/*
using Anki.DriveEngine.ExternalInterface;

namespace Anki {
	
  class ApplicationServices {

    // TODO: Move this into a swig-managed file.
    public enum RushHourState : byte {
      None,
      Created,
      Initializing,
      Loading,
      Running,
      HibernatingToHome,
      HibernatingInPlace,
      Terminating,
      Exit
    }

    private static ApplicationServices sInstance;
  
    public static event Action<RushHourState,RushHourState> OnDriveEngineStateChanged;
    public static event Action OnBackgroundedReturnToHome;
  
    private static bool _HasRequestedNotificationPrompt;

    public static EmotionService EmotionService { get { return GetInstance()._EmotionService; } }
    public static ProfileService ProfileService { get { return GetInstance()._ProfileService; } }
    public static AccountService AccountService { get { return GetInstance()._AccountService; } }
    public static StoreService StoreService { get { return GetInstance()._StoreService; } }
    public static QuestService QuestService { get { return GetInstance()._QuestService; } }
    public static AssetService AssetService { get { return GetInstance()._AssetService; } }
    public static JavaService JavaService { get { return GetInstance()._JavaService; } }

    private static ApplicationServices GetInstance() {
      if( sInstance == null ) {
        sInstance = new ApplicationServices();
      }
      return sInstance;
    }
  
  
    // services
    private EmotionService _EmotionService;
    private ProfileService _ProfileService;
    private AccountService _AccountService;
    private StoreService _StoreService;
    private QuestService _QuestService;
    private AssetService _AssetService;
    private JavaService _JavaService;
  
    private ApplicationServices() {
    }
  
    public static void Initialize() {

      ApplicationServices service = ApplicationServices.GetInstance();
      service._Initialize();
      
    }
    
    private void _Initialize() {
      ExternalInterfaceMessageDispatcher.Dispatcher.UnsubscribeAll();

      _EmotionService = new EmotionService();
      _ProfileService = new ProfileService();
      _AccountService = new AccountService();
      _StoreService = new StoreService();
      _QuestService = new QuestService();
      _AssetService = new AssetService();
      _JavaService = new JavaService();
      
      // Internationalization
      Anki.UI.TextHelper.LoadStrategies();

      string loc_language;
      string loc_country;
      GetCurrentLocale (out loc_language, out loc_country);
    
      DAS.Event("device.language_locale", loc_language);
      DAS.Event("device.country_locale", loc_country);
    
      Anki.UI.TextHelper.SetLanguage(loc_language);
      Localization.LoadStrings();

      int loopTimeMSec = 32;

      // Pre-load any metagame data that we don't want to fetch just-in-time
      GameType.RefreshGameTypes();
      Anki.RunLoop.MainThreadRunLoop.Run(loopTimeMSec);

      CommanderType.RefreshCommanderTypes();
      Anki.RunLoop.MainThreadRunLoop.Run(loopTimeMSec);

      GameItem.RefreshGameItems();
      Anki.RunLoop.MainThreadRunLoop.Run(loopTimeMSec);

      LootDrop.RefreshLootDrops();
      Anki.RunLoop.MainThreadRunLoop.Run(loopTimeMSec);
    
      AvatarData.RefreshAvatarData();
      Anki.RunLoop.MainThreadRunLoop.Run(loopTimeMSec);

      AchievementType.RefreshAchievementTypes();
      Anki.RunLoop.MainThreadRunLoop.Run(loopTimeMSec);

      MedalType.RefreshMedalTypes();
      Anki.RunLoop.MainThreadRunLoop.Run(loopTimeMSec);

      GameUpgradeGen2.RefreshGameUpgrades();
      Anki.RunLoop.MainThreadRunLoop.Run(loopTimeMSec);

      // Initialize services
      GameServiceDataReceiver.Initialize();
      PlayerServiceDataReceiver.Initialize();
      _EmotionService.Initialize();
      _ProfileService.Initialize();
      _StoreService.Initialize();
      _JavaService.Initialize();
      _QuestService.Initialize();
      _AssetService.Initialize();
      _AccountService.Initialize();

      // At this point, all C++ data has been loaded
      // And C# services are initialized and waiting
      // to receive data.

      // Spin the RunLoop for a short time to wait
      // for MessageDispatchers to transfer data (via CLAD messages).
      // NOTE: (BRC) This is pretty hacky... Each service
      // should have a way to signal when it is finished
      // loading data. Then we could just wait on actual
      // conditions.
      Anki.RunLoop.MainThreadRunLoop.Run(loopTimeMSec);

      // Flush our queue in case we are waiting for callback
      // responses from legacy C interface to be delivered.
      Anki.DispatchQueue.MainThreadDispatchQueue.Flush();

      // Continue loading C# data that depends on
      // services.
      LoadCampaignData.LoadData();

      // Initiate rams sync
      _AssetService.SendRequestAssetUpdate();
    
    }

    public static void RequestNotificationPrompt() {
      if( !_HasRequestedNotificationPrompt ) {
        _HasRequestedNotificationPrompt = true;
        ToEngineMessage toEngine = new ToEngineMessage();
        toEngine.requestNotificationPrompt = new RequestNotificationPrompt();
        ExternalInterfaceMessageDispatcher.Dispatcher.SendMessage(toEngine);
      }
      else {
        DAS.Debug("ApplicationServices.RequestNotificationPrompt", "Already prompted");
      }
    }
    
  public static void Reload() {
    DAS.Info("ApplicationServices.Reload", "bunny");
    DriveEngineContext.IsLoaded = false;
    DriveEngineContext.LoadDriveEngine();
  }

  
    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern void NativeUnityComponentRequestGuide(string initParam);

    public static void UnityComponentRequestGuide(string initParam) {
      NativeUnityComponentRequestGuide(initParam);
    }


    public static void OpenAppStoreLink() {
      Application.OpenURL("https://play.google.com/store/apps/details?id=com.anki.drive&hl=en");
    }

    public delegate void ApplicationStateHandler(byte oldState,byte newState);

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern void NativeRegisterApplication(ApplicationStateHandler appStateHandler,
                                                          ApplicationStateHandler engineStateHandler,
                                                          ApplicationStateHandler UIStateHandler);

    public static void RegisterApplication(ApplicationStateHandler appStateHandler,
                                         ApplicationStateHandler engineStateHandler,
                                         ApplicationStateHandler UIStateHandler) {
      NativeRegisterApplication(appStateHandler, engineStateHandler, UIStateHandler);
    }

    public static void RegisterApplicationStateHandlers() {
      RegisterApplication(OnApplicationStateUpdate, OnEngineStateUpdate, OnUIStateUpdate);
    }

    static uint StringBufferLen = 255;

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern uint NativeGetWifiSSIDName(StringBuilder sb, uint len);

    public static string GetWifiSSIDName() {
      StringBuilder sb = new StringBuilder((int)StringBufferLen);
      uint len = NativeGetWifiSSIDName(sb, (uint)sb.Capacity + 1);
      if (len > StringBufferLen) {
        DAS.Warn("ApplicationServices.GetWifiSSIDName", "Wifi SSID name.length exceeds 255 bytes");
      }
      else if (sb.Length == 0) {
        DAS.Warn("ApplicationServices.GetWifiSSIDName", "Wifi SSID name.length == 0");
      }
      return sb.ToString();
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
    #else
    [DllImport("DriveEngine")]
    #endif
    private static extern uint NativeIsBluetoothEnabled();

    public static bool IsBluetoothEnabled() {
      return NativeIsBluetoothEnabled() > 0;
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern uint NativeGetDeviceID(StringBuilder sb, uint len);
  
    public static string GetDeviceID() {
      StringBuilder sb = new StringBuilder((int)StringBufferLen);
      NativeGetDeviceID(sb, (uint)sb.Capacity + 1);
      return sb.ToString();
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern uint NativeGetAppVersion(StringBuilder sb, uint len);

    public static string GetAppVersion() {
      StringBuilder sb = new StringBuilder((int)StringBufferLen);
      uint versionLen = NativeGetAppVersion(sb, (uint)sb.Capacity + 1);
      string version = "N/A";
      if (versionLen > 0) {
        version = sb.ToString();
      }
      return version;
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern uint NativeGetAppVersionCode(StringBuilder sb, uint len);
  
    public static string GetAppVersionCode() {
      StringBuilder sb = new StringBuilder((int)StringBufferLen);
      NativeGetAppVersionCode(sb, (uint)sb.Capacity + 1);
      return sb.ToString();
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern uint NativeGetAppDisplayVersion(StringBuilder sb, uint len);
  
    public static string GetAppDisplayVersion() {
      StringBuilder sb = new StringBuilder((int)StringBufferLen);
      NativeGetAppDisplayVersion(sb, (uint)sb.Capacity + 1);
      return sb.ToString();
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern uint NativeGetAppPackage(StringBuilder sb, uint len);

    public static string GetAppPackage() {
      StringBuilder sb = new StringBuilder((int)StringBufferLen);
      NativeGetAppPackage(sb, (uint)sb.Capacity + 1);
      return sb.ToString();
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern uint NativeGetHockeyAppID(StringBuilder sb, uint len);

    public static string GetHockeyAppID() {
      StringBuilder sb = new StringBuilder((int)StringBufferLen);
      NativeGetHockeyAppID(sb, (uint)sb.Capacity + 1);
      return sb.ToString();
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
    private static extern uint NativeGetLastCrashDescription_ios(StringBuilder sb, uint len);
    #endif

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
    #else
    [DllImport("DriveEngine")]
    #endif
    private static extern uint NativeGetLastCrashDescription(StringBuilder sb, uint len);

    public static string GetLastCrashDescription() {
      int descriptionLen = 512;
      StringBuilder sb = new StringBuilder(descriptionLen);

      #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
      uint len = NativeGetLastCrashDescription_ios(sb, 0);
      sb.EnsureCapacity((int)len);
      NativeGetLastCrashDescription_ios(sb, (uint)sb.Capacity + 1);
      #else
      uint len = NativeGetLastCrashDescription(sb, 0);
      sb.EnsureCapacity((int)len);
      NativeGetLastCrashDescription(sb, (uint)sb.Capacity + 1);
      #endif
      return sb.ToString();
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
  	[DllImport("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
  	private static extern uint DriveEngineGetCurrentLocale(StringBuilder sb, uint len);

    public static string GetCurrentLocale(out string language, out string country) {
      StringBuilder sb = new StringBuilder((int)StringBufferLen);
      uint len = DriveEngineGetCurrentLocale(sb, (uint)sb.Capacity + 1);

      // default to en_US
      language = "en";
      country = "US";

      if (0 == len) {
        return "en_US";
      }

      string locale = sb.ToString();

      char[] delimiterChars = { '-', '_' };
      string[] chunks = locale.Split(delimiterChars);
      if (2 == chunks.Length) {
        language = chunks[0];
        country = chunks[1];
      }

      return locale;
    }

    public static void CrashNullReferenceException() {
      string crash = null;
      crash = crash.ToLower();
    }

    #if !UNITY_EDITOR && (UNITY_IPHONE || UNITY_XBOX360)
    [DllImport ("__Internal")]
  

  #else
    [DllImport("DriveEngine")]
    #endif
    private static extern void NativeUnityComponentCrashCPlusPlus();

    public static void CrashCrashCPlusPlus() {
      NativeUnityComponentCrashCPlusPlus();
    }

    #if !UNITY_EDITOR && UNITY_ANDROID
    [DllImport ("DriveEngine")]
    private static extern void NativeUnityComponentCrashJava();
    #endif

    public static void CrashJava() {
      #if !UNITY_EDITOR && UNITY_ANDROID
      NativeUnityComponentCrashJava();
      #endif
    }

    #if !UNITY_EDITOR && UNITY_IPHONE
    [DllImport ("__Internal")]
    private static extern void UnityDebugCrashObjC();
    #endif
    public static void CrashObjC() {
      #if !UNITY_EDITOR && UNITY_IPHONE
      UnityDebugCrashObjC();
      #endif
    }

    #if !UNITY_EDITOR && UNITY_IPHONE
    [DllImport ("__Internal")]
    private static extern void UnityDebugRecoverBLE();
    #endif
    public static void RecoverBLE() {
      #if !UNITY_EDITOR && UNITY_IPHONE
      UnityDebugRecoverBLE();
      #endif
    }

    [MonoPInvokeCallback(typeof(ApplicationStateHandler))]
    private static void OnApplicationStateUpdate(byte oldState, byte newState) {
      // BRC-TODO: Placeholder for when we need this.
      // We do not need to response to ApplicationEvents in Unity
    }

    [MonoPInvokeCallback(typeof(ApplicationStateHandler))]
    private static void OnEngineStateUpdate(byte oldState, byte newState) {
      Anki.DispatchQueue.MainThreadDispatchQueue.Async(() => {
        DAS.Debug("OnEngineStateUpdate", (RushHourState)oldState + " => " + (RushHourState)newState);
        if (null != OnDriveEngineStateChanged) {
          OnDriveEngineStateChanged((RushHourState)oldState, (RushHourState)newState);
        }
			
        byte appHibernatingToHomeState = Convert.ToByte(RushHourState.HibernatingToHome);

        if ((appHibernatingToHomeState == newState) && (appHibernatingToHomeState != oldState))
        {
          // app is hibernating (closes any open lobbies, network connections etc.)
          // also pop back to the main home screen as all lobbies etc. will now be invalid
          DAS.Info("App.Backgrounding", "Hibernating: Popping SceneDirector to Home");
          SceneDirector.Instance.PopToHome();
          if (OnBackgroundedReturnToHome != null) {
            OnBackgroundedReturnToHome();
          }
        }
      });
    }

    [MonoPInvokeCallback(typeof(ApplicationStateHandler))]
    private static void OnUIStateUpdate(byte oldState, byte newState) {
      // BRC-TODO: Placeholder for when we need this.
      // We do not need to response to UIEvents in Unity
    }
  
  } // class ApplicationService

} // namespace Anki
*/