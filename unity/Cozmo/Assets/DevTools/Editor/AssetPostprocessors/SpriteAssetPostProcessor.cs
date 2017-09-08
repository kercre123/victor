using UnityEngine;
using UnityEditor;
using System.IO;

/// <summary>
/// Sets Cozmo default import settings for sprites in AssetBundles/Art folder.
/// If in a bundle folder suffixed by -UHD, will generate mirror assets in
/// other bundle folders in the same position with the same name in -HD and -SD 
/// but at a smaller size.
/// </summary>
public class SpriteAssetPostProcessor : AssetPostprocessor {

  private const string _kNeedsHubParentFolder = "AssetBundles/NeedsHub/Art";
  private const string _kHomeHubParentFolder = "AssetBundles/HomeHub/Art";

  // Originally Packed indicated assets that would go into Atlases 
  // Now nothing is packed; these folders will take on project-wide settings
  // for all UI sprites
  private const string _kPackSpriteFolderName = "Packed";
  private const string _kUnpackSpriteFolderName = "Unpacked";

  // Sets project-wide settings for textures for use in materials
  private const string _kTexturesFolderName = "Textures";
  // Same as above, except sets wrap mode to "repeat" instead of "clamp"
  private const string _kRepeatTexturesFolderName = "RepeatTextures";

  // Keeps all the same import settings as UHD asset, but still downsizes correctly
  private const string _kCustomFolderName = "Custom";

  private const string _kUHDBundleTag = "UHD";
  private const string _kHDBundleTag = "HD";
  private const string _kSDBundleTag = "SD";

  private const string _kAtlasSuffix = "Atlas";
  private const string _kGraphicSuffix = ".png";

  private const float _kSpritePixelsPerUnit = 100f;
  private const int _kMaxTextureSize = 2048;
  private const int _kAnisoLevel = 16;

  private const float _kHDScaleFactor = 0.5f;
  private const float _kSDScaleFactor = 0.35f;

  static void OnPostprocessAllAssets(string[] importedAssets, string[] deletedAssets, string[] movedAssets, string[] movedFromAssetPaths) {
    // TODO handle materials
  }

  private void OnPreprocessTexture() {
    if (IsUISprite()) {
      PreprocessUISprite();
    }
    else if (IsTexture()) {
      PreprocessTexture(useRepeatSetting: false);
    }
    else if (IsRepeatTexture()) {
      PreprocessTexture(useRepeatSetting: true);
    }
    else if (IsCustomTexture()) {
      PreprocessCustomGraphic();
    }
    else {
      Debug.Log("Detected texture import at " + assetPath + " but was not part of 'Packed', 'Unpacked', or 'Textures' folder in"
                + "Assets/AssetBundles/Art. Ignoring.");
    }
  }

  private void PreprocessUISprite() {
    Debug.Log("Detected texture import at " + assetPath + ". Setting default values.");

    TextureImporter textureImporter = (TextureImporter)assetImporter;
    textureImporter.textureType = TextureImporterType.Sprite;
    textureImporter.spriteImportMode = SpriteImportMode.Single;

    // COZMO-14363: Not packing sprites so that less memory will be loaded in at runtime for low-end
    // devices at a small framerate hit.
    //if (ShouldPackSprite()) {
    //  textureImporter.spritePackingTag = GenerateSpritePackingTag();
    //}
    //else {
    //  textureImporter.spritePackingTag = null;
    //}
    textureImporter.spritePackingTag = null;

    textureImporter.spritePixelsPerUnit = GetPixelsPerUnit();
    textureImporter.mipmapEnabled = false;
    textureImporter.filterMode = FilterMode.Bilinear;
    textureImporter.maxTextureSize = _kMaxTextureSize;
    textureImporter.textureCompression = TextureImporterCompression.Uncompressed;

    //if our source sprite is FullRect, our copies should be as well
    bool setSpriteMeshTypeToFullRect = false;
    if (!IsUHDAsset()) {
      textureImporter.spriteBorder = GetBorderFromUHDSprite(out setSpriteMeshTypeToFullRect);
    }

    // This is required for textures to be packed.
    textureImporter.isReadable = false;

    TextureImporterSettings tis = new TextureImporterSettings();
    textureImporter.ReadTextureSettings(tis);
    tis.spriteAlignment = (int)SpriteAlignment.Center;

    if (setSpriteMeshTypeToFullRect) {
      tis.spriteMeshType = SpriteMeshType.FullRect;
    }

    textureImporter.SetTextureSettings(tis);
  }

