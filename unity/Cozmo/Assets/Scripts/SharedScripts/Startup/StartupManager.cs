using Anki.Assets;
using System;
using System.Collections;
using UnityEngine;
using DG.Tweening;

// Used by Android flow
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.IO.IsolatedStorage;
using System.Runtime.InteropServices;
using DataPersistence;

/// <summary>
/// Add managers to this object by calling
/// gameObject.AddComponent<ManagerTypeName>();
/// The managers should be simple enough to not require any 
/// serialized fields.
/// If the manager is not a MonoBehaviour, just create 
/// the object and cache it. 
/// Also loads all needed Asset Bundles for the initial app start.
/// </summary>
public class StartupManager : MonoBehaviour {

  [System.Serializable]
  public class MainSceneData {
    public string[] AssetBundlesToLoad;
    public string MainSceneAssetBundleName;
    public string MainSceneName;
  }

  private const string _kUHDVariant = "uhd";
  private const string _kHDVariant = "hd";
  private const string _kSDVariant = "sd";

  private const string _kiOSVariant = "ios";
  private const string _kAndroidVariant = "android";

  private const int _kMaxHDWidth = 1920;
  private const int _kMaxHDHeight = 1080;
  private const int _kMaxSDWidth = 1136;
  private const int _kMaxSDHeight = 640;
  private const int _kSDThresholdMB = 200;
  private const int _kHDDThresholdMB = 635;

  private const char _kVersionDelimiter = '.';
  private const int _kVersionNumbersToShow = 3;

  private static StartupManager _Instance;

  [SerializeField]
  private MainSceneData _NeedsHubData;

  [SerializeField]
  private string _BasicUIPrefabAssetBundleName;

  [SerializeField]
  private string _GameMetadataAssetBundleName;

  [SerializeField]
  private string _DebugAssetBundleName;

  [SerializeField]
  private string[] _StartupDebugPrefabNames;

  [SerializeField]
  private GameObject _LoadingCanvas;

  [SerializeField]
  private GameObject _AndroidPermissionPrefab;
  private GameObject _AndroidPermissionInstance;

  // DO NOT REMOVE WITHOUT TESTING A PLAY STORE BUILD OF ANDROID!!!!
  // Everything used in the android permissions must be directly referenced.
  [SerializeField]
  private TMPro.TMP_Settings _TMPSettingsDirectReference;

  [SerializeField]
  private Cozmo.UI.ProgressBar _LoadingBar;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _LoadingBarLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _LoadingVersionLabel;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _LoadingDeviceIdLabel;

  [SerializeField]
  private CanvasGroup _LoadingAnkiLogoIcon;

  private string _ExtractionErrorMessage;

  private bool _IsDebugBuild = false;

  private bool _EngineConnected = false;

  private MainSceneData _MainSceneData;

  private bool _LogoAnimationFinished = false;
  private bool LogoAnimationFinished {
    get { return _LogoAnimationFinished; }
    set {
      if (value && value != _LogoAnimationFinished) {
        _LoadingBar.SetTargetAndAnimate(_LoadingProgress);
      }
      _LogoAnimationFinished = value;
      TryLoadMainScene();
    }
  }

  private float _LoadingProgress = 0f;
  private float LoadingProgress {
    get { return _LoadingProgress; }
    set {
      _LoadingProgress = value;
      if (_LogoAnimationFinished) {
        _LoadingBar.SetTargetAndAnimate(_LoadingProgress);
      }
      TryLoadMainScene();
    }
  }

  public void StartLoadAsync() {
    // Animate the Anki logo and start loading
    LogoAnimationFinished = false;
    const float fadeDuration = 0.6f;
    const float holdDuration = 0.8f;
    _LoadingAnkiLogoIcon.DOFade(0f, fadeDuration).SetEase(Ease.OutSine).SetDelay(holdDuration)
                        .OnComplete(() => {
                          LogoAnimationFinished = true;
                          StartCoroutine(LoadCoroutine());
                        });
  }

