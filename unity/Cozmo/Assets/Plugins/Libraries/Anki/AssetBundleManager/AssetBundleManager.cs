using UnityEngine;
using UnityEngine.SceneManagement;

#if UNITY_EDITOR
using UnityEditor;
#endif

using System;
using System.Collections;
using System.Collections.Generic;


namespace Anki {
  namespace Assets {

    public class AssetBundleManager : MonoBehaviour {

      public const string kAssetBundlesFolder = "AssetBundles";

      private static AssetBundleManager _sInstance;
      private static bool _sIsLogEnabled;

      private List<string> _ActiveVariants = new List<string>();
      private AssetBundleManifest _AssetBundleManifest = null;
      private Dictionary<string, LoadedAssetBundle> _LoadedAssetBundles = new Dictionary<string, LoadedAssetBundle>();
      private Dictionary<string, List<string>> _Variants = new Dictionary<string, List<string>>();

      private List<string> _InProgressAssetBundles = new List<string>();

      private string _AssetBundleFolder;

#if UNITY_EDITOR
      static int m_SimulateAssetBundleInEditor = -1;
      const string kSimulateAssetBundles = "SimulateAssetBundles";

      static public bool SimulateAssetBundleInEditor {
        get {
          if (m_SimulateAssetBundleInEditor == -1)
            m_SimulateAssetBundleInEditor = EditorPrefs.GetBool(kSimulateAssetBundles, true) ? 1 : 0;

          return m_SimulateAssetBundleInEditor != 0;
        }
        set {
          int newValue = value ? 1 : 0;
          if (newValue != m_SimulateAssetBundleInEditor) {
            m_SimulateAssetBundleInEditor = newValue;
            EditorPrefs.SetBool(kSimulateAssetBundles, value);
          }
        }
      }
#endif

      public static AssetBundleManager Instance {
        get {
          return _sInstance;
        }
      }

      public static bool IsLogEnabled {
        get {
          return _sIsLogEnabled;
        }
        set {
          _sIsLogEnabled = value;
        }
      }

      public void AddActiveVariant(string variant) {
        if (!_ActiveVariants.Exists((string s) => (s.Equals(variant)))) {
          _ActiveVariants.Add(variant);
        }
        PrintActiveVariants();
      }

      public void RemoveActiveVariant(string variant) {
        _ActiveVariants.Remove(variant);
        PrintActiveVariants();
      }

      // Initializes the asset manager. Once the process is finished, the function calls the callback indicating if the
      // intialization process was successful or not.
      public void Initialize(Action<bool> callback) {
        Log(LogType.Log, "Initializing Asset Manager");
        _sInstance = this;

#if UNITY_EDITOR
        // In simulator mode we don't load any bundles
        Log(LogType.Log, "Simulation Mode: " + (SimulateAssetBundleInEditor ? "Enabled" : "Disabled"));
        if (SimulateAssetBundleInEditor) {
          CallCallback(callback, true);
          return;
        }
#endif

        // Determine where the assets are located
        string manifestAssetBundleName = "";

#if UNITY_EDITOR
        string platformName = GetPlatformName(EditorUserBuildSettings.activeBuildTarget);
        manifestAssetBundleName = platformName;
        _AssetBundleFolder = kAssetBundlesFolder + "/" + platformName + "/";
#else
        manifestAssetBundleName = GetRuntimePlatformName();
        _AssetBundleFolder = System.IO.Path.Combine(Application.streamingAssetsPath, kAssetBundlesFolder) + "/";
#endif
        Log(LogType.Log, "Asset bundle folder is " + _AssetBundleFolder);

        // Load the asset bundle manifest from the main bundle which is called like the platform
        Log(LogType.Log, "Loading asset bundle manifest");
        LoadAssetBundleAsync(manifestAssetBundleName, (bool successful) => {
          if (!successful) {
            Log(LogType.Error, "Error initializing the asset system. Couldn't load the manifest bundle");
            CallCallback(callback, false);
            return;
          }

          // Load the asset manifest asset from its bundle
          LoadAssetAsync<AssetBundleManifest>(manifestAssetBundleName, "AssetBundleManifest", (AssetBundleManifest manifest) => {
            _AssetBundleManifest = manifest;

            if (_AssetBundleManifest != null) {
              // Initialize the variant remapping information
              string[] bundlesWithVariant = _AssetBundleManifest.GetAllAssetBundlesWithVariant();
              for (int i = 0; i < bundlesWithVariant.Length; i++) {
                string[] split = bundlesWithVariant[i].Split('.');

                List<string> variants = null;
                if (_Variants.TryGetValue(split[0], out variants)) {
                  variants.Add(split[1]);
                }
                else {
                  _Variants.Add(split[0], new List<string> { split[1] });
                }
              }
            }
            else {
              Log(LogType.Error, "Error initializing the asset system. Couln't load the asset bundle manifest.");
            }

            CallCallback(callback, _AssetBundleManifest != null);
          });
        });
      }

