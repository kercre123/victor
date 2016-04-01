using UnityEngine;
using System.Collections;
using Anki.Assets;
using Cozmo;

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
    AssetBundleManager.Instance.LoadAssetAsync<ShaderHolder>(CozmoAssetBundleNames.BasicUIPrefabsBundleName, 
      "ShaderHolder", (ShaderHolder sh) => {
      ShaderHolder.SetInstance(sh);
    });
    
  }
}