  public static StartupManager Instance {
    get {
      if (_Instance == null) {
        string stackTrace = System.Environment.StackTrace;
        DAS.Error("StartupManager.NullInstance", "Do not access StartupManager until start");
        DAS.Debug("StartupManager.NullInstance.StackTrace", DASUtil.FormatStackTrace(stackTrace));
        HockeyApp.ReportStackTrace("StartupManager.NullInstance", stackTrace);
      }
      return _Instance;
    }
    private set {
      if (_Instance != null) {
        DAS.Error("StartupManager.DuplicateInstance", "StartupManager Instance already exists");
      }
      _Instance = value;
    }
  }

  private float _EngineLoadingProgress = 0.0f;

  private IEnumerator LoadCoroutine() {
    // Initialize DAS first so we can have error messages during intialization
#if ANIMATION_TOOL
    DAS.AddTarget(new ConsoleDasTarget());
#elif (UNITY_IPHONE || UNITY_ANDROID) && !UNITY_EDITOR
    DAS.AddTarget(new EngineDasTarget());
#else
    DAS.AddTarget(new UnityDasTarget());
#endif

    Screen.orientation = ScreenOrientation.LandscapeLeft;
    
    Localization.LoadLocaleAndCultureInfo(DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.OverrideLanguage,
                                          DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.LanguageSettingOverride);

    _MainSceneData = _NeedsHubData;

#if UNITY_ANDROID && !UNITY_EDITOR
    bool needPermission = false;
    using (var permissionUtil = new AndroidJavaClass("com.anki.util.PermissionUtil")) {
      System.Func<bool> permissionChecker =
        () => permissionUtil.CallStatic<bool>("hasPermission", AndroidPermissionBlocker.kPermission);
      needPermission = !permissionChecker();

      if (needPermission) {
        _AndroidPermissionInstance = GameObject.Instantiate(_AndroidPermissionPrefab);
        _AndroidPermissionInstance.GetComponent<AndroidPermissionBlocker>().SetStartupManager(this);
        _AndroidPermissionInstance.transform.SetParent(_LoadingCanvas.transform, false);

        do {
          yield return null;
        } while (!permissionChecker());

        // if we got here, we finally got permission
        GameObject.Destroy(_AndroidPermissionInstance);
      }
    }
#endif

    // Start loading bar at close to 0
    LoadingProgress = 0.05f;
    _LoadingVersionLabel.text = null;
    _LoadingDeviceIdLabel.text = null;

    // set up progress bar updater for resource extraction
    float startingProgress = LoadingProgress;
    const float totalResourceExtractionProgress = 0.5f;
    Action<float> progressUpdater = prog => {
      float totalProgress = startingProgress + totalResourceExtractionProgress * Mathf.Clamp(prog, 0.0f, 1.0f);
      AddLoadingBarProgress(totalProgress - LoadingProgress);
    };

    // Extract resource files in platforms that need it
    yield return ExtractResourceFiles(progressUpdater);

    // If there was any error extracting the files, this is blocker and we can't continue running the app
    if (!string.IsNullOrEmpty(_ExtractionErrorMessage)) {
      _LoadingBarLabel.color = Color.red;
      _LoadingBarLabel.text = _ExtractionErrorMessage;

      // Stop loading dots coroutine
      StopAllCoroutines();
      yield break;
    }

    RobotEngineManager.Instance.CozmoEngineInitialization();

    if (RobotEngineManager.Instance.RobotConnectionType != RobotEngineManager.ConnectionType.Mock) {
      RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToEngine;
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.EngineLoadingDataStatus>(HandleDataLoaded);
      ConnectToEngine();
    }

    // Load asset bundler
    AssetBundleManager.IsLogEnabled = true;

    AssetBundleManager assetBundleManager = gameObject.AddComponent<AssetBundleManager>();
    yield return InitializeAssetBundleManager(assetBundleManager);

    AddLoadingBarProgress(0.1f);

    // INGO: QA is testing on a release build so manually set to true for now
    // _IsDebugBuild = Debug.isDebugBuild;
    _IsDebugBuild = true;

    string delimiter = "-";
    string resolutionVariant = GetVariantBasedOnScreenResolution();

    // platform independent, resolution dependent
    assetBundleManager.AddActiveVariant(resolutionVariant);

    // platform dependent, resolution dependent
    string platformVariant = GetVariantBasedOnPlatform();
    assetBundleManager.AddActiveVariant(platformVariant + delimiter + resolutionVariant);

    // platform dependent only (usually prefabs)
    assetBundleManager.AddActiveVariant(platformVariant);

    // language dependent only; images with embedded text (Cozmo's password face)
    string localeVariant = Localization.GetStringsLocale().ToLower();
    assetBundleManager.AddActiveVariant(localeVariant);
    assetBundleManager.AddActiveVariant(localeVariant + delimiter + resolutionVariant);

    // platform and language dependent; device settings images with embedded text
    assetBundleManager.AddActiveVariant(platformVariant + delimiter + localeVariant + delimiter + resolutionVariant);

    // Font is different for latin vs. Japanese
    string fontVariant = Localization.GetCurrentFontBundleVariant();
    assetBundleManager.AddActiveVariant(fontVariant);

    yield return LoadDebugAssetBundle(assetBundleManager, _IsDebugBuild);

    AddLoadingBarProgress(0.1f);

    // Load initial asset bundles
    yield return LoadAssetBundles(assetBundleManager);

    // Instantiate startup assets, skip if we're doing factory test.
#if !FACTORY_TEST
    LoadAssets(assetBundleManager);
#endif

    // Set up localization files and add managers
    if (Application.isPlaying) {
      Localization.LoadStrings();

      AddComponents();
    }

    if (RobotEngineManager.Instance.RobotConnectionType != RobotEngineManager.ConnectionType.Mock) {
      yield return CheckForEngineConnection();
      // Locale used to load asset bundles might just be debug version.
      // Trust engine platform dependent version when not debugging.
      if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.OverrideLanguage) {
        SetupEngine(localeVariant);
      }
      else {
        SetupEngine(string.Empty);
      }
    }

