using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Assets;
using System.IO;
using System.Linq;
using System;

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

  private const string _kUHDVariant = "uhd";
  private const string _kHDVariant = "hd";
  private const string _kSDVariant = "sd";

  private const int _kMaxHDWidth = 1920;
  private const int _kMaxHDHeight = 1080;
  private const int _kMaxSDWidth = 1136;
  private const int _kMaxSDHeight = 640;

  [SerializeField]
  private Cozmo.UI.ProgressBar _LoadingBar;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _LoadingBarLabel;

  [SerializeField]
  private float _AddDotSeconds = 0.25f;

  private float _CurrentProgress;
  private int _CurrentNumDots;

  [SerializeField]
  private string[] _AssetBundlesToLoad;

  [SerializeField]
  private string _MainSceneAssetBundleName;

  [SerializeField]
  private string _MainSceneName;

  [SerializeField]
  private string _BasicUIPrefabAssetBundleName;

  [SerializeField]
  private string _GameMetadataAssetBundleName;

  [SerializeField]
  private string _DebugAssetBundleName;

  [SerializeField]
  private string[] _StartupDebugPrefabNames;

  private string _ExtractionErrorMessage;

  private bool _IsDebugBuild = false;

  private bool _EngineConnected = false;

  // Use this for initialization
  private IEnumerator Start() {
    // Initialize DAS first so we can have error messages during intialization
#if ANIMATION_TOOL
    DAS.AddTarget(new ConsoleDasTarget());
#elif (UNITY_IPHONE || UNITY_ANDROID) && !UNITY_EDITOR
    DAS.AddTarget(new EngineDasTarget());
#else
    DAS.AddTarget(new UnityDasTarget());
#endif

    Screen.orientation = ScreenOrientation.LandscapeLeft;

    // Start loading bar at close to 0
    _CurrentProgress = 0.05f;
    _LoadingBar.SetProgress(_CurrentProgress);
    _CurrentNumDots = 0;
    StartCoroutine(UpdateLoadingDots());

    // set up progress bar updater for resource extraction
    float startingProgress = _CurrentProgress;
    const float totalResourceExtractionProgress = 0.5f;
    Action<float> progressUpdater = prog => {
      float totalProgress = startingProgress + totalResourceExtractionProgress * Mathf.Clamp(prog, 0.0f, 1.0f);
      AddLoadingBarProgress(totalProgress - _CurrentProgress);
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

#if !UNITY_EDITOR
    RobotEngineManager.Instance.ConnectedToClient += HandleConnectedToEngine;
    ConnectToEngine();
#endif

    // Load asset bundler
    AssetBundleManager.IsLogEnabled = true;

    AssetBundleManager assetBundleManager = gameObject.AddComponent<AssetBundleManager>();
    yield return InitializeAssetBundleManager(assetBundleManager);

    AddLoadingBarProgress(0.1f);

    // INGO: QA is testing on a release build so manually set to true for now
    // _IsDebugBuild = Debug.isDebugBuild;
    _IsDebugBuild = true;

    assetBundleManager.AddActiveVariant(GetVariantBasedOnScreenResolution());

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
      // TODO: Localization based on variant?
      Localization.LoadStrings();

      AddComponents();
    }

#if !UNITY_EDITOR
    yield return CheckForEngineConnection();
#endif

    _LoadingBar.SetProgress(1.0f);

    // Stop loading dots coroutine
    StopAllCoroutines();

    // Load main scene
    LoadMainScene(assetBundleManager);

    int startSeed = System.Environment.TickCount;
    UnityEngine.Random.seed = startSeed;
    DAS.Info("Unity.Random.StartSeed", startSeed.ToString());
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
    if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.SOSLoggerEnabled) {
      ConsoleLogManager.Instance.EnableSOSLogs(true);
    }
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

  private string GetVariantBasedOnScreenResolution() {
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


    return variant;
  }

  private IEnumerator LoadAssetBundles(AssetBundleManager assetBundleManager) {
    // Load initial asset bundles
    int loadedAssetBundles = 0;
    foreach (string assetBundleName in _AssetBundlesToLoad) {
      assetBundleManager.LoadAssetBundleAsync(assetBundleName,
        (success) => {
          if (!success) {
            DAS.Error("StartupManager.Awake.AssetBundleLoading",
              string.Format("Failed to load Asset Bundle with name={0}", assetBundleName));
          }
          loadedAssetBundles++;
        });
    }

    while (loadedAssetBundles < _AssetBundlesToLoad.Length) {
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
    // gameObject.AddComponent<ManagerTypeName>();
    gameObject.AddComponent<ExceptionReportManager>();
    gameObject.AddComponent<ObjectTagRegistryManager>();

    // Initialize persistance manager
    DataPersistence.DataPersistenceManager.CreateInstance();
    ChestRewardManager.CreateInstance();


    if (_IsDebugBuild) {
      gameObject.AddComponent<SOSLogManager>();
    }
  }

  private void LoadAssets(AssetBundleManager assetBundleManager) {
    // TODO: Don't hardcode this?
    assetBundleManager.LoadAssetAsync<Cozmo.ShaderHolder>(_BasicUIPrefabAssetBundleName,
      "ShaderHolder", (Cozmo.ShaderHolder sh) => {
        Cozmo.ShaderHolder.SetInstance(sh);
      });

    assetBundleManager.LoadAssetAsync<Cozmo.UI.AlertViewLoader>(_BasicUIPrefabAssetBundleName,
      "AlertViewLoader", (Cozmo.UI.AlertViewLoader avl) => {
        Cozmo.UI.AlertViewLoader.SetInstance(avl);
      });

    assetBundleManager.LoadAssetAsync<Cozmo.UI.UIColorPalette>(_BasicUIPrefabAssetBundleName,
      "UIColorPalette", (Cozmo.UI.UIColorPalette colorP) => {
        Cozmo.UI.UIColorPalette.SetInstance(colorP);
      });


    assetBundleManager.LoadAssetAsync<Cozmo.UI.UIDefaultTransitionSettings>(_BasicUIPrefabAssetBundleName,
      "UIDefaultTransitionSettings", (Cozmo.UI.UIDefaultTransitionSettings colorP) => {
        Cozmo.UI.UIDefaultTransitionSettings.SetInstance(colorP);
      });

    assetBundleManager.LoadAssetAsync<Cozmo.UI.ProgressionStatConfig>(_GameMetadataAssetBundleName,
      "ProgressionStatConfig", (Cozmo.UI.ProgressionStatConfig psc) => {
        psc.Initialize();
        Cozmo.UI.ProgressionStatConfig.SetInstance(psc);
      });

    assetBundleManager.LoadAssetAsync<Cozmo.ItemDataConfig>(_GameMetadataAssetBundleName,
      "ItemDataConfig", (Cozmo.ItemDataConfig idc) => {
        Cozmo.ItemDataConfig.SetInstance(idc);
      });

    assetBundleManager.LoadAssetAsync<TagConfig>(_GameMetadataAssetBundleName,
      "TagListConfig", (TagConfig tc) => {
        TagConfig.SetInstance(tc);
      });

    assetBundleManager.LoadAssetAsync<ChestData>(_GameMetadataAssetBundleName,
      "DefaultChestConfig", (ChestData cd) => {
        ChestData.SetInstance(cd);
      });

    assetBundleManager.LoadAssetAsync<Cozmo.HexItemList>(_GameMetadataAssetBundleName,
      "HexItemList", (Cozmo.HexItemList cd) => {
        Cozmo.HexItemList.SetInstance(cd);
      });

    if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.RunPressDemo) {
      assetBundleManager.LoadAssetAsync<ChallengeDataList>(_GameMetadataAssetBundleName,
        "PressDemoChallengeList", (ChallengeDataList cd) => {
          ChallengeDataList.SetInstance(cd);
        });
    }
    else {
      assetBundleManager.LoadAssetAsync<ChallengeDataList>(_GameMetadataAssetBundleName,
        "ChallengeList", (ChallengeDataList cd) => {
          ChallengeDataList.SetInstance(cd);
        });
    }

    assetBundleManager.LoadAssetAsync<Cozmo.UI.GenericRewardsConfig>(_GameMetadataAssetBundleName,
      "GenericRewardsConfig", (Cozmo.UI.GenericRewardsConfig cd) => {
        Cozmo.UI.GenericRewardsConfig.SetInstance(cd);
      });


    assetBundleManager.LoadAssetAsync<Cozmo.Settings.DefaultSettingsValuesConfig>(_GameMetadataAssetBundleName,
      "DefaultSettingsValuesConfig", (Cozmo.Settings.DefaultSettingsValuesConfig dsvc) => {
        Cozmo.Settings.DefaultSettingsValuesConfig.SetInstance(dsvc);
      });

    Cozmo.UI.CubePalette.LoadCubePalette(_BasicUIPrefabAssetBundleName);

    SkillSystem.Instance.Initialize();
  }

  private void LoadMainScene(AssetBundleManager assetBundleManager) {
#if FACTORY_TEST
    assetBundleManager.LoadSceneAsync(_MainSceneAssetBundleName, "FactoryTest", loadAdditively: false, callback: null);
#else
    if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.RunPressDemo) {
      assetBundleManager.LoadSceneAsync(_MainSceneAssetBundleName, "PressDemo", loadAdditively: false, callback: null);
    }
    else {
      assetBundleManager.LoadSceneAsync(_MainSceneAssetBundleName, _MainSceneName, loadAdditively: false, callback: null);
    }
