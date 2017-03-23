using UnityEngine;
using System.Collections;
using TMPro;
using Anki.Core.UI.Components;

namespace Anki.Core.UI.Components {
  public class AnkiTextMeshPro : TextMeshProUGUI, ISkinnableComponent {
    #region Properties
    [SerializeField]
    protected string _LinkedComponentId = null;
    public virtual string LinkedComponentId {
      get {
        return _LinkedComponentId;
      }

      set {
        _LinkedComponentId = value;
      }
    }

    [SerializeField]
    protected bool _OverrideSkinFont = false;
    public virtual bool OverrideSkinFont {
      get {
        return _OverrideSkinFont;
      }

      set {
        _OverrideSkinFont = value;
      }
    }

    [SerializeField]
    protected bool _OverrideSkinFaceTint = false;
    public virtual bool OverrideSkinFaceTint {
      get {
        return _OverrideSkinFaceTint;
      }

      set {
        _OverrideSkinFaceTint = value;
      }
    }

    [SerializeField]
    protected bool _OverrideOutlineColor = false;
    public virtual bool OverrideOutlineColor {
      get {
        return _OverrideOutlineColor;
      }

      set {
        _OverrideOutlineColor = value;
      }
    }

    [SerializeField]
    protected bool _OverrideOutlineThickness = false;
    public virtual bool OverrideOutlineThickness {
      get {
        return _OverrideOutlineThickness;
      }

      set {
        _OverrideOutlineThickness = value;
      }
    }

    [SerializeField]
    protected bool _OverrideSkinFontSize = false;
    public virtual bool OverrideSkinFontSize {
      get {
        return _OverrideSkinFontSize;
      }

      set {
        _OverrideSkinFontSize = value;
      }
    }


    [SerializeField]
    protected bool _OverrideSkinFontStyle = false;
    public virtual bool OverrideSkinFontStyle {
      get {
        return _OverrideSkinFontStyle;
      }
      set {
        _OverrideSkinFontStyle = value;
      }
    }

    [SerializeField]
    protected bool _OverrideSkinTextAlignment = false;
    public virtual bool OverrideSkinTextAlignment {
      get {
        return _OverrideSkinTextAlignment;
      }
      set {
        _OverrideSkinTextAlignment = value;
      }
    }

    [SerializeField]
    protected bool _OverrideSkinTextOverflow = false;
    public virtual bool OverrideSkinTextOverflow {
      get {
        return _OverrideSkinTextOverflow;
      }
      set {
        _OverrideSkinTextOverflow = value;
      }
    }

    [SerializeField]
    protected bool _OverrideSkinAutoSizing = false;
    public virtual bool OverrideSkinAutoSizing {
      get {
        return _OverrideSkinAutoSizing;
      }
      set {
        _OverrideSkinAutoSizing = value;
      }
    }

    public virtual bool IsPreview {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return false;
        }
        return _ISkinnableMetaObj.IsPreview;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.IsPreview = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual string PreviewThemeIdShowing {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return string.Empty;
        }

        return _ISkinnableMetaObj.PreviewThemeIdShowing;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.PreviewThemeIdShowing = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual string PreviewSkinIdShowing {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return string.Empty;
        }
        return _ISkinnableMetaObj.PreviewSkinIdShowing;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.PreviewSkinIdShowing = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual string PreviewComponentIdShowing {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return string.Empty;
        }
        return _ISkinnableMetaObj.PreviewComponentIdShowing;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.PreviewComponentIdShowing = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual bool ShowRuntimeSkinningOptions {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return false;
        }
        return _ISkinnableMetaObj.ShowRuntimeSkinningOptions;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.ShowRuntimeSkinningOptions = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual bool ShowEditorSkinningOptions {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return false;
        }
        return _ISkinnableMetaObj.ShowEditorSkinningOptions;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.ShowEditorSkinningOptions = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual bool ShowSavingSkinningOptions {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return false;
        }
        return _ISkinnableMetaObj.ShowSavingSkinningOptions;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.ShowSavingSkinningOptions = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual ThemesJson.ComponentTypes MyType {
      get {
        return ThemesJson.ComponentTypes.TextMeshPro;
      }
    }

    public virtual Color SerializedFaceColor {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return Color.white;
        }
        return _ISkinnableMetaObj.SerializedColor;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.SerializedColor = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual Color SerializedOutlineColor {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return Color.white;
        }
        return _ISkinnableMetaObj.SerializeOutlineColor;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.SerializeOutlineColor = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual float SerializedOutlineThickness {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return 0;
        }
        return _ISkinnableMetaObj.SerializeOutlineThickness;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.SerializeOutlineThickness = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    public virtual int SerializeFontSize {
      get {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          return 0;
        }
        return _ISkinnableMetaObj.SerializeFontSize;
      }

      set {
        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = SkinnableMetaJson.LoadJson(GetInstanceID().ToString());
        }