    // As soon as we're done connecting to the engine, start up some music
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Connectivity);

    // Ask for device data as early as you can
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DeviceDataMessage>(HandleDeviceDataMessage);
    RobotEngineManager.Instance.SendRequestDeviceData();

    if (RobotEngineManager.Instance.RobotConnectionType != RobotEngineManager.ConnectionType.Mock) {
      float progressBeforeEngineLoading = LoadingProgress;
      while (_EngineLoadingProgress < 1.0f) {
        float progressToAdd = _EngineLoadingProgress * (1.0f - progressBeforeEngineLoading);
        LoadingProgress = (progressBeforeEngineLoading + progressToAdd);
        yield return 0;
      }
    }

    // Push notification setup
    DataPersistenceManager.Instance.Data.DefaultProfile.PlayerId = DAS.GetGlobal("$player_id");

    LoadingProgress = 1.0f;

    if (RobotEngineManager.Instance.RobotConnectionType != RobotEngineManager.ConnectionType.Mock) {
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.EngineLoadingDataStatus>(HandleDataLoaded);
    }
  }

  private void TryLoadMainScene() {
    if (!LogoAnimationFinished || LoadingProgress < 1f) {
      return;
    }

    // Stop loading dots coroutine
    StopAllCoroutines();

    // Load main scene
    LoadMainScene(AssetBundleManager.Instance);

    int startSeed = System.Environment.TickCount;
    UnityEngine.Random.InitState(startSeed);
    DAS.Info("Random.StartSeed", startSeed.ToString());
  }

  private IEnumerator InitializeAssetBundleManager(AssetBundleManager assetBundleManager) {
    bool initializedAssetManager = false;
    assetBundleManager.Initialize((success) => initializedAssetManager = true);

    while (!initializedAssetManager) {
      yield return 0;
    }
  }

  private void HandleConnectedToEngine(string connectionIdentifier) {
    DAS.Info("StartupManager.HandleConnectedToEngine", "Engine connected!");
    _EngineConnected = true;
  }

  private void HandleDataLoaded(Anki.Cozmo.ExternalInterface.EngineLoadingDataStatus message) {
    _EngineLoadingProgress = message.ratioComplete;
  }

  private void HandleDeviceDataMessage(Anki.Cozmo.ExternalInterface.DeviceDataMessage message) {
    for (int i = 0; i < message.dataList.Length; ++i) {
      Anki.Cozmo.DeviceDataPair currentPair = message.dataList[i];
      string shortData = ShortenData(currentPair.dataValue);
      switch (currentPair.dataType) {
      case Anki.Cozmo.DeviceDataType.DeviceID: {
          _LoadingDeviceIdLabel.text = GetBootString(LocalizationKeys.kBootDeviceId, new object[] { shortData });
          break;
        }
      case Anki.Cozmo.DeviceDataType.BuildVersion: {
          // COZMO-10049 Only show the release version number, not the full one (which includes build information)
          string releaseVersion = "";
          string[] versionNumbers = shortData.Split(_kVersionDelimiter);
          for (int n = 0; n < Mathf.Min(_kVersionNumbersToShow, versionNumbers.Length); n++) {
            releaseVersion += versionNumbers[n] + _kVersionDelimiter;
          }
          // Chop off the last delimiter
          releaseVersion = releaseVersion.Substring(0, releaseVersion.Length - 1);
          _LoadingVersionLabel.text = GetBootString(LocalizationKeys.kBootAppVersion, new object[] { releaseVersion });
          break;
        }
      default: {
          DAS.Debug("SettingsVersionsPanel.HandleDeviceDataMessage.UnhandledDataType", currentPair.dataType.ToString());
          break;
        }
      }
    }
  }

  private string ShortenData(string data) {
    // It's possible  DefaultSettingsValuesConfig.Instance.CharactersOfAppInfoToShow 
    // is not loaded yet so just hardcode it
    int desiredStringLength = 13;
    string shortData = data;
    if (shortData.Length > desiredStringLength) {
      shortData = shortData.Substring(0, desiredStringLength);
    }
    return shortData;
  }

  private void SetupEngine(string locale) {
    RobotEngineManager.Instance.StartEngine(locale);
    // Set initial volumes
    Anki.Cozmo.Audio.GameAudioClient.SetPersistenceVolumeValues();
  }

  private void Awake() {
    _Instance = this;
  }

  private IEnumerator LoadDebugAssetBundle(AssetBundleManager assetBundleManager, bool isDebugBuild) {
    // Load debug asset bundle if in debug build
    if (_IsDebugBuild) {
      bool loadedDebugAssetBundle = false;
      assetBundleManager.LoadAssetBundleAsync(_DebugAssetBundleName,
        (success) => {
          if (!success) {
            DAS.Error("StartupManager.Awake.AssetBundleLoading",
              string.Format("Failed to load Asset Bundle with name={0}", _DebugAssetBundleName));
          }
          loadedDebugAssetBundle = true;
        });
      while (!loadedDebugAssetBundle) {
        yield return 0;
      }

      int loadedDebugAssets = 0;
      foreach (string prefabName in _StartupDebugPrefabNames) {
        AssetBundleManager.Instance.LoadAssetAsync<GameObject>(_DebugAssetBundleName,
          prefabName, (GameObject prefab) => {
            if (prefab != null) {
              GameObject go = Instantiate(prefab);
              go.transform.SetParent(transform);
            }
            loadedDebugAssets++;
          });
      }
      while (loadedDebugAssets < _StartupDebugPrefabNames.Length) {
        yield return 0;
      }
    }
  }

  public string GetVariantBasedOnScreenResolution() {
    // Our checks assume landscape, so "width" is the longer side of the device
    Resolution screenResolution = Screen.currentResolution;
    int screenResolutionWidth = Mathf.Max(screenResolution.width, screenResolution.height);
    int screenResolutionHeight = Mathf.Min(screenResolution.width, screenResolution.height);

    // Err on the side of most popular size
    string variant = null;
    if (screenResolutionWidth > _kMaxHDWidth || screenResolutionHeight > _kMaxHDHeight) {
      variant = _kUHDVariant;
    }
    else if (screenResolutionWidth > _kMaxSDWidth || screenResolutionHeight > _kMaxSDHeight) {
      variant = _kHDVariant;
    }
    else {
      variant = _kSDVariant;
    }

#if UNITY_EDITOR
    string assetPrefsValue = UnityEditor.EditorPrefs.GetString(Anki.Build.BuildKeyConstants.kAssetsPrefKey);
    switch (assetPrefsValue) {
    case Anki.Build.BuildKeyConstants.kSDAssetsValueKey:
      variant = _kSDVariant;
      break;
    case Anki.Build.BuildKeyConstants.kHDAssetsValueKey:
      variant = _kHDVariant;
      break;
    case Anki.Build.BuildKeyConstants.kUHDAssetsValueKey:
      variant = _kUHDVariant;
      break;
    }
#endif

#if UNITY_IOS || UNITY_ANDROID
    // Memory total in MB
    int memoryLimitMB = SystemInfo.systemMemorySize;
    if (memoryLimitMB <= _kSDThresholdMB) {
      DAS.Event("App.ForceResolution.Memory", "SD", DASUtil.FormatExtraData(memoryLimitMB.ToString()));
      variant = _kSDVariant;
    }
    else if (memoryLimitMB < _kHDDThresholdMB && variant == _kUHDVariant) {
      DAS.Event("App.ForceResolution.Memory", "HD", DASUtil.FormatExtraData(memoryLimitMB.ToString()));
      variant = _kHDVariant;
    }
#endif
    if (SystemInfo.deviceModel == "iPad2,5" || SystemInfo.deviceModel == "iPad2,6" || SystemInfo.deviceModel == "iPad2,7") {
      DAS.Event("App.ForceResolution", "detected iPad2,5 iPad2,6 iPad2,7");
      variant = _kSDVariant;
    }
    return variant;
  }

  private string GetVariantBasedOnPlatform() {
#if UNITY_IOS
    return _kiOSVariant;
#elif UNITY_ANDROID
    return _kAndroidVariant;
#else
    return "";
#endif
  }

  private IEnumerator LoadAssetBundles(AssetBundleManager assetBundleManager) {
    // Load initial asset bundles
    int loadedAssetBundles = 0;
    foreach (string assetBundleName in _MainSceneData.AssetBundlesToLoad) {
      assetBundleManager.LoadAssetBundleAsync(assetBundleName,
        (success) => {
          if (!success) {
            DAS.Error("StartupManager.Awake.AssetBundleLoading",
              string.Format("Failed to load Asset Bundle with name={0}", assetBundleName));
          }
          loadedAssetBundles++;
        });
    }

    while (loadedAssetBundles < _MainSceneData.AssetBundlesToLoad.Length) {
      yield return 0;
    }
  }

  private void ConnectToEngine() {
    RobotEngineManager.Instance.Disconnect();
    RobotEngineManager.Instance.Connect(RobotEngineManager.kEngineIP);
  }

  private IEnumerator CheckForEngineConnection() {
    while (!_EngineConnected) {
      yield return 0;
    }

  }

  private void AddComponents() {
    // Add managers to this object here
    gameObject.AddComponent<ObjectTagRegistryManager>();

    // Initialize persistance manager
    DataPersistence.DataPersistenceManager.CreateInstance();
    DataPersistence.DataPersistenceManager.Instance.InitSaveData();

    Cozmo.RequestGame.RequestGameManager.CreateInstance();
    Cozmo.Notifications.NotificationsManager.CreateInstance();

    // Guaranteed that NativeLibMessageReceiver is alive by now
    Cozmo.Util.NativeLibMessageReceiver.Instance.DeliverStoredMessages();

    Cozmo.UI.HasHiccupsAlertController.InitializeInstance();
  }

  private void LoadAssets(AssetBundleManager assetBundleManager) {
    assetBundleManager.LoadAssetAsync<Cozmo.ShaderHolder>(_BasicUIPrefabAssetBundleName,
                                                          "ShaderHolder",
                                                          Cozmo.ShaderHolder.SetInstance);

    assetBundleManager.LoadAssetAsync<Cozmo.UI.AlertModalLoader>(_BasicUIPrefabAssetBundleName,
                                                          "AlertModalLoader",
                                                          Cozmo.UI.AlertModalLoader.SetInstance);

    assetBundleManager.LoadAssetAsync<Cozmo.UI.UIColorPalette>(_BasicUIPrefabAssetBundleName,
                                                          "UIColorPalette",
                                                          Cozmo.UI.UIColorPalette.SetInstance);

    assetBundleManager.LoadAssetAsync<Cozmo.UI.UIDefaultTransitionSettings>(_BasicUIPrefabAssetBundleName,
                                                          "UIDefaultTransitionSettings",
                                                          Cozmo.UI.UIDefaultTransitionSettings.SetInstance);

    assetBundleManager.LoadAssetAsync<Cozmo.ItemDataConfig>(_GameMetadataAssetBundleName,
                                                          "ItemDataConfig",
                                                          Cozmo.ItemDataConfig.SetInstance);

    assetBundleManager.LoadAssetAsync<Cozmo.Challenge.ChallengeDataList>(_GameMetadataAssetBundleName,
                                                          "ChallengeList",
                                                          Cozmo.Challenge.ChallengeDataList.SetInstance);

    assetBundleManager.LoadAssetAsync<Cozmo.UI.GenericRewardsConfig>(_GameMetadataAssetBundleName,
                                                          "GenericRewardsConfig",
                                                          Cozmo.UI.GenericRewardsConfig.SetInstance);

    assetBundleManager.LoadAssetAsync<Cozmo.Settings.DefaultSettingsValuesConfig>(_GameMetadataAssetBundleName,
                                                          "DefaultSettingsValuesConfig",
                                                          Cozmo.Settings.DefaultSettingsValuesConfig.SetInstance);

    assetBundleManager.LoadAssetAsync<Cozmo.RequestGame.RequestGameListConfig>(_GameMetadataAssetBundleName,
                                                          "RequestGameList",
                                                          Cozmo.RequestGame.RequestGameListConfig.SetInstance);

    assetBundleManager.LoadAssetAsync<Cozmo.Songs.SongLocMap>(_GameMetadataAssetBundleName,
                                                          "SongLocMap",
                                                          Cozmo.Songs.SongLocMap.SetInstance);

    Cozmo.UI.CubePalette.LoadCubePalette(_BasicUIPrefabAssetBundleName);

    SkillSystem.Instance.Initialize();
    CozmoThemeSystemUtils.Initialize();
    Anki.Core.UI.Components.ThemeSystem.Create();
  }

  private void LoadMainScene(AssetBundleManager assetBundleManager) {
#if FACTORY_TEST
    assetBundleManager.LoadSceneAsync(_MainSceneData.MainSceneAssetBundleName, "FactoryTest", loadAdditively: false, callback: null);
#else
    assetBundleManager.LoadSceneAsync(_MainSceneData.MainSceneAssetBundleName, _MainSceneData.MainSceneName,
                                      loadAdditively: false, callback: null);
#endif
  }

  private void AddLoadingBarProgress(float amount) {
    if (LoadingProgress + amount > 0.95f) {
      LoadingProgress = 0.95f;
    }
    else {
      LoadingProgress = LoadingProgress + amount;
    }
  }

  public string GetBootString(string key, params object[] args) {
    string unformatedStr = Anki.Cozmo.Generated.LocBootStrings.GetBootString(key, Localization.GetLanguage());
    return string.Format(unformatedStr, args);
  }

  private IEnumerator ExtractResourceFiles(Action<float> progressUpdater) {
#if !UNITY_EDITOR && UNITY_ANDROID
    // In Android the files in streamingAssetsPath are in the jar file which means our native code can't access them. Here
    // we extract them from the jar file into persistentDataPath using the resources manifest.
    string fromPath = Application.streamingAssetsPath + "/";
    string toPath = PlatformUtil.GetResourcesBaseFolder() + "/";

    Debug.Log("About to extract resource files. fromPath = " + fromPath + ". toPath = " + toPath);

    // see if we can skip resource load
    {
      bool hashMatches = false;
      byte[] diskBytes = null;
      const string assetPath = "cozmo_resources/allAssetHash.txt";

      try {
        if (File.Exists(toPath + assetPath)) {
          diskBytes = File.ReadAllBytes(toPath + assetPath);
        }
      }
      catch (Exception e) {
        _ExtractionErrorMessage = GetBootString(LocalizationKeys.kBootErrorReadingFiles, 0);
        Debug.LogError("Exception checking asset hash: " + e.ToString());
        yield break;
      }

      if (diskBytes != null) {
        WWW appAssetHash = new WWW(fromPath + assetPath);
        yield return appAssetHash;

        try {
          if (diskBytes.SequenceEqual(appAssetHash.bytes)) {
            // data on disk is equal to data in this app bundle
            hashMatches = true;
          }
        }
        catch (Exception e) {
          _ExtractionErrorMessage = GetBootString(LocalizationKeys.kBootErrorReadingFiles, 1);
          Debug.LogError("Exception checking asset hash: " + e.ToString());
          yield break;
        }
      }

      if (hashMatches) {
        Debug.Log("Skipping extraction, existing hash matches");
        progressUpdater(1.0f);
        yield break;
      }
    }

    // Clear the unity cache so Unity doesn't confuse old and new versions of the asset bundles
    Caching.ClearCache();

    // Delete and create target extraction folder
    try {
      if (Directory.Exists(toPath)) {
        Directory.Delete(toPath, true);
      }
      Directory.CreateDirectory(toPath);
    }
    catch (Exception e) {
      _ExtractionErrorMessage = GetBootString(LocalizationKeys.kBootErrorReadingFiles, 2);
      Debug.LogError("There was an exception extracting the resource files: " + e.ToString());
      yield break;
    }

    // Load the manifest file with all the resources
    WWW resourcesWWW = new WWW(fromPath + "resources.txt");
    yield return resourcesWWW;

    if (!string.IsNullOrEmpty(resourcesWWW.error)) {
      _ExtractionErrorMessage = GetBootString(LocalizationKeys.kBootErrorReadingFiles, 3);
      Debug.LogError("Error loading resources.txt: " + resourcesWWW.error);
      yield break;
    }

    // Extract every individual file
    string[] files = resourcesWWW.text.Split('\n');
    List<string> filesToLoad = new List<string>();
    foreach (string fileName in files) {
      if (fileName.Contains(".")) {
        filesToLoad.Add(fileName);
      }
      else {
        // Assume this is a directory
        try {
          Directory.CreateDirectory(toPath + fileName);
        }
        catch (Exception e) {
          _ExtractionErrorMessage = GetBootString(LocalizationKeys.kBootErrorDiskFull);
          Debug.LogError("Error creating directory - " + fileName + " : " + e.ToString());
          yield break;
        }
      }
    }

    Debug.Log("Starting resource extraction");


    
    // define how many parallel loads we allow
    // in the results of my testing, 8 was the smallest number at which we basically didn't get any faster
    // by increasing the number any higher (was ~33% faster than 4, roughly same speed as 16)
    int maxLoadCount;
    // Sam C. 14 March 2018 - COZMO-16548
    // After the Unity 2017 upgrade, some devices are crashing during loading due to a bug in Unity's WWW class
    // when multiple files are being loaded simultaneously. The crash isn't guaranteed, but the probability of it happening
    // increases with the number of simultaneous loads happening. We'll give the app 2 chances at loading at faster speeds
    // before we drop down to single threaded loading. 
    // This counter is reset once we've successfully loaded, so future load attempts (which will only copy new/updated files)
    // aren't impacted, on the chance this bug gets fixed.
    switch (DataPersistenceManager.Instance.Data.DefaultProfile.NumLoadingAttempts) {
      case 0:
        maxLoadCount = 8;
        break;
      case 1:
        maxLoadCount = 3; // Magic number from testing
        break;
      default:
        maxLoadCount = 1;
        break;
    }
    
    DataPersistenceManager.Instance.Data.DefaultProfile.NumLoadingAttempts++;
    DataPersistenceManager.Instance.Save();
    
    float totalFiles = (float)filesToLoad.Count;
    LinkedList<LoadWrapper> wwws = new LinkedList<LoadWrapper>();
    while (filesToLoad.Count > 0 || wwws.Count > 0) {
      // go through existing loads and remove completed ones
      for (var safeIter = wwws.First; safeIter != null;) {
        var node = safeIter;
        safeIter = node.Next;
        if (node.Value.www.isDone) {
          bool success = ExtractOneFile(node.Value.www, toPath + node.Value.filename);
          if (!success) {
            yield break;
          }
          wwws.Remove(node);
        }
      }
      // fill empty slots with more loads
      while (wwws.Count < maxLoadCount && filesToLoad.Count > 0) {
        string newFile = filesToLoad[filesToLoad.Count - 1];
        filesToLoad.RemoveAt(filesToLoad.Count - 1);
        LoadWrapper wrapper = new LoadWrapper();
        wrapper.www = new WWW(fromPath + newFile);
        wrapper.filename = newFile;
        wwws.AddLast(wrapper);
      }
      // update and return
      progressUpdater((totalFiles - filesToLoad.Count) / totalFiles);
      yield return null;
    }

    progressUpdater(1.0f);
    
    // Reset counter so that if the OBB updates we don't force slow loading
    DataPersistenceManager.Instance.Data.DefaultProfile.NumLoadingAttempts = 0;
    DataPersistenceManager.Instance.Save();
    
    Debug.Log("Done with resource extraction");

    resourcesWWW.Dispose();
#endif

    _ExtractionErrorMessage = null;
    yield return null;
  }

