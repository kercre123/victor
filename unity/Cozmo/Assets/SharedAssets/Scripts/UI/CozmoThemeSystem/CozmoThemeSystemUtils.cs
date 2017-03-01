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
    return Resources.Load<Sprite>(key);
  }

  public override Font LoadFont(string key) {
    if (Application.isPlaying) {
      Font fontAsset = Anki.Assets.AssetBundleManager.Instance.LoadAsset<Font>("fonts.en-us", System.IO.Path.GetFileNameWithoutExtension(key));
      if (fontAsset != null) {
        return fontAsset;
      }
      else {
        DAS.Warn("CozmoThemeSystemUtil.LoadFont.FontNotFound", System.IO.Path.GetFileNameWithoutExtension(key) + " font must be in fonts bundle.");
      }
    }
    else {
#if UNITY_EDITOR
      // not doing return UnityEditor.AssetDatabase.LoadAssetAtPath<Font>(key) directly
      // because that would cause unity to complain that the return null at the end is
      // un-reachable which is not always true depending on pre-processor directives.
      Font fontAsset = UnityEditor.AssetDatabase.LoadAssetAtPath<Font>(key);
      if (fontAsset != null) {
        return fontAsset;
      }
#endif
    }
    return null;
  }
  #endregion
}
