using UnityEngine;
using Anki.Assets;

namespace Cozmo {
  namespace UI {
    public class MinigameUIPrefabHolder : ScriptableObject {
      [SerializeField]
      private GameObjectDataLink _SharedMinigameViewPrefabData;

      public static void LoadSharedMinigameViewPrefab(string assetBundleName, System.Action<GameObject> dataLoadedCallback) {
        AssetBundleManager.Instance.LoadAssetAsync(
          assetBundleName, "MinigameUIPrefabHolder", (MinigameUIPrefabHolder prefabHolder) => {
            prefabHolder._SharedMinigameViewPrefabData.LoadAssetData(dataLoadedCallback);
          });
      }
    }
  }
}