      // Loads an asset bundle asynchronously. This function loads the bundle and all its dependencies. Variants are resolved so
      // the right bundle is loaded according to the current active variants.
      // Once the operation is completed, the callback will be called with a boolean indicating if the load was successful or not.
      // Asset bundles are reference counted so they are only loaded once.
      public void LoadAssetBundleAsync(string assetBundleName, Action<bool> callback) {
        Log(LogType.Log, "Loading asset bundle " + assetBundleName);

#if UNITY_EDITOR
        if (SimulateAssetBundleInEditor) {
          CallCallback(callback, true);
          return;
        }
#endif

        string variantAssetBundleName = RemapVariantName(assetBundleName);

        Log(LogType.Log, variantAssetBundleName);

        StartCoroutine(LoadAssetBundleAsyncInternal(variantAssetBundleName, callback));
      }

      // Decreases the reference count for the bundle and unloads it if necessary. If the bundle will be unloaded, its dependencies
      // will be checked to see if they have to be unloaded as well. When destroyObjectsCreatedFromBundle is true, all the assets
      // loaded from the bundle will be destroyed automatically. When it is false, the assets won't be destroyed but the connection
      // with the bundle is lost. If the same bundle and asset are loaded again, a new copy of the asset will be created.
      public void UnloadAssetBundle(string assetBundleName, bool destroyObjectsCreatedFromBundle = true) {
        Log(LogType.Log, "Try unloading asset bundle " + assetBundleName);

#if UNITY_EDITOR
        if (SimulateAssetBundleInEditor) {
          Log(LogType.Log, "Unloaded asset bundle " + assetBundleName);
          return;
        }
#endif

        string variantAssetBundleName = RemapVariantName(assetBundleName);

        UnloadAssetBundleInternal(variantAssetBundleName, destroyObjectsCreatedFromBundle);
      }

      public AssetType LoadAsset<AssetType>(string assetBundleName, string assetName) where AssetType : UnityEngine.Object {
#if UNITY_EDITOR
        if (SimulateAssetBundleInEditor) {
          string[] assetPaths = AssetDatabase.GetAssetPathsFromAssetBundleAndAssetName(assetBundleName, assetName);

          if (assetPaths.Length == 0) {
            Log(LogType.Error, "Couldn't find asset " + assetName + " in asset bundle " + assetBundleName);
            return null;
          }

          if (assetPaths.Length > 1) {
            Log(LogType.Warning, "This case in LoadAssets needs to be implemented");
            return null;
          }
          AssetType asset = AssetDatabase.LoadAssetAtPath(assetPaths[0], typeof(AssetType)) as AssetType;
          if (asset == null) {
            Log(LogType.Error, "Asset returned from LoadMainAssetsAtPath is null. " + assetPaths[0]);
          }
          return asset;

        }
        else
#endif
        {
          return LoadAssetInternal<AssetType>(assetBundleName, assetName);
        }
      }

      private AssetType LoadAssetInternal<AssetType>(string assetBundleName, string assetName) where AssetType : UnityEngine.Object {
        LoadedAssetBundle loadedAssetBundle = null;
        if (!_LoadedAssetBundles.TryGetValue(assetBundleName, out loadedAssetBundle) || (loadedAssetBundle == null)) {
          Log(LogType.Error, "Couldn't load asset " + assetName + " from asset bundle " + assetBundleName + ". The asset bundle is not loaded");
          return null;
        }

        if (loadedAssetBundle.AssetBundle == null) {
          Log(LogType.Error, "Couldn't load asset " + assetName + " from asset bundle " + assetBundleName + ". The asset bundle is null");
          return null;
        }

        AssetType asset = loadedAssetBundle.AssetBundle.LoadAsset(assetName, typeof(AssetType)) as AssetType;
        if (asset == null) {
          Log(LogType.Error, "Couldn't load asset " + assetName + " from asset bundle " + assetBundleName + ". The request returned a null asset");
          return null;
        }
        return asset;
      }

