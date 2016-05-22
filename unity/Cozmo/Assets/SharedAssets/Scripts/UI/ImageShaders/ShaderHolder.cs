using UnityEngine;
using System.Collections;

namespace Cozmo {
  public class ShaderHolder : ScriptableObject {

    private static ShaderHolder _sInstance;

    public static void SetInstance(ShaderHolder instance) {
      _sInstance = instance;
    }

    public static ShaderHolder Instance {
      get { return _sInstance; }
    }

    [SerializeField]
    private Material _DefaultMaterial;

    public Material DefaultMaterial {
      get { return _DefaultMaterial; }
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