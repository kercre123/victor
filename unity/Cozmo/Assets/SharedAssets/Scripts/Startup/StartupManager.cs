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

  // TODO: For now we're loading all asset bundles
  // In the future we can add a loading screen between hub/minigame
  // to load different UI
  [SerializeField]
  private string[] _AssetBundlesToLoad;

  [SerializeField]
  private string _DebugAssetBundleName;

  [SerializeField]
  private string[] _StartupDebugPrefabNames;

  private bool _IsDebugBuild = false;

  // Use this for initialization
  private IEnumerator Start() {
    // Load asset bundler
    AssetBundleManager.IsLogEnabled = true;
      
    bool initializedAssetManager = false;
    AssetBundleManager assetBundleManager = gameObject.AddComponent<AssetBundleManager>();
    assetBundleManager.Initialize((success) => initializedAssetManager = true);

    while (!initializedAssetManager) {
      yield return 0;
    }

    // INGO: QA is testing on a release build so manually set to true for now
    // _IsDebugBuild = Debug.isDebugBuild;
    _IsDebugBuild = true;

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

    // Set up localization files and add managers
    if (Application.isPlaying) {
      Localization.LoadStrings();
      AddComponents();
    }

    // TODO: Pick sd or hd based on device
    assetBundleManager.AddActiveVariant("hd");

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

    // Instantiate startup assets
    LoadAssets();
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

  private void LoadAssets() {
    // TODO: Don't hardcode this?
    AssetBundleManager.Instance.LoadAssetAsync<Cozmo.ShaderHolder>(CozmoAssetBundleNames.BasicUIPrefabsBundleName, 
      "ShaderHolder", (Cozmo.ShaderHolder sh) => {
      Cozmo.ShaderHolder.SetInstance(sh);
    });

    AssetBundleManager.Instance.LoadAssetAsync<Cozmo.UI.AlertViewLoader>(CozmoAssetBundleNames.BasicUIPrefabsBundleName, 
      "AlertViewLoader", (Cozmo.UI.AlertViewLoader avl) => {
      Cozmo.UI.AlertViewLoader.SetInstance(avl);
    });

    AssetBundleManager.Instance.LoadAssetAsync<Cozmo.UI.UIColorPalette>(CozmoAssetBundleNames.BasicUIPrefabsBundleName, 
      "UIColorPalette", (Cozmo.UI.UIColorPalette colorP) => {
      Cozmo.UI.UIColorPalette.SetInstance(colorP);
    });

    AssetBundleManager.Instance.LoadAssetAsync<Cozmo.UI.ProgressionStatConfig>(CozmoAssetBundleNames.GameMetadataBundleName, 
      "ProgressionStatConfig", (Cozmo.UI.ProgressionStatConfig psc) => {
      psc.Initialize();
      Cozmo.UI.ProgressionStatConfig.SetInstance(psc);
    });

    AssetBundleManager.Instance.LoadAssetAsync<Cozmo.CubePalette>(CozmoAssetBundleNames.GameMetadataBundleName, 
      "CubePalette", (Cozmo.CubePalette cp) => {
      Cozmo.CubePalette.SetInstance(cp);
    });

    AssetBundleManager.Instance.LoadAssetAsync<Cozmo.UI.MinigameUIPrefabHolder>(CozmoAssetBundleNames.MinigameUIPrefabsBundleName, 
      "MinigameUIPrefabHolder", (Cozmo.UI.MinigameUIPrefabHolder mph) => {
      Cozmo.UI.MinigameUIPrefabHolder.SetInstance(mph);
    });
  }
}
