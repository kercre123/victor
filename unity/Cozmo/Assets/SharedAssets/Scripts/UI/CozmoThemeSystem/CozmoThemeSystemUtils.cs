using UnityEngine;

public class CozmoThemeSystemUtils : Anki.Core.UI.Components.ThemeSystemUtils {

  #region Static Methods
  public static void Initialize() {
    //Sets our base instance to this, this will make all our override methods below work :)
    _sInstance = new CozmoThemeSystemUtils();
  }
  #endregion

  #region Override Methods
  public override Sprite LoadSprite(string key) {
    if (string.IsNullOrEmpty(key)) {
      return null;
    }
    // parse out the directory name
    string[] directoryNames = System.IO.Path.GetDirectoryName(key).Split('/');
    string assetBundleDirectoryName = directoryNames[directoryNames.Length - 2];
    // strip out the variant name
    int dashIndex = assetBundleDirectoryName.IndexOf("-");
    if (dashIndex > 0) {
      assetBundleDirectoryName = assetBundleDirectoryName.Substring(0, dashIndex);
    }

    // loads parsed key and asset bundle name
    if (Application.isPlaying) {
      return LoadAssetHelper<Sprite>(assetBundleDirectoryName + "." + StartupManager.Instance.GetVariantBasedOnScreenResolution(), key);
    }
    else {
      return LoadAssetHelper<Sprite>(assetBundleDirectoryName + ".uhd", key);
    }
  }

  public override Font LoadFont(string key) {
    if (string.IsNullOrEmpty(key)) {
      return null;
    }
    return LoadAssetHelper<Font>(GetFontBundleName(), key);
  }

  public override ScriptableObject LoadFontTMP(string key) {
    if (string.IsNullOrEmpty(key)) {
      return null;
    }
    return LoadAssetHelper<ScriptableObject>(GetFontBundleName(), key);
  }

  private string GetFontBundleName() {
    return "fonts." + Localization.GetStringsLocale().ToLower();
  }

  // generic asset loader that uses direct asset loading when in editor and asset bundle loading
  // when running.
  private T LoadAssetHelper<T>(string assetBundleName, string key) where T : UnityEngine.Object {
    if (Application.isPlaying) {
      T asset = Anki.Assets.AssetBundleManager.Instance.LoadAsset<T>(assetBundleName, System.IO.Path.GetFileNameWithoutExtension(key));
      if (asset != null) {
        return asset;
      }
      else {
        DAS.Warn("CozmoThemeSystemUtil.LoadAssetHelper.AssetNotFound", System.IO.Path.GetFileNameWithoutExtension(key) + " asset not found in " + assetBundleName + " bundle.");
      }
    }
    else {
#if UNITY_EDITOR
      // not doing return UnityEditor.AssetDatabase.LoadAssetAtPath<Font>(key) directly
      // because that would cause unity to complain that the return null at the end is
      // un-reachable which is not always true depending on pre-processor directives.
      T asset = UnityEditor.AssetDatabase.LoadAssetAtPath<T>(key);
      if (asset != null) {
        return asset;
      }
#endif
    }
    return null;
  }

  #endregion
}
