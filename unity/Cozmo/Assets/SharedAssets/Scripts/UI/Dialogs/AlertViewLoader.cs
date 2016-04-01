using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    [CreateAssetMenu]
    public class AlertViewLoader : ScriptableObject {

      private const string _kAlertViewLoaderAssetName = "AlertViewLoader";

      public delegate void AlertViewGetter(bool success,AlertViewLoader instance);

      private static AlertViewLoader _sInstance;

      private static event AlertViewGetter _sGetterCallbacks;

      public static void LoadInstance(AlertViewGetter callback) {
        if (_sInstance == null) {
          _sGetterCallbacks += callback;
          Anki.Assets.AssetBundleManager.Instance.LoadAssetBundleAsync(CozmoAssetBundleNames.BasicUIPrefabsBundleName, 
            HandleAssetBundleFinishedLoading);
        }
        else {
          if (callback != null) {
            callback(true, _sInstance);
          }
        }
      }

      private static void HandleAssetBundleFinishedLoading(bool success) {
        if (success) {
          Anki.Assets.AssetBundleManager.Instance.LoadAssetAsync<AlertViewLoader>(CozmoAssetBundleNames.BasicUIPrefabsBundleName, 
            _kAlertViewLoaderAssetName, HandleAssetFinishedLoading);
        }
        else {
          if (_sGetterCallbacks != null) {
            _sGetterCallbacks(false, null);
          }
        }
      }

      private static void HandleAssetFinishedLoading(AlertViewLoader loader) {
        // TODO: Handle loader == null?
        if (loader != null) {
          _sInstance = loader;
          if (_sGetterCallbacks != null) {
            _sGetterCallbacks(true, _sInstance);
          }
        }
      }

      [SerializeField]
      private AlertView _AlertViewPrefab;

      public AlertView AlertViewPrefab {
        get { return _AlertViewPrefab; }
      }

      [SerializeField]
      private AlertView _AlertViewPrefab_NoText;

      public AlertView AlertViewPrefab_NoText {
        get { return _AlertViewPrefab_NoText; }
      }

      [SerializeField]
      private AlertView _AlertViewPrefab_Icon;

      public AlertView AlertViewPrefab_Icon {
        get { return _AlertViewPrefab_Icon; }
      }
    }
  }
}