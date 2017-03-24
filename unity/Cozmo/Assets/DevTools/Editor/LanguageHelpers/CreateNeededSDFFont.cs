using UnityEngine;
using UnityEditor;
using TMPro;
using TMPro.EditorUtilities;
using System;
using System.Collections.Generic;
using System.IO;

// CreateJapaneseSDFFont needs to be in the global namespace right now because we are
// calling it from the command line in a build script
public class CreateNeededSDFFont {
  // Default location of input text asset; may be provided through command line
  private const string _kDefaultCharacterListAssetPath = "Assets/DevTools/Editor/LanguageHelpers/japaneseTranslations.txt";

  // Location of fonts to use to build atlases
  private const string _kJapaneseFontDirectory = "Assets/AssetBundles/Fonts/Fonts-ja-JP/";
  private static string[] _sFontAssetNames = { "AvenirLTStd-Medium", "AvenirLTStd-Black", "AvenirLTStd-Heavy" };
  private const string _kOtfFontAssetExtension = ".otf";

  // SDF Font generation settings. Should not change much
  private const string _kSDFFontNameSuffix = " SDF";
  private const string _kSDFFontAssetExtension = ".asset";
  private static string[] _sSdfFontFallbackNames = { " Kana", " Ascii" };
  private const int _kFontSize = 36;
  private const int _kFontPadding = 4;
  private const int _kFontAtlasWidth = 1024;
  private const int _kFontAtlasHeight = 1024;
  private const FaceStyles _kFontStyle = FaceStyles.Normal;
  private const int _kFontMod = 2;
  private const string _kShaderName = "TextMeshPro/Mobile/Distance Field";

  // Should only ever be DistanceField16 or DistanceField32
  private static RenderModes _sRenderMode = RenderModes.DistanceField16;

  // 0 maps to "Fast"; see FontPackingModes private enum in TMPro_FontAssetCreatorWindow
  private const int _kFontPackingMode = 0;

  // Glyph report file, lists missing glyphs (if any). Written to the same directory as text asset input
  private const string _kGlyphReportFile = "glyphReport.txt";

  private enum GenerationResult {
    Success = 0,
    TextAssetInputNotFound = 1,
    FontMissingGlyphs = 2,
    ShaderNotFound = 3,
    FallbackFontNotFound = 4,
    TMPSuccessException = 99
  }

  // Entry point for generateSDFFromTranslations.py
  public static void CreateSDFFontsFromScript() {
    string[] argv = new string[0];
    try {
      argv = System.Environment.GetCommandLineArgs();
    }
    catch (UnityException ex) {
      Debug.LogError(ex.Message);
    }

    string result = CreateSDFFontsWithArgs(argv);

    if (!string.IsNullOrEmpty(result)) {
      // If builder.Build returned a response, then there was an error.
      // Throw an exception as an attempt to ensure that Unity exits with an error code.yep
      throw new Exception(result);
    }
  }

  private static string CreateSDFFontsWithArgs(string[] args) {
    string characterListAssetPath;
    ParseArgs(args, out characterListAssetPath);

    // If null or empty, everything went smoothly
    return CreateSDFFonts(characterListAssetPath);
  }

  private static void ParseArgs(string[] args, out string characterListAssetPath) {
    characterListAssetPath = null;
    int i = 0;
    while (i < args.Length) {
      string arg = args[i++];
      switch (arg.ToLower()) {
      case "--txt-output-asset-path":
        characterListAssetPath = args[i++];
        break;
      default:
        break;
      }
    }
    if (string.IsNullOrEmpty(characterListAssetPath)) {
      // Use default asset path if none is provied by command line
      Debug.LogWarning("CreateNeededSDFFont.ParseArgs: --txt-output-asset-path is empty; using default "
                       + _kDefaultCharacterListAssetPath);
      characterListAssetPath = _kDefaultCharacterListAssetPath;
    }
  }

  [MenuItem("Cozmo/Text Mesh Pro/Generate ja-JP SDF Font")]
  public static void CreateSDFFontsFromMenu() {
    CreateSDFFonts(_kDefaultCharacterListAssetPath);
  }