      // Loads an asset asynchronously from the given asset bundle. The asset bundle must have been loaded previously.
      // When the operation  is completed, the callback will be called with the asset or a null refrence if it wasn't found.
      public void LoadAssetAsync<AssetType>(string assetBundleName, string assetName, Action<AssetType> callback) where AssetType : UnityEngine.Object {
        Log(LogType.Log, "Loading asset " + assetName + " from asset bundle " + assetBundleName);

#if UNITY_EDITOR
        if (SimulateAssetBundleInEditor) {
          string[] assetPaths = AssetDatabase.GetAssetPathsFromAssetBundleAndAssetName(assetBundleName, assetName);
          if (assetPaths.Length == 0) {
            Log(LogType.Error, "Couldn't find asset " + assetName + " in asset bundle " + assetBundleName);
            CallCallback(callback, null);
            return;
          }

          if (assetPaths.Length > 1) {
            Log(LogType.Warning, "This case in LoadAssets needs to be implemented");
            CallCallback(callback, null);
            return;
          }

          UnityEngine.Object obj = AssetDatabase.LoadMainAssetAtPath(assetPaths[0]);
          CallCallback(callback, obj as AssetType);
        }
        else
#endif
        {
          StartCoroutine(LoadAssetAsyncInternal<AssetType>(assetBundleName, assetName, callback));
        }
      }

      // Loads a new scene asynchronously from the given asset bundle. The asset bundle must have been loaded previously.
      // The loadAdditively parameter indicates whether the scene should be loaded additively or not.
      // Once the operation is completed, the callback will be called with a boolean indicating if the load was successful
      // or not.
      public void LoadSceneAsync(string assetBundleName, string sceneName, bool loadAdditively, Action<bool> callback) {
        Log(LogType.Log, "Loading scene " + sceneName + " from asset bundle " + assetBundleName + ((loadAdditively) ? " additively" : " non additively"));

#if UNITY_EDITOR
        if (SimulateAssetBundleInEditor) {
          StartCoroutine(LoadSceneAsyncInternalInEditor(assetBundleName, sceneName, loadAdditively, callback));
        }
        else
#endif
        {
          StartCoroutine(LoadSceneAsyncInternal(assetBundleName, sceneName, loadAdditively, callback));
        }
      }

      // Unloads an scene. Note that only scene that were loaded additively can be unloaded.
      // If unloadUnusedAssets is true then the function tries to unload all the assets that may be unused
      // after the scene is unloaded. It is a good idea to do this when transitioning between scenes to make sure that unused
      // assets are not left in memory.
      // After the operation is completed, the callback is called with a boolean to indicate if it was successful
      public void UnloadSceneAsync(string sceneName, Action<bool> callback, bool unloadUnusedAssets = true) {
        Log(LogType.Log, "Unloading scene " + sceneName);

        SceneManager.UnloadSceneAsync(sceneName);

        if (unloadUnusedAssets) {
          StartCoroutine(UnloadUnusedAssets(callback));
        }
        else {
          CallCallback(callback, true);
        }
      }

#if UNITY_EDITOR
      public static string GetPlatformName(BuildTarget buildTarget) {

        switch (buildTarget) {
        case BuildTarget.Android:
          return "android";
        case BuildTarget.iOS:
          return "ios";
        case BuildTarget.StandaloneOSXIntel:
        case BuildTarget.StandaloneOSXIntel64:
        case BuildTarget.StandaloneOSXUniversal:
          return "mac";
        default:
          Log(LogType.Error, "Unsupported platform " + Application.platform);
          return null;
        }
      }
#else
      
      public static string GetRuntimePlatformName() {
        switch (Application.platform) {
        case RuntimePlatform.Android:
          return "android";
        case RuntimePlatform.IPhonePlayer:
          return "ios";
        case RuntimePlatform.OSXPlayer:
          return "mac";
        default:
          Log(LogType.Error, "Unsupported platform " + Application.platform);
          return null;
        }
      }
#endif

      #region Private Methods

