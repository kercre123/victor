// Credit glennpow
// Sourced from - http://forum.unity3d.com/threads/free-script-particle-systems-in-ui-screen-space-overlay.406862/
//
//donovan drane's note: this system respects local space particles, whereas UIParticles.cs does not. however, i noticed
//  this component seemed to cause more garbage collection as currently written, so we should only switch to this
//  script if we need local particles (which we might for the NeedsMeter)

namespace UnityEngine.UI.Extensions {
  [ExecuteInEditMode]
  [RequireComponent(typeof(CanvasRenderer))]
  [RequireComponent(typeof(ParticleSystem))]
  public class UIParticleSystem : MaskableGraphic {

    [SerializeField]
    private Texture _ParticleTexture = null;

    [SerializeField]
    private Sprite _ParticleSprite = null;

    private ParticleSystem _ParticleSystem;
    private ParticleSystem.Particle[] _Particles;
    private UIVertex[] _Quad = new UIVertex[4];
    private Vector4 _UV = Vector4.zero;
    private ParticleSystem.TextureSheetAnimationModule _TextureSheetAnimation;
    private int _TextureSheetAnimationFrames;
    private Vector2 _TextureSheedAnimationFrameSize;

    public override Texture mainTexture {
      get {
        if (_ParticleTexture) {
          return _ParticleTexture;
        }

        if (_ParticleSprite) {
          return _ParticleSprite.texture;
        }

        return null;
      }
    }

    protected bool Initialize() {
      // prepare particle system
      ParticleSystemRenderer rend = GetComponent<ParticleSystemRenderer>();
      bool setParticleSystemMaterial = false;

      if (_ParticleSystem == null) {
        _ParticleSystem = GetComponent<ParticleSystem>();

        if (_ParticleSystem == null) {
          return false;
        }

        // get current particle texture
        if (rend == null) {
          rend = _ParticleSystem.gameObject.AddComponent<ParticleSystemRenderer>();
        }
        Material currentMaterial = rend.sharedMaterial;
        if (currentMaterial && currentMaterial.HasProperty("_MainTex")) {
          _ParticleTexture = currentMaterial.mainTexture;
        }

        // automatically set scaling
        var mainModule = _ParticleSystem.main;
        mainModule.scalingMode = ParticleSystemScalingMode.Local;

        _Particles = null;
        setParticleSystemMaterial = true;
      }
      else {
        if (Application.isPlaying) {
          setParticleSystemMaterial = (rend.material == null);
        }
#if UNITY_EDITOR
        else {
          setParticleSystemMaterial = (rend.sharedMaterial == null);
        }
#endif
      }

      if (setParticleSystemMaterial) {
        Material mat = new Material(material);
        if (Application.isPlaying) {
          rend.material = mat;
        }
#if UNITY_EDITOR
        else {
          mat.hideFlags = HideFlags.DontSave;
          rend.sharedMaterial = mat;
        }
#endif
      }

      // prepare particles array
      if (_Particles == null) {
        _Particles = new ParticleSystem.Particle[_ParticleSystem.main.maxParticles];
      }

      // prepare uvs
      if (_ParticleTexture) {
        _UV = new Vector4(0, 0, 1, 1);
      }
      else if (_ParticleSprite) {
        _UV = UnityEngine.Sprites.DataUtility.GetOuterUV(_ParticleSprite);
      }

      // prepare texture sheet animation
      _TextureSheetAnimation = _ParticleSystem.textureSheetAnimation;
      _TextureSheetAnimationFrames = 0;
      _TextureSheedAnimationFrameSize = Vector2.zero;
      if (_TextureSheetAnimation.enabled) {
        _TextureSheetAnimationFrames = _TextureSheetAnimation.numTilesX * _TextureSheetAnimation.numTilesY;
        _TextureSheedAnimationFrameSize = new Vector2(1f / _TextureSheetAnimation.numTilesX, 1f / _TextureSheetAnimation.numTilesY);
      }

      return true;
    }

    protected override void Awake() {
      base.Awake();

      if (!Initialize()) {
        enabled = false;
      }
    }