  private static string CreateSDFFonts(string characterListAssetPath) {
    int errorCode = (int)GenerationResult.Success;
    string ref_errorMsg = null;

    errorCode = InitializeFontEngine(ref ref_errorMsg);
    if (errorCode != (int)GenerationResult.Success) {
      return ref_errorMsg;
    }

    List<int> characterList;
    errorCode = GetCharacterSequenceFromFile(characterListAssetPath, out characterList, ref ref_errorMsg);
    if (errorCode != (int)GenerationResult.Success) {
      return ref_errorMsg;
    }

    for (int i = 0; i < _sFontAssetNames.Length; i++) {
      errorCode = GenerateFontAtlas(_sFontAssetNames[i], characterList, characterListAssetPath, ref ref_errorMsg);
      if (errorCode != (int)GenerationResult.Success) {
        return ref_errorMsg;
      }
    }

    // If null or empty, everything went smoothly
    return ref_errorMsg;
  }

  private static int InitializeFontEngine(ref string ref_errorMsg) {
    int errorCode = TMPro_FontPlugin.Initialize_FontEngine(); // Initialize Font Engine
    if (errorCode != (int)GenerationResult.Success) {
      if (errorCode != (int)GenerationResult.TMPSuccessException) {
        ref_errorMsg = "CreateNeededSDFFont.InitializeFontEngine: Error Code - " + errorCode
          + " occurred while Initializing the FreeType Library.";
        Debug.LogError(ref_errorMsg);
      }
      else {
        // Debug.LogWarning("CreateNeededSDFFont.CreateSDFFonts: Warning - Font Library was already initialized!");
        errorCode = (int)GenerationResult.Success;
      }
    }
    return errorCode;
  }

  private static int GetCharacterSequenceFromFile(string characterListAssetPath, out List<int> out_characterList,
                                                  ref string ref_errorMsg) {
    out_characterList = new List<int>();

    TextAsset characterListAsset = AssetDatabase.LoadAssetAtPath(characterListAssetPath, typeof(TextAsset)) as TextAsset;
    if (characterListAsset == null) {
      ref_errorMsg = "CreateNeededSDFFont.GetCharacterSequenceFromFile: Error Code - "
        + GenerationResult.TextAssetInputNotFound + " Could not find TextAsset at '" + characterListAssetPath + "'";
      Debug.LogError(ref_errorMsg);
      return (int)GenerationResult.TextAssetInputNotFound;
    }

    string characterSequence = characterListAsset.text;
    for (int i = 0; i < characterSequence.Length; i++) {
      // Check to make sure we don't include duplicates
      if (out_characterList.FindIndex(item => item == characterSequence[i]) == -1) {
        out_characterList.Add(characterSequence[i]);
      }
    }

    return (int)GenerationResult.Success;
  }

  private static int GenerateFontAtlas(string fontAssetName, List<int> characterList, string characterListAssetPath,
                                       ref string ref_errorMsg) {
    // Load the selected font.
    int errorCode = TMPro_FontPlugin.Load_TrueType_Font(_kJapaneseFontDirectory + fontAssetName + _kOtfFontAssetExtension);
    if (errorCode != (int)GenerationResult.Success) {
      if (errorCode != (int)GenerationResult.TMPSuccessException) {
        ref_errorMsg = "CreateNeededSDFFont.GenerateFontAtlas: FontAssetName=" + fontAssetName + " Error Code - "
          + errorCode + "  occurred while Loading the font.";
        Debug.LogError(ref_errorMsg);
        return errorCode;
      }
      else {
        // Debug.LogWarning("CreateNeededSDFFont.GenerateFontAtlas: FontAssetName=" + fontAssetName 
        // + " Warning - Font was already loaded!");
        errorCode = (int)GenerationResult.Success;
      }
    }

    errorCode = TMPro_FontPlugin.FT_Size_Font(_kFontSize); // Load the selected font and size it accordingly.
    if (errorCode != (int)GenerationResult.Success) {
      ref_errorMsg = "CreateNeededSDFFont.GenerateFontAtlas: FontAssetName=" + fontAssetName + " Error Code - "
        + errorCode + "  occurred while Sizing the font.";
      Debug.LogError(ref_errorMsg);
      return errorCode;
    }

    // Define an array containing the characters we will render.
    int[] characterSet = characterList.ToArray();
    int characterCount = characterSet.Length;
    byte[] out_textureBuffer = new byte[_kFontAtlasWidth * _kFontAtlasHeight];
    FT_FaceInfo ref_fontFaceInfo = new FT_FaceInfo();
    FT_GlyphInfo[] out_fontGlyphInfo = new FT_GlyphInfo[characterCount];
    bool autoSizing = false;

    float strokeSize = _kFontMod;
    if (_sRenderMode == RenderModes.DistanceField16) strokeSize = _kFontMod * 16;
    if (_sRenderMode == RenderModes.DistanceField32) strokeSize = _kFontMod * 32;

    errorCode = TMPro_FontPlugin.Render_Characters(out_textureBuffer, _kFontAtlasWidth, _kFontAtlasHeight, _kFontPadding,
                                                    characterSet, characterCount, _kFontStyle, strokeSize,
                                                    autoSizing, RenderModes.DistanceField16, _kFontPackingMode,
                                                    ref ref_fontFaceInfo, out_fontGlyphInfo);

    if (errorCode != (int)GenerationResult.Success) {
      ref_errorMsg = "CreateNeededSDFFont.GenerateFontAtlas: FontAssetName=" + fontAssetName + " Error Code - "
        + errorCode + "  occurred while rendering the font.";
      Debug.LogError(ref_errorMsg);
      return errorCode;
    }

    WriteMissingGlyphsToText(characterListAssetPath, characterCount, ref_fontFaceInfo, out_fontGlyphInfo);
    if (ref_fontFaceInfo.characterCount < characterCount) {
      // Didn't render all characters because texture was too small or source font was missing characters!
      ref_errorMsg = "CreateNeededSDFFont.GenerateFontAtlas: FontAssetName=" + fontAssetName + " Error Code - "
        + GenerationResult.FontMissingGlyphs + " Source font missing glyphs, see " + _kGlyphReportFile;
      Debug.LogError(ref_errorMsg);
      return (int)GenerationResult.FontMissingGlyphs;
    }

    Texture2D fontAtlas = CreateFontTexture(out_textureBuffer);

    errorCode = SaveSDFFontAsset(ref_fontFaceInfo, out_fontGlyphInfo, fontAtlas, fontAssetName, ref ref_errorMsg);

    return errorCode;
  }