      private void Start() {
        if (_sInstance != null && _sInstance != this) {
          DAS.Error(this, "There are two AssetBundleManager instances in the project!!!");
          return;
        }

        _sInstance = this;
        DontDestroyOnLoad(gameObject);
      }

      // Remaps the asset bundle name to the best fitting asset bundle variant.
      private string RemapVariantName(string assetBundleName) {

        // Check if the bundle is supposed to have variants
        string[] split = assetBundleName.Split('.');
        if (split.Length <= 1) {
          return assetBundleName;
        }

        // Find the variant information for the bundle
        List<string> variants = null;
        if (!_Variants.TryGetValue(split[0], out variants)) {
          return assetBundleName;
        }

        // Find if there is an active variant for the bundle
        int variantIndex = -1;
        for (int i = 0; i < variants.Count; i++) {
          int found = _ActiveVariants.IndexOf(variants[i]);
          if (found != -1) {
            variantIndex = i;
            break;
          }
        }

        if (variantIndex != -1) {
          string variantAssetBundleName = split[0] + "." + variants[variantIndex];
          Log(LogType.Log, "Remapping asset bundle " + assetBundleName + " to " + variantAssetBundleName);

          return variantAssetBundleName;
        }
        else {
          return assetBundleName;
        }
      }

      private IEnumerator LoadAssetBundleAsyncInternal(string assetBundleName, Action<bool> callback) {
        // Check if the bundle is already load it and return it directly in that case
        LoadedAssetBundle loadedAssetBundle = null;
        _LoadedAssetBundles.TryGetValue(assetBundleName, out loadedAssetBundle);
        if (loadedAssetBundle != null) {
          loadedAssetBundle.OnLoaded();
          PrintLoadedBundleInfo();
          CallCallback(callback, true);
          yield break;
        }

        // Don't try to load the asset bundle if some other thread already started to load it
        if (_InProgressAssetBundles.Contains(assetBundleName)) {
          // Wait for someone else to finish loading it
          while (true) {
            _LoadedAssetBundles.TryGetValue(assetBundleName, out loadedAssetBundle);
            if (loadedAssetBundle != null) {
              loadedAssetBundle.OnLoaded();
              PrintLoadedBundleInfo();
              CallCallback(callback, true);
              yield break;
            }
            else {
              yield return 0;
            }
          }
        }

        _InProgressAssetBundles.Add(assetBundleName);

        // The bundle hasn't been loaded yet so load it from disk
        string assetBundlePath = _AssetBundleFolder + assetBundleName;
        AssetBundle assetBundle;

        if (!assetBundlePath.StartsWith("jar:")) {
          AssetBundleCreateRequest request = AssetBundle.LoadFromFileAsync(assetBundlePath);
          yield return request;
          assetBundle = request.assetBundle;
        }
        else {
          // This case happens when we have the bundles embeded in the apk file on Android. In this case we can't use LoadFromFileAsync directly.
          WWW www = WWW.LoadFromCacheOrDownload(assetBundlePath, 0);
          yield return www;
          assetBundle = www.assetBundle;
          www.Dispose();
        }

        _InProgressAssetBundles.Remove(assetBundleName);

        if (assetBundle != null) {
          // The bundle was loaded. Track it.
          loadedAssetBundle = new LoadedAssetBundle(assetBundle);
          _LoadedAssetBundles.Add(assetBundleName, loadedAssetBundle);
          PrintLoadedBundleInfo();

          yield return LoadAssetBundleDependenciesAsync(assetBundleName, loadedAssetBundle);

        }
        else {
          Log(LogType.Error, "Couldn't load asset bundle " + assetBundleName);
        }

        CallCallback(callback, assetBundle != null);
      }

