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
  private string[] _assetBundlesToLoad;

  // Use this for initialization
  private IEnumerator Start() {
    if (Application.isPlaying) {
      // Set up localization files.
      Localization.LoadStrings();

      // Add managers to this object here
      // gameObject.AddComponent<ManagerTypeName>();
      gameObject.AddComponent<HockeyAppManager>();
      gameObject.AddComponent<ObjectTagRegistryManager>();
    }

    AssetBundleManager.IsLogEnabled = true;
      
    bool initializedAssetManager = false;
    AssetBundleManager assetBundleManager = gameObject.AddComponent<AssetBundleManager>();
    assetBundleManager.Initialize((success) => initializedAssetManager = true);

    while (!initializedAssetManager) {
      yield return 0;
    }

    // TODO: Pick sd or hd based on device
    assetBundleManager.AddActiveVariant("hd");
      
    foreach (string assetBundleName in _assetBundlesToLoad) {
      bool loadedAssetBundle = false;
      assetBundleManager.LoadAssetBundleAsync(assetBundleName, 
        (success) => {
          if (!success) {
            DAS.Error("StartupManager.Awake.AssetBundleLoading", 
              string.Format("Failed to load Asset Bundle with name={0}", assetBundleName));
          }
          loadedAssetBundle = true;
        });

      while (!loadedAssetBundle) {
        yield return 0;
      }
    }
  }
}