        if (_ISkinnableMetaObj == null) {
          _ISkinnableMetaObj = new SkinnableMetaJson();
        }
        _ISkinnableMetaObj.SerializeFontSize = value;
        SkinnableMetaJson.SaveJson(GetInstanceID().ToString(), _ISkinnableMetaObj);
      }
    }

    #endregion

    #region Protected Variables
    protected SkinnableMetaJson _ISkinnableMetaObj = null;
    #endregion

    #region Unity Methods
    protected override void Start() {
      base.Start();

      if (Application.isPlaying && !string.IsNullOrEmpty(ThemeSystemConfigJson.CurrentlyLoadedInstance.CurrentThemeId) &&
          !string.IsNullOrEmpty(ThemeSystemConfigJson.CurrentlyLoadedInstance.CurrentSkinId)) {
        UpdateSkinnableElements(ThemesJson.GetCurrentTheme().Id, ThemesJson.GetCurrentThemeSkin().Id);
      }

    }
    #endregion

    #region Helper Methods
    public virtual void UpdateSkinnableElements(string themeId, string skinId) {
      //Make sure we have a linkage to a skin component
      if (!string.IsNullOrEmpty(_LinkedComponentId)) {
        //We do, so we need to apply our tint and our image if we are not overriding with what is set in the prefab

        //Lets grab our linked component
        ThemesJson.ThemeComponentObj linkedComponentObj = ThemesJson.GetComponentById(themeId, skinId, _LinkedComponentId);

        //Make sure we actually have that component in the current skin(if not we just load what is set in the prefab and print a warning)
        if (linkedComponentObj != null) {
          //Confirm that we want to skin this at the prefab level and that this element is supposed to be skinned at the theme level
          if (!_OverrideSkinFont && linkedComponentObj.SkinFont) {
            SkinFont(linkedComponentObj);
          }

          if (!_OverrideSkinFaceTint && linkedComponentObj.SkinFontColor) {
            SkinFontTint(linkedComponentObj);
          }

          if (!_OverrideOutlineColor && linkedComponentObj.SkinTMPOutlineColor) {
            SkinOutlineColor(linkedComponentObj);
          }

          if (!_OverrideOutlineThickness && linkedComponentObj.SkinTMPOutlineThickness) {
            SkinOutlineThickness(linkedComponentObj);
          }

          if (!_OverrideSkinFontSize && linkedComponentObj.SkinFontSize) {
            SkinFontSize(linkedComponentObj);
          }

          if (!_OverrideSkinFontStyle && linkedComponentObj.SkinFontStyle) {
            SkinFontStyle(linkedComponentObj);
          }

          if (!_OverrideSkinTextAlignment && linkedComponentObj.SkinTextAlignment) {
            SkinTextAlignment(linkedComponentObj);
          }

          if (!_OverrideSkinTextOverflow && linkedComponentObj.SkinTextOverflow) {
            SkinTextOverflow(linkedComponentObj);
          }

          if (!_OverrideSkinAutoSizing && linkedComponentObj.SkinAutoSizing) {
            SkinAutoSizing(linkedComponentObj);
          }
        }
      }
    }

    protected virtual void SkinFont(ThemesJson.ThemeComponentObj linkedComponentObj) {
      //Load font from skin
      TMP_FontAsset skinFont = ThemeSystemUtils.sInstance.LoadFontTMP(linkedComponentObj.FontResourceKey) as TMP_FontAsset;
      //Apply to our font
      this.font = skinFont;
    }

    protected virtual void SkinFontTint(ThemesJson.ThemeComponentObj linkedComponentObj) {
      //Apply tint from skin
      this.color = linkedComponentObj.FontColor;
    }

    protected virtual void SkinOutlineColor(ThemesJson.ThemeComponentObj linkedComponentObj) {
      this.outlineColor = linkedComponentObj.TMPOutlineColor;
    }

    protected virtual void SkinOutlineThickness(ThemesJson.ThemeComponentObj linkedComponentObj) {
      this.outlineWidth = linkedComponentObj.TMPOutlineThickness;
    }

    protected virtual void SkinFontSize(ThemesJson.ThemeComponentObj linkedComponentObj) {
      //Load size from skin
      int fontOffset = linkedComponentObj.FontSizeOffset;
      //Apply to our fontsize
      this.fontSize += fontOffset;
      //Make sure we are between 1-1000
      this.fontSize = Mathf.Clamp(this.fontSize, 1, 1000);
    }

    protected virtual void SkinFontStyle(ThemesJson.ThemeComponentObj linkedComponentObj) {
      this.fontStyle = (FontStyles)linkedComponentObj.FontStyle;
    }

    protected virtual void SkinTextAlignment(ThemesJson.ThemeComponentObj linkedComponentObj) {
      this.alignment = (TextAlignmentOptions)linkedComponentObj.TextAlignment;
    }

    protected virtual void SkinTextOverflow(ThemesJson.ThemeComponentObj linkedComponentObj) {
      this.overflowMode = (TextOverflowModes)linkedComponentObj.TextOverflow;
    }
    protected virtual void SkinAutoSizing(ThemesJson.ThemeComponentObj linkedComponentObj) {
      this.enableAutoSizing = linkedComponentObj.EnableAutoSizing;
      this.fontSizeMin = linkedComponentObj.AutoSizingMin;
      this.fontSizeMax = linkedComponentObj.AutoSizingMax;
    }
    #endregion
  }
}