#if !UNITY_EDITOR && UNITY_ANDROID
  private class LoadWrapper {
    public WWW www;
    public string filename;
  }

  private bool ExtractOneFile(WWW www, string toPath) {
    if (!string.IsNullOrEmpty(www.error)) {
      _ExtractionErrorMessage = GetBootString(LocalizationKeys.kBootErrorDiskFull);
      Debug.LogError("Error extracting file - streaming asset error: " + www.error);
      return false;
    }

    bool retryOnce = false;
    try {
      File.WriteAllBytes(toPath, www.bytes);
    }
    catch (IsolatedStorageException ise) {
      // This exception sometimes gets thrown during the first run on an Android 8.0 device, and supposedly indicates that
      // the intended file cannot be found. The file does in fact exist at the correct location, and on second run the app
      // extracts everything fine. To avoid showing an error to the user and making them restart the app, we'll retry the
      // write one time.
      retryOnce = true;
      Debug.Log("First attempt to extract file failed\n " + ise.ToString());
    }
    catch (Exception e) {
      _ExtractionErrorMessage = GetBootString(LocalizationKeys.kBootErrorDiskFull);
      Debug.LogError("Extracting file - unknown error: " + e.ToString());
      return false;
    }

    if (retryOnce) {
      try {
        Debug.Log("Retrying file extract " + toPath);
        File.WriteAllBytes(toPath, www.bytes);
      }
      catch (Exception e) {
        _ExtractionErrorMessage = GetBootString(LocalizationKeys.kBootErrorDiskFull);
        Debug.LogError("File extract retry failed!\n" + e.ToString());
        return false;
      }
    }

    www.Dispose();
    return true;
  }
