// this component was sourced from the needs meter prototype project sent over by Present Creative
// donovan drane's note: i noticed that it does not respect local space particles. if you need to stick local
//  space particles into the UI, see UIParticleSystem.cs

using UnityEngine;
using UnityEngine.Serialization;
using UnityEngine.UI;

namespace UiParticles {
  /// <summary>
  /// Ui Parcticles, require ParticleSystem component
  /// </summary>
  [RequireComponent(typeof(ParticleSystem))]
  public class UiParticles : MaskableGraphic {

    #region InspectorFields

    [SerializeField]
    [FormerlySerializedAs("m_ParticleSystem")]
    private ParticleSystem m_ParticleSystem;

    [FormerlySerializedAs("m_ParticleSystemRenderer")]
    private ParticleSystemRenderer m_ParticleSystemRenderer;

    /// <summary>
    /// If true, particles renders in streched mode
    /// </summary>
    [FormerlySerializedAs("m_RenderMode")]
    [SerializeField]
    [Tooltip("Render mode of particles")]
    private UiParticleRenderMode m_RenderMode = UiParticleRenderMode.Billboard;

    /// <summary>
    /// Scale particle size, depends on particle velocity
    /// </summary>
    [FormerlySerializedAs("m_StretchedSpeedScale")]
    [SerializeField]
    [Tooltip("Speed Scale for streched billboards")]
    private float m_StretchedSpeedScale = 1f;

    /// <summary>
    /// Sclae particle length in streched mode
    /// </summary>
    [FormerlySerializedAs("m_StretchedLenghScale")]
    [SerializeField]
    [Tooltip("Speed Scale for streched billboards")]
    private float m_StretchedLenghScale = 1f;

    #endregion

    #region Public properties
    /// <summary>
    /// ParticleSystem used for generate particles
    /// </summary>
    /// <value>The particle system.</value>
    public ParticleSystem ParticleSystem {
      get { return m_ParticleSystem; }
      set {
        if (SetPropertyUtility.SetClass(ref m_ParticleSystem, value))
          SetAllDirty();
      }
    }

    public ParticleSystemRenderer particleSystemRenderer {
      get { return m_ParticleSystemRenderer; }
      set {
        if (SetPropertyUtility.SetClass(ref m_ParticleSystemRenderer, value))
          SetAllDirty();
      }
    }

    public override Texture mainTexture {
      get {
        if (material != null && material.mainTexture != null) {
          return material.mainTexture;
        }
        return s_WhiteTexture;
      }
    }

    /// <summary>
    /// Particle system render mode (billboard, strechedBillobard)
    /// </summary>
    public UiParticleRenderMode RenderMode {
      get { return m_RenderMode; }
      set {
        if (SetPropertyUtility.SetStruct(ref m_RenderMode, value))
          SetAllDirty();
      }
    }

    #endregion


    ParticleSystem.Particle[] m_Particles;

    protected override void Awake() {
      var _particleSystem = GetComponent<ParticleSystem>();
      var _particleSystemRenderer = GetComponent<ParticleSystemRenderer>();
      if (m_Material == null) {
        m_Material = _particleSystemRenderer.sharedMaterial;
      }
      if (_particleSystemRenderer.renderMode == ParticleSystemRenderMode.Stretch)
        RenderMode = UiParticleRenderMode.StreachedBillboard;
      base.Awake();
      ParticleSystem = _particleSystem;
      particleSystemRenderer = _particleSystemRenderer;
    }

    public override void SetMaterialDirty() {
      base.SetMaterialDirty();
      if (particleSystemRenderer != null)
        particleSystemRenderer.sharedMaterial = m_Material;
    }

    protected override void OnPopulateMesh(VertexHelper vh) {
      if (ParticleSystem == null) {
        base.OnPopulateMesh(vh);
        return;
      }
      GenerateParticlesBillboards(vh);
    }

    void InitParticlesBuffer() {
      if (m_Particles == null || m_Particles.Length < ParticleSystem.main.maxParticles)
        m_Particles = new ParticleSystem.Particle[ParticleSystem.main.maxParticles];
    }

    void GenerateParticlesBillboards(VertexHelper vh) {
      InitParticlesBuffer();
      int numParticlesAlive = ParticleSystem.GetParticles(m_Particles);

      vh.Clear();

      for (int i = 0; i < numParticlesAlive; i++) {
        DrawParticleBillboard(m_Particles[i], vh);
      }
    }

