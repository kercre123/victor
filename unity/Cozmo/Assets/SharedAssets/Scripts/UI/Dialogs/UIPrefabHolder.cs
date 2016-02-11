using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class UIPrefabHolder : ScriptableObject {

      private static UIPrefabHolder _Instance;

      public static UIPrefabHolder Instance {
        get {
          if (_Instance == null) {
            _Instance = Resources.Load<UIPrefabHolder>("Prefabs/UI/UIPrefabHolder");
          }
          return _Instance;
        }
      }

      public static Vector4 GetAtlasUVs(Sprite graphic) {
        float textureAtlasPixelWidth = graphic.texture.width;
        float xMinNormalizedUV = graphic.textureRect.xMin / textureAtlasPixelWidth;
        float xMaxNormalizedUV = graphic.textureRect.xMax / textureAtlasPixelWidth;

        float textureAtlasPixelHeight = graphic.texture.height;
        float yMinNormalizedUV = graphic.textureRect.yMin / textureAtlasPixelHeight;
        float yMaxNormalizedUV = graphic.textureRect.yMax / textureAtlasPixelHeight;

        return new Vector4(xMinNormalizedUV,
          yMinNormalizedUV,
          xMaxNormalizedUV - xMinNormalizedUV,
          yMaxNormalizedUV - yMinNormalizedUV);
      }

      [SerializeField]
      private GameObject _FullScreenButtonPrefab;

      public GameObject FullScreenButtonPrefab {
        get { return _FullScreenButtonPrefab; }
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

      [SerializeField]
      private Cozmo.MinigameWidgets.SharedMinigameView _SharedMinigameViewPrefab;

      public Cozmo.MinigameWidgets.SharedMinigameView SharedMinigameViewPrefab {
        get { return _SharedMinigameViewPrefab; }
      }

      [SerializeField]
      private ChallengeEndedDialog _ChallengeEndViewPrefab;

      public ChallengeEndedDialog ChallengeEndViewPrefab {
        get { return _ChallengeEndViewPrefab; }
      }

      [SerializeField]
      private GameObject _ShowCozmoCubeSlide;

      public GameObject InitialCubesSlide {
        get { return _ShowCozmoCubeSlide; }
      }

      [SerializeField]
      private Shader _GradiantSimpleClippingShader;

      public Shader GradiantSimpleClippingShader {
        get { return _GradiantSimpleClippingShader; }
      }

      [SerializeField]
      private Shader _GradiantComplexClippingShader;

      public Shader GradiantComplexClippingShader {
        get { return _GradiantComplexClippingShader; }
      }

      [SerializeField]
      private Shader _GradiantSimpleClippingScreenspaceShader;

      public Shader GradiantSimpleClippingScreenspaceShader {
        get { return _GradiantSimpleClippingScreenspaceShader; }
      }

      [SerializeField]
      private Shader _GradiantComplexClippingScreenspaceShader;

      public Shader GradiantComplexClippingScreenspaceShader {
        get { return _GradiantComplexClippingScreenspaceShader; }
      }

      [SerializeField]
      private Shader _GrayscaleShader;

      public Shader GrayscaleShader {
        get { return _GrayscaleShader; }
      }

      [SerializeField]
      private Shader _AnimatedGlintShader;

      public Shader AnimatedGlintShader {
        get { return _AnimatedGlintShader; }
      }

      [SerializeField]
      private Shader _BlurShader;

      public Shader BlurShader {
        get { return _BlurShader; }
      }
    }
  }
}