#endif

  protected void LogCodeLabImportErrors(string errorString) {
    LogCodeLabImportErrors("CodeLab.OpenCodelabFile.Error.PlatformSpecific", errorString);
  }

  protected void LogCodeLabImportErrors(string errorCategory, string errorDetail) {
    DAS.Error(errorCategory, errorDetail);

    // This will export to android logcat in case DAS isn't running
    Debug.LogError(errorCategory + " " + errorDetail);

    CodeLab.CodeLabGame.PushImportError(errorCategory, errorDetail);
  }

  // Parses an incoming json stream expected to contain a codelab file.
  // The json is converted into a codeLabProject and inserted into the my_projects collection.
  protected void LoadCodeLabFromRawJson(string data) {
    try {
      if (CodeLab.CodeLabGame.IsRawStringValidCodelab(data)) {
        DataPersistence.CodeLabProject receivedProject = CodeLab.CodeLabGame.CreateProjectFromJsonString(data);
        DAS.Info("CodeLab.OpenCodelabFile.RecievedFromPlatform", "");

        if (receivedProject != null) {
          if (CodeLab.CodeLabGame.AddExternalProject(receivedProject)) {
            DAS.Info("CodeLab.OpenCodelabFile.LoadedSuccessfully", "size=" + data.Length);
            return;
          }
          else {
            LogCodeLabImportErrors("CodeLab.OpenCodelabFile.Error.AddProjectToList", "Could not add generated project from platform; size=" + data.Length);
          }
        }
        else {
          LogCodeLabImportErrors("CodeLab.OpenCodelabFile.Error.ProjectCreationError", "Could not generate a project from platform; size=" + data.Length);
        }
      }
      else {
        LogCodeLabImportErrors("CodeLab.OpenCodelabFile.Error.RecievedBadJson Recieved improperly formatted raw json; size=" + data.Length);
      }
    }
    catch (Exception e) {
      LogCodeLabImportErrors("CodeLab.OpenCodelabFile.Error.UnhandleException", e.Message + "; size=" + data.Length);
    }
  }
}