  private float GetPixelsPerUnit() {
    float ppu = _kSpritePixelsPerUnit;
    if (IsSDAsset()) {
      ppu *= _kSDScaleFactor;
    }
    else if (IsHDAsset()) {
      ppu *= _kHDScaleFactor;
    }
    return ppu;
  }

  private string GetUHDAssetPath() {
    string uhdAssetPath = null;
    if (IsHDAsset()) {
      uhdAssetPath = assetPath.Replace(_kHDBundleTag, _kUHDBundleTag);
    }
    else if (IsSDAsset()) {
      uhdAssetPath = assetPath.Replace(_kSDBundleTag, _kUHDBundleTag);
    }
    return uhdAssetPath;
  }

  private Vector4 GetBorderFromUHDSprite(out bool uhdSpriteIsFullRect) {
    uhdSpriteIsFullRect = false;
    Vector4 border = Vector4.one;
    string uhdAssetPath = GetUHDAssetPath();
    float borderScale = 1f;
    if (IsHDAsset()) {
      borderScale = _kHDScaleFactor;
    }
    else if (IsSDAsset()) {
      borderScale = _kSDScaleFactor;
    }

    if (!string.IsNullOrEmpty(uhdAssetPath)) {
      Sprite uhdSprite = AssetDatabase.LoadAssetAtPath(uhdAssetPath, typeof(Sprite)) as Sprite;
      if (uhdSprite != null) {
        border = uhdSprite.border * borderScale;

        //if our source sprite is FullRect, our copies should be as well
        TextureImporter textureImporter = AssetImporter.GetAtPath(uhdAssetPath) as TextureImporter;
        if (textureImporter != null) {
          TextureImporterSettings tis = new TextureImporterSettings();
          textureImporter.ReadTextureSettings(tis);
          uhdSpriteIsFullRect = (tis.spriteMeshType == SpriteMeshType.FullRect);
        }
      }
      else {
        Debug.LogError("Tried to set spriteBorder but failed: Could not find UHD asset for sprite at path " + assetPath + " at UHD path " + uhdAssetPath);
      }
    }
    return border;
  }

  private void PreprocessTexture(bool useRepeatSetting) {
    Debug.Log("Detected texture import at " + assetPath + ". Setting default values.");

    TextureImporter textureImporter = (TextureImporter)assetImporter;
    textureImporter.textureType = TextureImporterType.Default;
    textureImporter.alphaSource = TextureImporterAlphaSource.FromInput;
    textureImporter.alphaIsTransparency = true;
    textureImporter.wrapMode = (useRepeatSetting) ? TextureWrapMode.Repeat : TextureWrapMode.Clamp;
    textureImporter.filterMode = FilterMode.Bilinear;
    textureImporter.anisoLevel = _kAnisoLevel;
    textureImporter.maxTextureSize = _kMaxTextureSize;
    textureImporter.textureCompression = TextureImporterCompression.Uncompressed;
  }

  private void PreprocessCustomGraphic() {
    if (!IsUHDAsset()) {
      string uhdAssetPath = GetUHDAssetPath();

      // Copy over texture settings from UHD assets
      TextureImporter uhdTextureImporter = AssetImporter.GetAtPath(uhdAssetPath) as TextureImporter;
      if (uhdTextureImporter != null) {
        TextureImporterSettings uhdTis = new TextureImporterSettings();
        uhdTextureImporter.ReadTextureSettings(uhdTis);

        TextureImporter textureImporter = (TextureImporter)assetImporter;
        textureImporter.SetTextureSettings(uhdTis);
        textureImporter.spritePixelsPerUnit = GetPixelsPerUnit();

        // TODO: Handle downsizing borders for sliced sprites
      }
    }
  }

  private void OnPostprocessTexture(Texture2D texture) {
    if (IsUISprite() || IsTexture() || IsRepeatTexture() || IsCustomTexture()) {
      if (IsUHDAsset()) {
        Debug.Log("Detected UHD texture import at " + assetPath + ". Creating smaller versions in HD and SD folders.");
        CreateCopyOfTexture(texture, _kHDScaleFactor, _kHDBundleTag);
        CreateCopyOfTexture(texture, _kSDScaleFactor, _kSDBundleTag);
        AssetDatabase.Refresh();
      }
    }
  }