#endif
  }

  private IEnumerator UpdateLoadingDots() {
    while (true) {
      string loadingText = "Loading";
      for (int i = 0; i < _CurrentNumDots; i++) {
        loadingText += ".";
      }
      _CurrentNumDots = (_CurrentNumDots + 1) % 4;
      _LoadingBarLabel.text = loadingText;
      yield return new WaitForSeconds(_AddDotSeconds);
    }
  }

  private void AddLoadingBarProgress(float amount) {
    if (_CurrentProgress + amount > 0.95f) {
      _LoadingBar.SetProgress(0.95f);
    }
    else {
      _CurrentProgress += amount;
      _LoadingBar.SetProgress(_CurrentProgress);
    }
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
      } catch (Exception e) {
        _ExtractionErrorMessage = "Exception checking asset hash: " + e.ToString();
        Debug.LogError(_ExtractionErrorMessage);
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
        } catch (Exception e) {
          _ExtractionErrorMessage = "Exception checking asset hash: " + e.ToString();
          Debug.LogError(_ExtractionErrorMessage);
          yield break;
        }
      }

      if (hashMatches) {
        Debug.Log("Skipping extraction, existing hash matches");
        progressUpdater(1.0f);
        yield break;
      }
    }

    try {
      if (Directory.Exists(toPath)) {
        Directory.Delete(toPath, true);
      }
      Directory.CreateDirectory(toPath);
    }
    catch (Exception e) {
      _ExtractionErrorMessage = "There was an exception extracting the resource files: " + e.ToString();
      Debug.LogError(_ExtractionErrorMessage);
      yield break;
    }

    // Load the manifest file with all the resources
    WWW resourcesWWW = new WWW(fromPath + "resources.txt");
    yield return resourcesWWW;

    if (!string.IsNullOrEmpty(resourcesWWW.error)) {
      _ExtractionErrorMessage = "Error loading resources.txt: " + resourcesWWW.error;
      Debug.LogError(_ExtractionErrorMessage);
      yield break;
    }

    // Extract every individual file
    string[] files = resourcesWWW.text.Split('\n');
    List<string> filesToLoad = new List<string>();
    foreach (string fileName in files) {
      if (fileName.Contains(".")) {
        filesToLoad.Add(fileName);
      } else {
        // Assume this is a directory
        try {
          Directory.CreateDirectory(toPath + fileName);
        }
        catch (Exception e) {
          _ExtractionErrorMessage = "Error extracting file: " + e.ToString();
          Debug.LogError(_ExtractionErrorMessage);
          yield break;
        }
      }
    }

    Debug.Log("Starting resource extraction");

    // define how many parallel loads we allow
    // in the results of my testing, 8 was the smallest number at which we basically didn't get any faster
    // by increasing the number any higher (was ~33% faster than 4, roughly same speed as 16)
    const int maxLoadCount = 8;

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
      _ExtractionErrorMessage = "Error extracting file: " + www.error;
      Debug.LogError(_ExtractionErrorMessage);
      return false;
    }

    try {
      File.WriteAllBytes(toPath, www.bytes);
    }
    catch (Exception e) {
      _ExtractionErrorMessage = "Error extracting file: " + e.ToString();
      Debug.LogError(_ExtractionErrorMessage);
      return false;
    }

    www.Dispose();
    return true;
  }
#endif
}
