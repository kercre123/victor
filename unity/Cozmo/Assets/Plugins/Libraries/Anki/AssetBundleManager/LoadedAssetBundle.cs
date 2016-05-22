using UnityEngine;
using System.Collections;

namespace Anki {
  namespace Assets {
    
    public class LoadedAssetBundle {
      private AssetBundle _AssetBundle;
      private int _ReferenceCount;
      private string[] _Dependencies;

      public AssetBundle AssetBundle {
        get {
          return _AssetBundle;
        }
      }

      public int ReferenceCount {
        get {
          return _ReferenceCount;
        }
      }

      public string[] Dependencies {
        get {
          return _Dependencies;
        }

        set {
          _Dependencies = value;
        }
      }

      public LoadedAssetBundle(AssetBundle assetBundle) {
        _AssetBundle = assetBundle;
        _ReferenceCount = 1;
      }

      public void OnLoaded() {
        _ReferenceCount++;
      }

      public void OnUnloaded() {
        _ReferenceCount--;
      }
    }
  }
}
