using UnityEngine;
using UnityEditor;
using System.IO;

/// <summary>
/// Sets Cozmo default import settings for sprites in AssetBundles/Art folder
/// for textures located in Packed or Unpacked folders.
/// If in a Packed folder, will also update the packing tag to an atlas based on 
/// parent bundle folder's name.
/// If in a bundle folder suffixed by -UHD, will generate mirror assets in
/// other bundle folders in the same position with the same name in -HD and -SD 
/// but at half and quarter width (1/4 and 1/16 memory) size, respectively.
/// </summary>
public class SpriteAssetPostProcessor : AssetPostprocessor {

  private const string _kParentFolder = "AssetBundles/Art";
  private const string _kPackSpriteFolderName = "Packed";
  private const string _kUnpackSpriteFolderName = "Unpacked";
  private const string _kTexturesFolderName = "Textures";
  private const string _kRepeatTexturesFolderName = "RepeatTextures";

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

  private const float _kMinWidth = 200f;
  private const float _kMinHeight = 200f;

  static void OnPostprocessAllAssets(string[] importedAssets, string[] deletedAssets, string[] movedAssets, string[] movedFromAssetPaths) {
    // TODO handle materials
  }

  private void OnPreprocessTexture() {
    if (IsUISprite()) {
      PostprocessUISprite();
    }
    else if (IsTexture()) {
      PostprocessTexture(useRepeatSetting: false);
    }
    else if (IsRepeatTexture()) {
      PostprocessTexture(useRepeatSetting: true);
    }
    else {
      Debug.Log("Detected texture import at " + assetPath + " but was not part of 'Packed', 'Unpacked', or 'Textures' folder in"
                + "Assets/AssetBundles/Art. Ignoring.");
    }
  }

  private void PostprocessUISprite() {
    Debug.Log("Detected texture import at " + assetPath + ". Setting default values.");

    TextureImporter textureImporter = (TextureImporter)assetImporter;
    textureImporter.textureType = TextureImporterType.Sprite;
    textureImporter.spriteImportMode = SpriteImportMode.Single;

    if (ShouldPackSprite()) {
      textureImporter.spritePackingTag = GenerateSpritePackingTag();
    }
    else {
      textureImporter.spritePackingTag = null;
    }

    textureImporter.spritePixelsPerUnit = GetPixelsPerUnit();
    textureImporter.mipmapEnabled = false;
    textureImporter.filterMode = FilterMode.Bilinear;
    textureImporter.maxTextureSize = _kMaxTextureSize;
    textureImporter.textureFormat = TextureImporterFormat.AutomaticTruecolor;

    if (!IsUHDAsset()) {
      textureImporter.spriteBorder = GetBorderFromUHDSprite();
    }

    // This is required for textures to be packed.
    textureImporter.isReadable = false;

    TextureImporterSettings tis = new TextureImporterSettings();
    textureImporter.ReadTextureSettings(tis);
    tis.spriteAlignment = (int)SpriteAlignment.Center;

    textureImporter.SetTextureSettings(tis);
  }

  private float GetPixelsPerUnit() {
    float ppu = _kSpritePixelsPerUnit;
    string uhdAssetPath = null;
    if (IsSDAsset()) {
      ppu *= _kSDScaleFactor;
      uhdAssetPath = assetPath.Replace(_kHDBundleTag, _kUHDBundleTag);
    }
    else if (IsHDAsset()) {
      ppu *= _kHDScaleFactor;
      uhdAssetPath = assetPath.Replace(_kSDBundleTag, _kUHDBundleTag);
    }

    if (!string.IsNullOrEmpty(uhdAssetPath)) {
      Sprite uhdSprite = AssetDatabase.LoadAssetAtPath(uhdAssetPath, typeof(Sprite)) as Sprite;
      if (uhdSprite != null) {
        if ((uhdSprite.texture.height < _kMinHeight && uhdSprite.texture.width < _kMinWidth)) {
          // if we didn't downsample due to minimum sprite requirements then we shouldn't downsample the ppu either.
          ppu = _kSpritePixelsPerUnit;
        }
      }
    }
    return ppu;
  }