  private static Texture2D CreateFontTexture(byte[] textureBuffer) {
    Texture2D fontAtlas = new Texture2D(_kFontAtlasWidth, _kFontAtlasHeight, TextureFormat.Alpha8, false, true);

    Color32[] colors = new Color32[_kFontAtlasWidth * _kFontAtlasHeight];

    for (int i = 0; i < (_kFontAtlasWidth * _kFontAtlasHeight); i++) {
      byte c = textureBuffer[i];
      colors[i] = new Color32(c, c, c, c);
    }

    if (_sRenderMode == RenderModes.RasterHinted) {
      fontAtlas.filterMode = FilterMode.Point;
    }

    fontAtlas.SetPixels32(colors, 0);
    fontAtlas.Apply(false, true);

    return fontAtlas;
  }

  private static int SaveSDFFontAsset(FT_FaceInfo fontFaceInfo, FT_GlyphInfo[] fontGlyphInfo, Texture2D fontAtlas,
                                      string fontAssetName, ref string ref_errorMsg) {
    string saveAssetPath = _kJapaneseFontDirectory + fontAssetName + _kSDFFontNameSuffix + _kSDFFontAssetExtension;
    string sdfFontName = Path.GetFileNameWithoutExtension(saveAssetPath);

    // Check if TextMeshPro font asset already exists. If not, create a new one. Otherwise update the existing one.
    TMP_FontAsset fontAsset = AssetDatabase.LoadAssetAtPath(saveAssetPath, typeof(TMP_FontAsset)) as TMP_FontAsset;
    if (fontAsset == null) {
      fontAsset = ScriptableObject.CreateInstance<TMP_FontAsset>(); // Create new TextMeshPro Font Asset.
      AssetDatabase.CreateAsset(fontAsset, saveAssetPath);

      //Set Font Asset Type
      fontAsset.fontAssetType = TMP_FontAsset.FontAssetTypes.SDF;

      // Add FaceInfo to Font Asset
      FaceInfo face = GetFaceInfo(fontFaceInfo);
      fontAsset.AddFaceInfo(face);

      // Add GlyphInfo[] to Font Asset
      TMP_Glyph[] glyphs = GetGlyphInfo(fontGlyphInfo);
      fontAsset.AddGlyphInfo(glyphs);

      // Add Font Atlas as Sub-Asset
      fontAsset.atlas = fontAtlas;
      fontAtlas.name = sdfFontName + " Atlas";

      // Special handling due to a bug in earlier versions of Unity.
#if UNITY_5_3_OR_NEWER
      // Nothing
#else
                    m_font_Atlas.hideFlags = HideFlags.HideInHierarchy;
#endif

      AssetDatabase.AddObjectToAsset(fontAtlas, fontAsset);

      // Create new Material and Add it as Sub-Asset
      Shader fontAtlasShader = Shader.Find(_kShaderName);
      if (fontAtlasShader == null) {
        ref_errorMsg = "CreateNeededSDFFont.SaveSDFFontAsset: FontAssetName=" + fontAssetName + " Error Code - "
          + GenerationResult.ShaderNotFound + " Could not find shader with name: '" + _kShaderName + "'";
        Debug.LogError(ref_errorMsg);
        return (int)GenerationResult.ShaderNotFound;
      }

      Material fontAtlasMaterial = new Material(fontAtlasShader);

      fontAtlasMaterial.name = sdfFontName + " Material";
      fontAtlasMaterial.SetTexture(ShaderUtilities.ID_MainTex, fontAtlas);
      fontAtlasMaterial.SetFloat(ShaderUtilities.ID_TextureWidth, fontAtlas.width);
      fontAtlasMaterial.SetFloat(ShaderUtilities.ID_TextureHeight, fontAtlas.height);

      int spread = _kFontPadding + 1;
      fontAtlasMaterial.SetFloat(ShaderUtilities.ID_GradientScale, spread); // Spread = Padding for Brute Force SDF.

      fontAtlasMaterial.SetFloat(ShaderUtilities.ID_WeightNormal, fontAsset.normalStyle);
      fontAtlasMaterial.SetFloat(ShaderUtilities.ID_WeightBold, fontAsset.boldStyle);

      fontAsset.material = fontAtlasMaterial;

      // Special handling due to a bug in earlier versions of Unity.
#if UNITY_5_3_OR_NEWER
      // Nothing
#else
                    tmp_material.hideFlags = HideFlags.HideInHierarchy;
#endif

      AssetDatabase.AddObjectToAsset(fontAtlasMaterial, fontAsset);

      // Set up fallback fonts
      List<TMP_FontAsset> fallbackFontAssets = new List<TMP_FontAsset>();
      for (int i = 0; i < _sSdfFontFallbackNames.Length; i++) {
        string fallbackPath = _kJapaneseFontDirectory + fontAssetName + _kSDFFontNameSuffix + _sSdfFontFallbackNames[i]
          + _kSDFFontAssetExtension;
        TMP_FontAsset fallbackFont = AssetDatabase.LoadAssetAtPath(fallbackPath, typeof(TMP_FontAsset)) as TMP_FontAsset;
        if (fallbackFont != null) {
          fallbackFontAssets.Add(fallbackFont);
        }
        else {
          ref_errorMsg = "CreateNeededSDFFont.SaveSDFFontAsset: FontAssetName=" + fontAssetName + " Error Code - "
            + GenerationResult.FallbackFontNotFound + "Could not add fallback font for fallback: '"
            + _sSdfFontFallbackNames[i] + "'";
          Debug.LogError(ref_errorMsg);
          return (int)GenerationResult.FallbackFontNotFound;
        }
      }

      fontAsset.fallbackFontAssets = fallbackFontAssets;
    }
    else {
      // Find all Materials referencing this font atlas.
      Material[] materialReferences = TMP_EditorUtility.FindMaterialReferences(fontAsset);

      // Destroy Assets that will be replaced.
      GameObject.DestroyImmediate(fontAsset.atlas, true);

      //Set Font Asset Type
      fontAsset.fontAssetType = TMP_FontAsset.FontAssetTypes.SDF;

      // Add FaceInfo to Font Asset  
      FaceInfo face = GetFaceInfo(fontFaceInfo);
      fontAsset.AddFaceInfo(face);

      // Add GlyphInfo[] to Font Asset
      TMP_Glyph[] glyphs = GetGlyphInfo(fontGlyphInfo);
      fontAsset.AddGlyphInfo(glyphs);

      // Add Font Atlas as Sub-Asset
      fontAsset.atlas = fontAtlas;
      fontAtlas.name = sdfFontName + " Atlas";

      // Special handling due to a bug in earlier versions of Unity.
#if UNITY_5_3_OR_NEWER
      fontAtlas.hideFlags = HideFlags.None;
      fontAsset.material.hideFlags = HideFlags.None;
#else
                    m_font_Atlas.hideFlags = HideFlags.HideInHierarchy;
#endif

      AssetDatabase.AddObjectToAsset(fontAtlas, fontAsset);

      // Assign new font atlas texture to the existing material.
      fontAsset.material.SetTexture(ShaderUtilities.ID_MainTex, fontAsset.atlas);

      // Update the Texture reference on the Material
      for (int i = 0; i < materialReferences.Length; i++) {
        materialReferences[i].SetTexture(ShaderUtilities.ID_MainTex, fontAtlas);
        materialReferences[i].SetFloat(ShaderUtilities.ID_TextureWidth, fontAtlas.width);
        materialReferences[i].SetFloat(ShaderUtilities.ID_TextureHeight, fontAtlas.height);

        int spread = _kFontPadding + 1;
        materialReferences[i].SetFloat(ShaderUtilities.ID_GradientScale, spread); // Spread = Padding for Brute Force SDF.

        materialReferences[i].SetFloat(ShaderUtilities.ID_WeightNormal, fontAsset.normalStyle);
        materialReferences[i].SetFloat(ShaderUtilities.ID_WeightBold, fontAsset.boldStyle);
      }
    }

    AssetDatabase.SaveAssets();

    Debug.Log("CreateNeededSDFFont.SaveSDFFontAsset: FontAssetName=" + fontAssetName + " SUCCESS - Finished SDF Creation");

    return (int)GenerationResult.Success;
  }