    private void DrawParticleBillboard(ParticleSystem.Particle particle, VertexHelper vh) {
      var center = particle.position;

      var rotation = Quaternion.Euler(particle.rotation3D);

      if (ParticleSystem.main.simulationSpace == ParticleSystemSimulationSpace.World) {
        center = rectTransform.InverseTransformPoint(center);
      }

      float timeAlive = particle.startLifetime - particle.remainingLifetime;
      float globalTimeAlive = timeAlive / particle.startLifetime;

      Vector3 size3D = particle.GetCurrentSize3D(ParticleSystem);

      if (m_RenderMode == UiParticleRenderMode.StreachedBillboard) {
        GetStrechedBillboardsSizeAndRotation(particle, globalTimeAlive, ref size3D, ref rotation);
      }

      var leftTop = new Vector3(-size3D.x * 0.5f, size3D.y * 0.5f);
      var rightTop = new Vector3(size3D.x * 0.5f, size3D.y * 0.5f);
      var rightBottom = new Vector3(size3D.x * 0.5f, -size3D.y * 0.5f);
      var leftBottom = new Vector3(-size3D.x * 0.5f, -size3D.y * 0.5f);

      leftTop = rotation * leftTop + center;
      rightTop = rotation * rightTop + center;
      rightBottom = rotation * rightBottom + center;
      leftBottom = rotation * leftBottom + center;

      Color32 color32 = particle.GetCurrentColor(ParticleSystem);
      var i = vh.currentVertCount;

      Vector2[] uvs = new Vector2[4];

      if (!ParticleSystem.textureSheetAnimation.enabled) {
        uvs[0] = new Vector2(0f, 0f);
        uvs[1] = new Vector2(0f, 1f);
        uvs[2] = new Vector2(1f, 1f);
        uvs[3] = new Vector2(1f, 0f);
      }
      else {
        var textureAnimator = ParticleSystem.textureSheetAnimation;

        float lifeTimePerCycle = particle.startLifetime / textureAnimator.cycleCount;
        float timePerCycle = timeAlive % lifeTimePerCycle;

        float timeAliveAnim01 = timePerCycle / lifeTimePerCycle; ; // in percents


        float frame01 = textureAnimator.frameOverTime.Evaluate(timeAliveAnim01);
        var totalFramesCount = textureAnimator.numTilesY * textureAnimator.numTilesX;

        var frame = 0f;
        switch (textureAnimator.animation) {
        case ParticleSystemAnimationType.WholeSheet: {
            frame = Mathf.Clamp(Mathf.Floor(frame01 * totalFramesCount), 0, totalFramesCount - 1);
            break;
          }
        case ParticleSystemAnimationType.SingleRow: {
            frame = Mathf.Clamp(Mathf.Floor(frame01 * textureAnimator.numTilesX), 0, textureAnimator.numTilesX - 1);
            int row = textureAnimator.rowIndex;
            if (textureAnimator.useRandomRow) {
              Random.InitState((int)particle.randomSeed);
              row = Random.Range(0, textureAnimator.numTilesY);

            }
            frame += row * textureAnimator.numTilesX;
            break;
          }
        }

        int x = (int)frame % textureAnimator.numTilesX;
        int y = (int)frame / textureAnimator.numTilesY;

        var xDelta = 1f / textureAnimator.numTilesX;
        var yDelta = 1f / textureAnimator.numTilesY;
        y = textureAnimator.numTilesY - 1 - y;
        var sX = x * xDelta;
        var sY = y * yDelta;
        var eX = sX + xDelta;
        var eY = sY + yDelta;

        uvs[0] = new Vector2(sX, sY);
        uvs[1] = new Vector2(sX, eY);
        uvs[2] = new Vector2(eX, eY);
        uvs[3] = new Vector2(eX, sY);
      }

      vh.AddVert(leftBottom, color32, uvs[0]);
      vh.AddVert(leftTop, color32, uvs[1]);
      vh.AddVert(rightTop, color32, uvs[2]);
      vh.AddVert(rightBottom, color32, uvs[3]);

      vh.AddTriangle(i, i + 1, i + 2);
      vh.AddTriangle(i + 2, i + 3, i);
    }

    /// <summary>
    /// Evaluate size and roatation of particle in streched billboard mode
    /// </summary>
    /// <param name="particle">particle</param>
    /// <param name="timeAlive01">current life time percent [0,1] range</param>
    /// <param name="size3D">particle size</param>
    /// <param name="rotation">particle rotation</param>
    private void GetStrechedBillboardsSizeAndRotation(ParticleSystem.Particle particle, float timeAlive01,
        ref Vector3 size3D, ref Quaternion rotation) {

      var currVell = new Vector3();

      if (ParticleSystem.velocityOverLifetime.enabled) {
        currVell.x = ParticleSystem.velocityOverLifetime.x.Evaluate(timeAlive01);
        currVell.y = ParticleSystem.velocityOverLifetime.y.Evaluate(timeAlive01);
        currVell.z = ParticleSystem.velocityOverLifetime.z.Evaluate(timeAlive01);
      }

      var finalVel = particle.velocity + currVell;

      var ang = Vector3.Angle(finalVel, Vector3.up);
      var sig = finalVel.x < 0 ? 1 : -1;
      rotation = Quaternion.Euler(new Vector3(0, 0, ang * sig));
      size3D.y *= m_StretchedLenghScale;
      size3D += new Vector3(0, m_StretchedSpeedScale * finalVel.magnitude);
    }

    protected virtual void Update() {
      if (ParticleSystem != null && ParticleSystem.isPlaying)
        SetVerticesDirty();

      // disable default particle renderer, we using our custom
      if (particleSystemRenderer != null && particleSystemRenderer.enabled)
        particleSystemRenderer.enabled = false;
    }
  }

  public enum UiParticleRenderMode {
    Billboard,
    StreachedBillboard
  }
}