  private Vector4 GetBorderFromUHDSprite() {
    Vector4 border = Vector4.one;
    string uhdAssetPath = null;
    float borderScale = 1f;
    if (IsHDAsset()) {
      uhdAssetPath = assetPath.Replace(_kHDBundleTag, _kUHDBundleTag);
      borderScale = _kHDScaleFactor;
    }
    else if (IsSDAsset()) {
      uhdAssetPath = assetPath.Replace(_kSDBundleTag, _kUHDBundleTag);
      borderScale = _kSDScaleFactor;
    }

    if (!string.IsNullOrEmpty(uhdAssetPath)) {
      Sprite uhdSprite = AssetDatabase.LoadAssetAtPath(uhdAssetPath, typeof(Sprite)) as Sprite;
      if (uhdSprite != null) {
        if ((uhdSprite.texture.height < _kMinHeight && uhdSprite.texture.width < _kMinWidth)) {
          border = uhdSprite.border;
        }
        else {
          border = uhdSprite.border * borderScale;
        }
      }
      else {
        Debug.LogError("Tried to set spriteBorder but failed: Could not find UHD asset for sprite at path " + assetPath + " at UHD path " + uhdAssetPath);
      }
    }
    return border;
  }

  private void PostprocessTexture(bool useRepeatSetting) {
    Debug.Log("Detected texture import at " + assetPath + ". Setting default values.");

    TextureImporter textureImporter = (TextureImporter)assetImporter;
    textureImporter.textureType = TextureImporterType.Image;
    textureImporter.grayscaleToAlpha = false;
    textureImporter.alphaIsTransparency = true;
    textureImporter.wrapMode = (useRepeatSetting) ? TextureWrapMode.Repeat : TextureWrapMode.Clamp;
    textureImporter.filterMode = FilterMode.Bilinear;
    textureImporter.anisoLevel = _kAnisoLevel;
    textureImporter.maxTextureSize = _kMaxTextureSize;
    textureImporter.textureFormat = TextureImporterFormat.AutomaticTruecolor;
  }

  private void OnPostprocessTexture(Texture2D texture) {
    if (IsUISprite() || IsTexture() || IsRepeatTexture()) {
      if (IsUHDAsset()) {
        Debug.Log("Detected UHD texture import at " + assetPath + ". Creating smaller versions in HD and SD folders.");
        CreateCopyOfTexture(texture, _kHDScaleFactor, _kHDBundleTag);
        CreateCopyOfTexture(texture, _kSDScaleFactor, _kSDBundleTag);
        AssetDatabase.Refresh();
      }
    }
  }

  private bool IsUISprite() {
    return (assetPath.Contains(_kParentFolder) &&
        (assetPath.Contains(_kPackSpriteFolderName) || assetPath.Contains(_kUnpackSpriteFolderName))
            && assetPath.EndsWith(_kGraphicSuffix));
  }

  private bool ShouldPackSprite() {
    return assetPath.Contains(_kParentFolder) && assetPath.Contains(_kPackSpriteFolderName);
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

  private bool IsUHDAsset() {
    return assetPath.Contains(_kParentFolder) && assetPath.Contains(_kUHDBundleTag);
  }

  private bool IsHDAsset() {
    return assetPath.Contains(_kParentFolder) && assetPath.Contains(_kHDBundleTag) && !assetPath.Contains(_kUHDBundleTag);
  }

  private bool IsSDAsset() {
    return assetPath.Contains(_kParentFolder) && assetPath.Contains(_kSDBundleTag);
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

    if (baseTexture.height < _kMinHeight && baseTexture.width < _kMinWidth) {
      newHeight = baseTexture.height;
      newWidth = baseTexture.width;
    }

    TextureScale.Bilinear(newTexture, newWidth, newHeight);
    byte[] newTextureData = newTexture.EncodeToPNG();
    string newAssetPath = assetPath.Replace(_kUHDBundleTag, newBundleTag);
    bool forceReimport = File.Exists(newAssetPath);
    File.WriteAllBytes(newAssetPath, newTextureData);
    if (forceReimport) {
      AssetDatabase.ImportAsset(newAssetPath);
    }
  }

  private bool IsTexture() {
    return assetPath.Contains(_kParentFolder) && assetPath.Contains(_kTexturesFolderName)
                    && assetPath.EndsWith(_kGraphicSuffix) && !assetPath.Contains(_kRepeatTexturesFolderName);
  }
  private bool IsRepeatTexture() {
    return assetPath.Contains(_kParentFolder) && assetPath.Contains(_kRepeatTexturesFolderName)
                    && assetPath.EndsWith(_kGraphicSuffix);
  }
}