  // Convert from FT_FaceInfo to FaceInfo
  private static FaceInfo GetFaceInfo(FT_FaceInfo fontFace) {
    FaceInfo face = new FaceInfo();

    face.Name = fontFace.name;
    face.PointSize = (float)fontFace.pointSize;
    face.Padding = fontFace.padding;
    face.LineHeight = fontFace.lineHeight;
    face.CapHeight = 0;
    face.Baseline = 0;
    face.Ascender = fontFace.ascender;
    face.Descender = fontFace.descender;
    face.CenterLine = fontFace.centerLine;
    face.Underline = fontFace.underline;

    // Set Thickness to 5 if TTF value is Zero.
    face.UnderlineThickness = (fontFace.underlineThickness.IsNear(0, float.Epsilon)) ? 5 : fontFace.underlineThickness;

    face.strikethrough = (face.Ascender + face.Descender) / 2.75f;
    face.strikethroughThickness = face.UnderlineThickness;
    face.SuperscriptOffset = face.Ascender;
    face.SubscriptOffset = face.Underline;
    face.SubSize = 0.5f;
    face.AtlasWidth = fontFace.atlasWidth;
    face.AtlasHeight = fontFace.atlasHeight;

    return face;
  }


  // Convert from FT_GlyphInfo[] to GlyphInfo[]
  private static TMP_Glyph[] GetGlyphInfo(FT_GlyphInfo[] fontGlyphs) {
    List<TMP_Glyph> glyphs = new List<TMP_Glyph>();

    for (int i = 0; i < fontGlyphs.Length; i++) {
      // Filter out characters with missing glyphs.
      if (fontGlyphs[i].x.IsNear(-1, float.Epsilon))
        continue;

      TMP_Glyph g = new TMP_Glyph();

      g.id = fontGlyphs[i].id;
      g.x = fontGlyphs[i].x;
      g.y = fontGlyphs[i].y;
      g.width = fontGlyphs[i].width;
      g.height = fontGlyphs[i].height;
      g.xOffset = fontGlyphs[i].xOffset;
      g.yOffset = fontGlyphs[i].yOffset;
      g.xAdvance = fontGlyphs[i].xAdvance;

      glyphs.Add(g);
    }

    return glyphs.ToArray();
  }