      private IEnumerator LoadAssetBundleDependenciesAsync(string assetBundleName, LoadedAssetBundle loadedAssetBundle) {
        if (_AssetBundleManifest == null) {
          // The manifest can still be null as we are loading its bundle itself.
          yield break;
        }

        loadedAssetBundle.Dependencies = _AssetBundleManifest.GetAllDependencies(assetBundleName);
        if (loadedAssetBundle.Dependencies.Length == 0) {
          yield break;
        }

        Log(LogType.Log, "Loading dependencies for asset bundle " + assetBundleName);

        // Record and load all dependencies.
        int loadedDependencies = 0;
        for (int i = 0; i < loadedAssetBundle.Dependencies.Length; i++) {
          loadedAssetBundle.Dependencies[i] = RemapVariantName(loadedAssetBundle.Dependencies[i]);
          string dependencyName = loadedAssetBundle.Dependencies[i];
          Log(LogType.Log, "Loading asset bundle " + dependencyName + " which is a dependency of " + assetBundleName);

          StartCoroutine(LoadAssetBundleAsyncInternal(loadedAssetBundle.Dependencies[i], (bool successful) => {
            loadedDependencies++;

            if (!successful) {
              Log(LogType.Error, "Couldn't load asset bundle " + dependencyName + " which is a dependency of " + assetBundleName);
            }
          }));
        }

        // Wait until allt the loading coroutines have finished
        yield return new WaitWhile(() => {
          return loadedDependencies < loadedAssetBundle.Dependencies.Length;
        });
      }

      private void UnloadAssetBundleInternal(string assetBundleName, bool destroyObjectsCreatedFromBundle) {
        LoadedAssetBundle loadedAssetBundle = null;
        if (_LoadedAssetBundles.TryGetValue(assetBundleName, out loadedAssetBundle)) {
          loadedAssetBundle.OnUnloaded();

          if (loadedAssetBundle.ReferenceCount == 0) {
            // Unload the bundle itself
            loadedAssetBundle.AssetBundle.Unload(destroyObjectsCreatedFromBundle);
            _LoadedAssetBundles.Remove(assetBundleName);
            Log(LogType.Log, "Unloaded asset bundle " + assetBundleName);
            PrintLoadedBundleInfo();

            // Unload the bundle dependencies
            if (loadedAssetBundle.Dependencies.Length != 0) {

              for (int i = 0; i < loadedAssetBundle.Dependencies.Length; i++) {
                Log(LogType.Log, "Try unloading " + loadedAssetBundle.Dependencies[i] + " which " + assetBundleName + " depends on");
                UnloadAssetBundleInternal(loadedAssetBundle.Dependencies[i], destroyObjectsCreatedFromBundle);
              }
            }
          }
        }
        else {
          Log(LogType.Error, "Couldn't unload bundle " + assetBundleName + " because it is not loaded");
        }

        PrintLoadedBundleInfo();
      }

      private IEnumerator LoadAssetAsyncInternal<AssetType>(string assetBundleName, string assetName, Action<AssetType> callback) where AssetType : UnityEngine.Object {
        LoadedAssetBundle loadedAssetBundle = null;
        if (!_LoadedAssetBundles.TryGetValue(assetBundleName, out loadedAssetBundle) || (loadedAssetBundle == null)) {
          Log(LogType.Error, "Couldn't load asset " + assetName + " from asset bundle " + assetBundleName + ". The asset bundle is not loaded");
          CallCallback(callback, null);
          yield break;
        }

        if (loadedAssetBundle.AssetBundle == null) {
          Log(LogType.Error, "Couldn't load asset " + assetName + " from asset bundle " + assetBundleName + ". The asset bundle is null");
          CallCallback(callback, null);
          yield break;
        }

        AssetBundleRequest request = loadedAssetBundle.AssetBundle.LoadAssetAsync<AssetType>(assetName);
        if (request == null) {
          Log(LogType.Error, "Couldn't load asset " + assetName + " from asset bundle " + assetBundleName + ". Request to load failed");
          CallCallback(callback, null);
          yield break;
        }
        yield return request;

        if (request.asset == null) {
          Log(LogType.Error, "Couldn't load asset " + assetName + " from asset bundle " + assetBundleName + ". The request returned a null asset");
          CallCallback(callback, null);
          yield break;
        }

        CallCallback(callback, request.asset as AssetType);
      }

#if UNITY_EDITOR
      private IEnumerator LoadSceneAsyncInternalInEditor(string assetBundleName, string sceneName, bool loadAdditively, Action<bool> callback) {
        string[] scenePaths = UnityEditor.AssetDatabase.GetAssetPathsFromAssetBundleAndAssetName(assetBundleName, sceneName);
        if (scenePaths.Length == 0) {
          Log(LogType.Error, "Couldn't load scene " + sceneName + " from asset bundle " + assetBundleName);
          CallCallback(callback, false);
          yield break;
        }

        AsyncOperation operation;
        if (loadAdditively)
          operation = UnityEditor.EditorApplication.LoadLevelAdditiveAsyncInPlayMode(scenePaths[0]);
        else
          operation = UnityEditor.EditorApplication.LoadLevelAsyncInPlayMode(scenePaths[0]);

        yield return operation;

        CallCallback(callback, true);
      }
#endif