    protected override void OnPopulateMesh(VertexHelper vh) {
#if UNITY_EDITOR
      if (!Application.isPlaying) {
        if (!Initialize()) {
          return;
        }
      }
#endif

      // prepare vertices
      vh.Clear();

      if (!gameObject.activeInHierarchy) {
        return;
      }

      // iterate through current particles
      int count = _ParticleSystem.GetParticles(_Particles);

      for (int i = 0; i < count; ++i) {
        ParticleSystem.Particle particle = _Particles[i];

        // get particle properties
        Vector2 position = (_ParticleSystem.main.simulationSpace == ParticleSystemSimulationSpace.Local ? particle.position : transform.InverseTransformPoint(particle.position));
        float rotation = -particle.rotation * Mathf.Deg2Rad;
        float rotation90 = rotation + Mathf.PI / 2;
        Color32 col = particle.GetCurrentColor(_ParticleSystem);
        float size = particle.GetCurrentSize(_ParticleSystem) * 0.5f;

        // apply scale
        if (_ParticleSystem.main.scalingMode == ParticleSystemScalingMode.Shape) {
          position /= canvas.scaleFactor;
        }

        // apply texture sheet animation
        Vector4 particleUV = _UV;
        if (_TextureSheetAnimation.enabled) {
          float frameProgress = 1 - (particle.remainingLifetime / particle.startLifetime);
          //                float frameProgress = textureSheetAnimation.frameOverTime.curveMin.Evaluate(1 - (particle.lifetime / particle.startLifetime)); // TODO - once Unity allows MinMaxCurve reading
          frameProgress = Mathf.Repeat(frameProgress * _TextureSheetAnimation.cycleCount, 1);
          int frame = 0;

          switch (_TextureSheetAnimation.animation) {

          case ParticleSystemAnimationType.WholeSheet:
            frame = Mathf.FloorToInt(frameProgress * _TextureSheetAnimationFrames);
            break;

          case ParticleSystemAnimationType.SingleRow:
            frame = Mathf.FloorToInt(frameProgress * _TextureSheetAnimation.numTilesX);

            int row = _TextureSheetAnimation.rowIndex;
            //                    if (textureSheetAnimation.useRandomRow) { // FIXME - is this handled internally by rowIndex?
            //                        row = Random.Range(0, textureSheetAnimation.numTilesY, using: particle.randomSeed);
            //                    }
            frame += row * _TextureSheetAnimation.numTilesX;
            break;

          }

          frame %= _TextureSheetAnimationFrames;

          particleUV.x = (frame % _TextureSheetAnimation.numTilesX) * _TextureSheedAnimationFrameSize.x;
          particleUV.y = Mathf.FloorToInt(frame / _TextureSheetAnimation.numTilesX) * _TextureSheedAnimationFrameSize.y;
          particleUV.z = particleUV.x + _TextureSheedAnimationFrameSize.x;
          particleUV.w = particleUV.y + _TextureSheedAnimationFrameSize.y;
        }

        _Quad[0] = UIVertex.simpleVert;
        _Quad[0].color = col;
        _Quad[0].uv0 = new Vector2(particleUV.x, particleUV.y);

        _Quad[1] = UIVertex.simpleVert;
        _Quad[1].color = col;
        _Quad[1].uv0 = new Vector2(particleUV.x, particleUV.w);

        _Quad[2] = UIVertex.simpleVert;
        _Quad[2].color = col;
        _Quad[2].uv0 = new Vector2(particleUV.z, particleUV.w);

        _Quad[3] = UIVertex.simpleVert;
        _Quad[3].color = col;
        _Quad[3].uv0 = new Vector2(particleUV.z, particleUV.y);

        if (rotation.IsNear(0f, float.Epsilon)) {
          // no rotation
          Vector2 corner1 = new Vector2(position.x - size, position.y - size);
          Vector2 corner2 = new Vector2(position.x + size, position.y + size);

          _Quad[0].position = new Vector2(corner1.x, corner1.y);
          _Quad[1].position = new Vector2(corner1.x, corner2.y);
          _Quad[2].position = new Vector2(corner2.x, corner2.y);
          _Quad[3].position = new Vector2(corner2.x, corner1.y);
        }
        else {
          // apply rotation
          Vector2 right = new Vector2(Mathf.Cos(rotation), Mathf.Sin(rotation)) * size;
          Vector2 up = new Vector2(Mathf.Cos(rotation90), Mathf.Sin(rotation90)) * size;

          _Quad[0].position = position - right - up;
          _Quad[1].position = position - right + up;
          _Quad[2].position = position + right + up;
          _Quad[3].position = position + right - up;
        }

        vh.AddUIVertexQuad(_Quad);
      }
    }

    void Update() {
      if (Application.isPlaying) {
        // unscaled animation within UI
        _ParticleSystem.Simulate(Time.unscaledDeltaTime, false, false);

        SetAllDirty();
      }
    }

#if UNITY_EDITOR
    void LateUpdate() {
      if (!Application.isPlaying) {
        SetAllDirty();
      }
    }
#endif
  }

}