  private static void WriteMissingGlyphsToText(string characterListAssetPath, int targetCharacterCount,
                                               FT_FaceInfo fontFace, FT_GlyphInfo[] fontGlyphs) {
    string glyphReport = "Font: " + fontFace.name + "\n";
    glyphReport += "Pt.Size: " + fontFace.pointSize + "\n";
    glyphReport += "Characters packed: " + fontFace.characterCount + "/" + targetCharacterCount + "\n";

    if (fontFace.characterCount == targetCharacterCount) {
      glyphReport += "\nSuccessfully packed all characters!";
    }
    else {
      // Report missing requested glyph
      glyphReport += "\nMissing Characters:\n";
      glyphReport += "----------------------------------------";

      for (int i = 0; i < targetCharacterCount; i++) {
        if (fontGlyphs[i].x.IsNear(-1, float.Epsilon)) {
          glyphReport += string.Format("\nID: {0:D5}\tHex: {1:X4}\tChar: [{2}]",
                                              fontGlyphs[i].id,
                                              fontGlyphs[i].id,
                                              (char)fontGlyphs[i].id);
        }
      }
    }

    // Save Missing Glyph Report file
    string fullCharacterListAssetPath = Path.GetFullPath(characterListAssetPath);
    string characterListAssetDir = Path.GetDirectoryName(fullCharacterListAssetPath);
    string glyphReportPath = Path.Combine(characterListAssetDir, _kGlyphReportFile);
    System.IO.File.WriteAllText(glyphReportPath, glyphReport);
  }
}