      private IEnumerator LoadSceneAsyncInternal(string assetBundleName, string sceneName, bool loadAdditively, Action<bool> callback) {
        LoadedAssetBundle loadedAssetBundle = null;
        if (!_LoadedAssetBundles.TryGetValue(assetBundleName, out loadedAssetBundle)) {
          Log(LogType.Error, "Couldn't load scene " + sceneName + " from asset bundle " + assetBundleName + ". The asset bundle is not loaded");
          CallCallback(callback, false);
          yield break;
        }

        LoadSceneMode loadMode = (loadAdditively) ? LoadSceneMode.Additive : LoadSceneMode.Single;
        AsyncOperation operation = SceneManager.LoadSceneAsync(sceneName, loadMode);
        yield return operation;

        CallCallback(callback, true);
      }

      private IEnumerator UnloadUnusedAssets(Action<bool> callback) {
        AsyncOperation operation = Resources.UnloadUnusedAssets();
        yield return operation;

        CallCallback(callback, true);
      }

      private static void Log(LogType logType, string text) {
        if (_sIsLogEnabled) {
          string message = "[AssetManager] " + text;

          switch (logType) {
          case LogType.Log:
            DAS.Debug("[AssetManager]", message);
            break;

          case LogType.Warning:
            DAS.Warn("[AssetManager]", message);
            break;

          case LogType.Error:
            DAS.Error("[AssetManager]", message);
            break;
          }
        }
      }

      private void PrintLoadedBundleInfo() {
        System.Text.StringBuilder sb = new System.Text.StringBuilder("Loaded bundles:\n");
        foreach (var pair in _LoadedAssetBundles) {
          sb.Append(pair.Key + " (" + pair.Value.ReferenceCount + ")\n");
        }
        Log(LogType.Log, sb.ToString());
      }

      private void PrintActiveVariants() {
        Log(LogType.Log, ActiveVariantsToString());
      }

      public string ActiveVariantsToString() {
        System.Text.StringBuilder sb = new System.Text.StringBuilder("Active variants: ");
        foreach (var variant in _ActiveVariants) {
          sb.Append(variant + ", ");
        }
        return sb.ToString();
      }

      private void CallCallback<ParameterType>(Action<ParameterType> callback, ParameterType value) {
        if (callback != null) {
          callback(value);
        }
      }

      #endregion
    }

    /// <summary>
    /// Unity expects asset bundle names to be all_lowercase so we are keeping that 
    /// expectation here.
    /// </summary>
    public enum AssetBundleNames {
      minigame_metadata,
      minigame_data_prefabs,
      minigame_ui_prefabs,
      basic_ui_prefabs,
      minigame_ui_sprites,
      basic_ui_sprites,
      debug_prefabs,
      debug_assets,
      home_view_prefabs,
      home_view_ui_sprites,
      loot_view_prefabs,
      loot_view_ui_sprites,
      start_view_prefabs,
      start_view_ui_sprites,
      main_assets,
      checkin_ui_sprites,
      checkin_ui_prefabs,
      onboarding_ui_sprites,
      onboarding_ui_prefabs,
      platform_specific_start_view_sprites,
      needs_hub_connection,
      needs_hub_view,
      activities_view
    }

    [Serializable]
    public class SerializableAssetBundleNames : SerializableEnum<AssetBundleNames> {

    }

    [Serializable]
    public class AssetBundleAssetLink<T> where T : UnityEngine.Object {
      [SerializeField]
      private SerializableAssetBundleNames _AssetBundle;

      public string AssetBundle {
        get { return _AssetBundle.Value.ToString(); }
      }

      [SerializeField]
      private string _AssetDataName;

      public void LoadAssetData(Action<T> dataLoadedCallback) {
        AssetBundleManager.Instance.LoadAssetAsync<T>(
          _AssetBundle.Value.ToString(), _AssetDataName,
          (T dataInstance) => {
            dataLoadedCallback(dataInstance);
          });
      }
    }

    [Serializable]
    public class GameObjectDataLink : AssetBundleAssetLink<GameObject> {

    }
  }
}