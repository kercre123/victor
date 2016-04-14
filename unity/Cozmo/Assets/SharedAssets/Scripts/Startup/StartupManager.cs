using UnityEngine;
using System.Collections;
using Anki.Assets;

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

  [SerializeField]
  private string[] _AssetBundlesToLoad;

  [SerializeField]
  private string _MainSceneAssetBundleName;

  [SerializeField]
  private string _MainSceneName;

  [SerializeField]
  private string _MinigameUIPrefabAssetBundleName;

  [SerializeField]
  private string _BasicUIPrefabAssetBundleName;

  [SerializeField]
  private string _GameMetadataAssetBundleName;

  [SerializeField]
  private string _DebugAssetBundleName;

  [SerializeField]
  private string[] _StartupDebugPrefabNames;

  private bool _IsDebugBuild = false;

  // Use this for initialization
  private IEnumerator Start() {
    // TODO: Start loading bar at 0

    // Load asset bundler
    AssetBundleManager.IsLogEnabled = true;
      
    AssetBundleManager assetBundleManager = gameObject.AddComponent<AssetBundleManager>();
    yield return InitializeAssetBundleManager(assetBundleManager);

    // INGO: QA is testing on a release build so manually set to true for now
    // _IsDebugBuild = Debug.isDebugBuild;
    _IsDebugBuild = true;

    yield return LoadDebugAssetBundle(assetBundleManager, _IsDebugBuild);

    assetBundleManager.AddActiveVariant(GetActiveVariant());

    // Load initial asset bundles
    yield return LoadAssetBundles(assetBundleManager);

    // Instantiate startup assets
    LoadAssets(assetBundleManager);

    // Set up localization files and add managers
    if (Application.isPlaying) {
      // TODO: Localization based on variant?
      Localization.LoadStrings();
      AddComponents();
    }

    // TODO: Destory loading bar

    // Load main scene
    LoadMainScene(assetBundleManager);
  }

  private IEnumerator InitializeAssetBundleManager(AssetBundleManager assetBundleManager) {
    bool initializedAssetManager = false;
    assetBundleManager.Initialize((success) => initializedAssetManager = true);

    while (!initializedAssetManager) {
      yield return 0;
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
            GameObject go = GameObject.Instantiate(prefab);
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

  private string GetActiveVariant() {
    // TODO: Pick sd or hd or uhd based on device
    return "hd";
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

  private void AddComponents() {
    // Add managers to this object here
    // gameObject.AddComponent<ManagerTypeName>();
    gameObject.AddComponent<HockeyAppManager>();
    gameObject.AddComponent<ObjectTagRegistryManager>();
    AnimationManager.Instance = new AnimationManager();

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

    assetBundleManager.LoadAssetAsync<Cozmo.UI.ProgressionStatConfig>(_GameMetadataAssetBundleName, 
      "ProgressionStatConfig", (Cozmo.UI.ProgressionStatConfig psc) => {
      psc.Initialize();
      Cozmo.UI.ProgressionStatConfig.SetInstance(psc);
    });

    assetBundleManager.LoadAssetAsync<Cozmo.CubePalette>(_GameMetadataAssetBundleName, 
      "CubePalette", (Cozmo.CubePalette cp) => {
      Cozmo.CubePalette.SetInstance(cp);
    });

    assetBundleManager.LoadAssetAsync<Cozmo.UI.MinigameUIPrefabHolder>(_MinigameUIPrefabAssetBundleName, 
      "MinigameUIPrefabHolder", (Cozmo.UI.MinigameUIPrefabHolder mph) => {
      Cozmo.UI.MinigameUIPrefabHolder.SetInstance(mph);
    });
  }

  private void LoadMainScene(AssetBundleManager assetBundleManager) {
    assetBundleManager.LoadSceneAsync(_MainSceneAssetBundleName, _MainSceneName, loadAdditively: false, callback: UnloadMainSceneAssetBundle);
  }

  private void UnloadMainSceneAssetBundle(bool successLoadScene) {
    // INGO: This seems to also unload all the dependencies...
    if (successLoadScene) {
      AssetBundleManager.Instance.UnloadAssetBundle(_MainSceneAssetBundleName, destroyObjectsCreatedFromBundle: false);
    }
    else {
      DAS.Error("StartupManager.UnloadMainSceneAssetBundle", "Could not load main scene!");
    }
  }
}
