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

  // Use this for initialization
  private IEnumerator Start() {
    if (Application.isPlaying) {
      // Set up localization files.
      Localization.LoadStrings();
      AddComponents();
    }

    // Load asset bundler
    AssetBundleManager.IsLogEnabled = true;
      
    bool initializedAssetManager = false;
    AssetBundleManager assetBundleManager = gameObject.AddComponent<AssetBundleManager>();
    assetBundleManager.Initialize((success) => initializedAssetManager = true);

    while (!initializedAssetManager) {
      yield return 0;
    }

    // TODO: Pick sd or hd based on device
    assetBundleManager.AddActiveVariant("hd");

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

    LoadAssets();
  }

  private void AddComponents() {
    // Add managers to this object here
    // gameObject.AddComponent<ManagerTypeName>();
    gameObject.AddComponent<HockeyAppManager>();
    gameObject.AddComponent<ObjectTagRegistryManager>();
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