  private bool IsUISprite() {
    return (InHubParentFolder(assetPath) &&
        (assetPath.Contains(_kPackSpriteFolderName) || assetPath.Contains(_kUnpackSpriteFolderName))
            && assetPath.EndsWith(_kGraphicSuffix));
  }

  private bool ShouldPackSprite() {
    return InHubParentFolder(assetPath) && assetPath.Contains(_kPackSpriteFolderName);
  }

  private string GenerateSpritePackingTag() {
    string[] assetPathSplit = assetPath.Split(new char[] { '/' }, int.MaxValue);
    string bundleName = null;
    foreach (string section in assetPathSplit) {
      if (section.Contains(_kUHDBundleTag) || section.Contains(_kHDBundleTag) || section.Contains(_kSDBundleTag)) {
        bundleName = section;
        break;
      }
    }

    string packingTag = null;
    if (!string.IsNullOrEmpty(bundleName)) {
      packingTag = bundleName.Replace("-", "");
      packingTag += _kAtlasSuffix;
    }
    return packingTag;
  }

  private bool InHubParentFolder(string assetPath) {
    return assetPath.Contains(_kNeedsHubParentFolder) || assetPath.Contains(_kHomeHubParentFolder);
  }

  private bool IsUHDAsset() {
    return InHubParentFolder(assetPath) && assetPath.Contains(_kUHDBundleTag);
  }

  private bool IsHDAsset() {
    return InHubParentFolder(assetPath) && assetPath.Contains(_kHDBundleTag) && !assetPath.Contains(_kUHDBundleTag);
  }

  private bool IsSDAsset() {
    return InHubParentFolder(assetPath) && assetPath.Contains(_kSDBundleTag);
  }

  private void CreateCopyOfTexture(Texture2D baseTexture, float scale, string newBundleTag) {
    Texture2D newTexture = new Texture2D(baseTexture.width, baseTexture.height, baseTexture.format, baseTexture.mipmapCount > 1);
    newTexture.LoadRawTextureData(baseTexture.GetRawTextureData());
    newTexture.Apply();
    int newWidth = Mathf.FloorToInt(baseTexture.width * scale);
    if (newWidth <= 0) {
      Debug.LogError("Scaled image to " + scale + " for UHD asset at " + assetPath + " but base UHD image is too small! Clamping new texture width to 1.");
      newWidth = 1;
    }
    int newHeight = Mathf.FloorToInt(baseTexture.height * scale);
    if (newHeight <= 0) {
      Debug.LogError("Scaled image to " + scale + " for UHD a" +
                     "sset at " + assetPath + " but base UHD image is too small! Clamping new texture height to 1.");
      newHeight = 1;
    }

    TextureScale.Bilinear(newTexture, newWidth, newHeight);
    byte[] newTextureData = newTexture.EncodeToPNG();
    string newAssetPath = assetPath.Replace(_kUHDBundleTag, newBundleTag);
    bool forceReimport = File.Exists(newAssetPath);

    // if the directory does not exist create it
    System.IO.FileInfo file = new System.IO.FileInfo(newAssetPath);
    file.Directory.Create();

    File.WriteAllBytes(newAssetPath, newTextureData);
    if (forceReimport) {
      AssetDatabase.ImportAsset(newAssetPath);
    }
  }

  private bool IsTexture() {
    return InHubParentFolder(assetPath) && assetPath.Contains(_kTexturesFolderName)
                    && assetPath.EndsWith(_kGraphicSuffix) && !assetPath.Contains(_kRepeatTexturesFolderName);
  }

  private bool IsRepeatTexture() {
    return InHubParentFolder(assetPath) && assetPath.Contains(_kRepeatTexturesFolderName)
                    && assetPath.EndsWith(_kGraphicSuffix);
  }

  private bool IsCustomTexture() {
    return InHubParentFolder(assetPath) && assetPath.Contains(_kCustomFolderName)
                    && assetPath.EndsWith(_kGraphicSuffix);
  }
}
