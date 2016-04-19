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
    // Start loading bar at close to 0
    _CurrentProgress = 0.05f;
    _LoadingBar.SetProgress(_CurrentProgress);
    _CurrentNumDots = 0;
    StartCoroutine(UpdateLoadingDots());

    // Load asset bundler
    AssetBundleManager.IsLogEnabled = true;
      
    AssetBundleManager assetBundleManager = gameObject.AddComponent<AssetBundleManager>();
    yield return InitializeAssetBundleManager(assetBundleManager);

    AddLoadingBarProgress(0.1f);

    // INGO: QA is testing on a release build so manually set to true for now
    // _IsDebugBuild = Debug.isDebugBuild;
    _IsDebugBuild = true;

    yield return LoadDebugAssetBundle(assetBundleManager, _IsDebugBuild);

    AddLoadingBarProgress(0.1f);

    assetBundleManager.AddActiveVariant(GetActiveVariant());

    AddLoadingBarProgress(0.1f);

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

    _LoadingBar.SetProgress(1.0f);

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
    AnimationManager.Instance.Initialize();

    // Initialize persistance manager
    DataPersistence.DataPersistenceManager.InitializeInstance();
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

    assetBundleManager.LoadAssetAsync<Cozmo.UI.ProgressionStatConfig>(_GameMetadataAssetBundleName, 
      "ProgressionStatConfig", (Cozmo.UI.ProgressionStatConfig psc) => {
      psc.Initialize();
      Cozmo.UI.ProgressionStatConfig.SetInstance(psc);
    });

    assetBundleManager.LoadAssetAsync<Cozmo.CubePalette>(_GameMetadataAssetBundleName, 
      "CubePalette", (Cozmo.CubePalette cp) => {
      Cozmo.CubePalette.SetInstance(cp);
    });

    assetBundleManager.LoadAssetAsync<Cozmo.ItemDataConfig>(_GameMetadataAssetBundleName, 
      "ItemDataConfig", (Cozmo.ItemDataConfig idc) => {
      Cozmo.ItemDataConfig.SetInstance(idc);
    });
        
    assetBundleManager.LoadAssetAsync<ChestData>(_GameMetadataAssetBundleName, 
      "DefaultChestConfig", (ChestData cd) => {
      ChestData.SetInstance(cd);
    });

    assetBundleManager.LoadAssetAsync<Cozmo.HexItemList>(_GameMetadataAssetBundleName, 
      "HexItemList", (Cozmo.HexItemList cd) => {
      Cozmo.HexItemList.SetInstance(cd);
    });

    assetBundleManager.LoadAssetAsync<Cozmo.UI.MinigameUIPrefabHolder>(_MinigameUIPrefabAssetBundleName, 
      "MinigameUIPrefabHolder", (Cozmo.UI.MinigameUIPrefabHolder mph) => {
      Cozmo.UI.MinigameUIPrefabHolder.SetInstance(mph);
    });
  }

  private void LoadMainScene(AssetBundleManager assetBundleManager) {
    StopCoroutine(UpdateLoadingDots());
    assetBundleManager.LoadSceneAsync(_MainSceneAssetBundleName, _MainSceneName, loadAdditively: false, callback: null);
  }

  private IEnumerator UpdateLoadingDots() {
    string loadingText = "Loading";
    for (int i = 0; i < _CurrentNumDots; i++) {
      loadingText += ".";
    }
    _CurrentNumDots = (_CurrentNumDots + 1) % 4;
    _LoadingBarLabel.text = loadingText;
    yield return new WaitForSeconds(_AddDotSeconds);
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
}
