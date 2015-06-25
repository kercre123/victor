//
// Procedural Lightning for Unity
// (c) 2015 Digital Ruby, LLC
// Source code may be used for personal or commercial projects.
// Source code may NOT be redistributed or sold.
// 

// #define ENABLE_LINE_RENDERER

using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DigitalRuby.ThunderAndLightning
{
    /// <summary>
    /// Abstract class for lightning renderer
    /// </summary>
    public abstract class LightningBoltRenderer
    {
        /// <summary>
        /// Begin a lightning bolt
        /// </summary>
        /// <param name="lightningBolt">Lightning bolt</param>
        public virtual void Begin(LightningBolt lightningBolt) { }

        /// <summary>
        /// Render a lightning bolt
        /// </summary>
        /// <param name="lightningBolt">Lightning bolt to render</param>
        public virtual void Render(LightningBolt lightningBolt) { }

        /// <summary>
        /// Add a group to the renderer
        /// </summary>
        /// <param name="lightningBolt">Lightning bolt the group belongs to</param>
        /// <param name="group">Group to add</param>
        /// <param name="growthMultiplier">0 - 1, determines how fast the lightning grows over its lifetime. 0 grows instantly, 1 grows slowly.</param>
        public virtual void AddGroup(LightningBolt lightningBolt, LightningBoltSegmentGroup group, float growthMultiplier) { }

        /// <summary>
        /// Remove a group
        /// </summary>
        /// <param name="group">Group to remove</param>
        public virtual void RemoveGroup(LightningBoltSegmentGroup group) { }

        /// <summary>
        /// Perform any cleanup
        /// </summary>
        /// <param name="lightningBolt">Lightning bolt to cleanup</param>
        public virtual void Cleanup(LightningBolt lightningBolt) { }

        /// <summary>
        /// Lightning bolt script
        /// </summary>
        public LightningBoltScript Script { get; set; }

        /// <summary>
        /// Shader index
        /// </summary>
        public virtual int ShaderIndex { get { return 0; } }

        /// <summary>
        /// Material
        /// </summary>
        public Material Material { get; set; }

        /// <summary>
        /// Whether this renderer supports glow
        /// </summary>
        public virtual bool CanGlow { get { return false; } }

        /// <summary>
        /// Glow width multiplier (0 for none)
        /// </summary>
        public float GlowWidthMultiplier { get; set; }

        /// <summary>
        /// Glow intensity multiplier (0 for none)
        /// </summary>
        public float GlowIntensityMultiplier { get; set; }
    }

#if ENABLE_LINE_RENDERER

    /// <summary>
    /// Render lightning using line renderers
    /// </summary>
    public class LightningBoltLineRenderer : LightningBoltRenderer
    {
        private IEnumerator EnableLineRenderer(LineRenderer r, LightningBolt lightningBolt, float delay)
        {
            yield return new WaitForSeconds(delay);

            if (r != null && lightningBolt.ElapsedTime < lightningBolt.LifeTime)
            {
                r.gameObject.SetActive(true);
                r.enabled = true;
            }

            yield break;
        }

        private void CreateLineRenderer(LightningBolt lightningBolt, LightningBoltSegmentGroup group)
        {
            if (group.Tag == null)
            {
                // create a line renderer in a new game object underneath the parent
                GameObject obj = new GameObject();
                obj.name = "LightningBoltLineRenderer";
                obj.transform.parent = lightningBolt.Parent.transform;
                obj.transform.position = lightningBolt.Parent.transform.position;

                // setup the line renderer, using world space with no shadows or light
                LineRenderer lineRenderer = obj.AddComponent<LineRenderer>();
                Color c = new Color(0, 0, 0, 0);
                lineRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                lineRenderer.receiveShadows = false;
                lineRenderer.SetColors(c, c);
                lineRenderer.sharedMaterial = Material;
                lineRenderer.useWorldSpace = true;
                lineRenderer.useLightProbes = false;
                lineRenderer.enabled = false;

                group.Tag = lineRenderer;
            }

            LineRenderer r = group.Tag as LineRenderer;
            int index = 0;
            r.SetWidth(group.LineWidth, group.LineWidth * group.EndWidthMultiplier);
            r.SetVertexCount(group.Segments.Count - group.StartIndex);
            r.SetPosition(index++, group.Segments[group.StartIndex].Start);

            for (int i = group.StartIndex + 1; i < group.Segments.Count; i++)
            {
                r.SetPosition(index++, group.Segments[i].End);
            }

            // enable the line renderer after currentDelayAmount has elapsed
            Script.StartCoroutine(EnableLineRenderer(r, lightningBolt, lightningBolt.Delay + lightningBolt.CurrentDelayAmount));
        }

        private void UpdateColors(LightningBoltSegmentGroup group, float elapsed)
        {
            float lerp, multiplier1, multiplier2;
            if (elapsed >= group.PeakStart)
            {
                if (elapsed <= group.PeakEnd)
                {
                    lerp = 1.0f;
                    multiplier1 = 1.0f;
                    multiplier2 = 1.0f;
                }
                else
                {
                    lerp = (elapsed - group.PeakEnd) / (group.LifeTime - group.PeakEnd);
                    multiplier1 = 1.0f;
                    multiplier2 = 0.0f;
                }
            }
            else
            {
                lerp = elapsed / group.PeakStart;
                multiplier1 = 0.0f;
                multiplier2 = 1.0f;
            }

            Color c = group.Color;
            c.a = Mathf.Lerp(c.a * multiplier1, c.a * multiplier2, lerp);
            (group.Tag as LineRenderer).SetColors(c, c);
        }

        public override void Render(LightningBolt lightningBolt)
        {
            foreach (LightningBoltSegmentGroup group in lightningBolt.Groups)
            {
                if (lightningBolt.ElapsedTime < group.Delay || group.Tag == null || !(group.Tag as LineRenderer).enabled)
                {
                    return;
                }

                UpdateColors(group, lightningBolt.ElapsedTime - group.Delay);
            }
        }

        public override void AddGroup(LightningBolt lightningBolt, LightningBoltSegmentGroup group, float growthMultiplier)
        {
            CreateLineRenderer(lightningBolt, group);
        }

        public override void RemoveGroup(LightningBoltSegmentGroup group)
        {
            if (group.Tag != null)
            {
                LineRenderer r = (group.Tag as LineRenderer);
                r.enabled = false;
                r.gameObject.SetActive(false);
            }
        }
    }

#endif

    /// <summary>
    /// Render lightning using meshes
    /// </summary>
    public class LightningBoltMeshRenderer : LightningBoltRenderer
    {
        [RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
        public class LineRendererMesh : MonoBehaviour
        {
            private static readonly Vector2 uv1 = new Vector2(0.0f, 0.0f);
            private static readonly Vector2 uv2 = new Vector2(1.0f, 0.0f);
            private static readonly Vector2 uv3 = new Vector2(0.0f, 1.0f);
            private static readonly Vector2 uv4 = new Vector2(1.0f, 1.0f);

            private Mesh mesh;
            private MeshRenderer meshRenderer;
            private readonly List<int> indices = new List<int>();
            private readonly List<Vector2> texCoords = new List<Vector2>();
            private readonly List<Vector3> vertices = new List<Vector3>();
            private readonly List<Vector4> lineDirs = new List<Vector4>();
            private readonly List<Color> colors = new List<Color>();
            private readonly List<Vector2> glowModifiers = new List<Vector2>();
            private readonly List<Vector3> ends = new List<Vector3>();
            private bool needsUpdate = true;

            private void Awake()
            {
                mesh = new Mesh();
                GetComponent<MeshFilter>().sharedMesh = mesh;

                meshRenderer = GetComponent<MeshRenderer>();
                meshRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                meshRenderer.useLightProbes = false;
                meshRenderer.receiveShadows = false;
                meshRenderer.reflectionProbeUsage = UnityEngine.Rendering.ReflectionProbeUsage.Off;
                meshRenderer.enabled = false;
            }

            private void Update()
            {
                if (needsUpdate)
                {
                    mesh.vertices = vertices.ToArray();
                    mesh.tangents = lineDirs.ToArray();
                    mesh.colors = colors.ToArray();
                    mesh.uv = texCoords.ToArray();
                    mesh.uv2 = glowModifiers.ToArray();
					mesh.normals = ends.ToArray();
                    mesh.triangles = indices.ToArray();
                    needsUpdate = false;
                }
            }

            private void AddIndices()
            {
                int vertexIndex = vertices.Count;
                indices.Add(vertexIndex++);
                indices.Add(vertexIndex++);
                indices.Add(vertexIndex);
                indices.Add(vertexIndex--);
                indices.Add(vertexIndex);
                indices.Add(vertexIndex += 2);
            }

            public void Begin()
            {
                meshRenderer.enabled = true;
            }

            public bool PrepareForLines(int lineCount)
            {
                int vertexCount = lineCount * 4;
                if (vertices.Count + vertexCount >= 65000)
                {
                    return false;
                }
                return true;
            }

            public void BeginLine(Vector3 start, Vector3 end, float radius, Color c, float glowWidthModifier, float glowIntensity)
            {
                AddIndices();

                Vector2 glowModifier = new Vector2(glowWidthModifier, glowIntensity);
                Vector4 dir = (end - start);
                dir.w = radius;

                vertices.Add(start);
                texCoords.Add(uv1);
                lineDirs.Add(dir);
                colors.Add(c);
                glowModifiers.Add(glowModifier);
				ends.Add(dir);

                vertices.Add(end);
                texCoords.Add(uv2);
                lineDirs.Add(dir);
                colors.Add(c);
                glowModifiers.Add(glowModifier);
				ends.Add(dir);

                dir.w = -radius;

                vertices.Add(start);
                texCoords.Add(uv3);
                lineDirs.Add(dir);
                colors.Add(c);
                glowModifiers.Add(glowModifier);
				ends.Add(dir);

                vertices.Add(end);
                texCoords.Add(uv4);
                lineDirs.Add(dir);
                colors.Add(c);
                glowModifiers.Add(glowModifier);
				ends.Add(dir);

                needsUpdate = true;
            }

            public void AppendLine(Vector3 start, Vector3 end, float radius, Color c, float glowWidthModifier, float glowIntensity)
            {
                AddIndices();

                Vector2 glowModifier = new Vector2(glowWidthModifier, glowIntensity);
                Vector4 dir = (end - start);
                dir.w = radius;

                vertices.Add(start);
                texCoords.Add(uv1);
                // rotation will be in the same direction as the previous connected vertices
                lineDirs.Add(lineDirs[lineDirs.Count - 3]);
                colors.Add(c);
                glowModifiers.Add(glowModifier);
				ends.Add(dir);

                vertices.Add(end);
                texCoords.Add(uv2);
                lineDirs.Add(dir);
                colors.Add(c);
                glowModifiers.Add(glowModifier);
				ends.Add(dir);

                dir.w = -radius;

                vertices.Add(start);
                texCoords.Add(uv3);
                // rotation will be in the same direction as the previous connected vertices
                lineDirs.Add(lineDirs[lineDirs.Count - 3]);
                colors.Add(c);
                glowModifiers.Add(glowModifier);
				ends.Add(dir);

                vertices.Add(end);
                texCoords.Add(uv4);
                lineDirs.Add(dir);
                colors.Add(c);
                glowModifiers.Add(glowModifier);
				ends.Add(dir);
            }

            public void Reset()
            {
                mesh.triangles = null;
                meshRenderer.enabled = false;
                indices.Clear();
                vertices.Clear();
                colors.Clear();
                lineDirs.Clear();
                texCoords.Clear();
                glowModifiers.Clear();
                ends.Clear();
            }

            public Material Material
            {
                get { return GetComponent<MeshRenderer>().sharedMaterial; }
                set { GetComponent<MeshRenderer>().sharedMaterial = value; }
            }
        }

        private readonly Dictionary<LightningBolt, List<LineRendererMesh>> renderers = new Dictionary<LightningBolt, List<LineRendererMesh>>();
        private readonly List<LineRendererMesh> rendererCache = new List<LineRendererMesh>();

        private IEnumerator EnableRenderer(LineRendererMesh renderer, LightningBolt lightningBolt, float delay)
        {
            yield return new WaitForSeconds(delay);

            if (renderer != null && lightningBolt.ElapsedTime < lightningBolt.LifeTime)
            {
                renderer.Begin();
            }
        }

        private LineRendererMesh CreateLineRenderer(LightningBolt lightningBolt, List<LineRendererMesh> lineRenderers)
        {
            LineRendererMesh lineRenderer;

            if (rendererCache.Count == 0)
            {
                // create a line renderer in a new game object underneath the parent
                GameObject obj = new GameObject();
                obj.name = "LightningBoltMeshRenderer";
                obj.transform.parent = lightningBolt.Parent.transform;
                obj.transform.position = lightningBolt.Parent.transform.position;
                //obj.hideFlags = HideFlags.HideAndDontSave;
                lineRenderer = obj.AddComponent<LineRendererMesh>();
            }
            else
            {
                lineRenderer = rendererCache[rendererCache.Count - 1];
                rendererCache.RemoveAt(rendererCache.Count - 1);
            }
            lineRenderers.Add(lineRenderer);
            lineRenderer.Material = (lightningBolt.HasGlow ? Material : MaterialNoGlow);
            Script.StartCoroutine(EnableRenderer(lineRenderer, lightningBolt, lightningBolt.Delay));

            return lineRenderer;
        }

        public override void Begin(LightningBolt lightningBolt)
        {
            List<LineRendererMesh> lineRenderers;
            if (!renderers.TryGetValue(lightningBolt, out lineRenderers))
            {
                lineRenderers = new List<LineRendererMesh>();
                renderers[lightningBolt] = lineRenderers;
                CreateLineRenderer(lightningBolt, lineRenderers);
            }
        }

        public override void AddGroup(LightningBolt lightningBolt, LightningBoltSegmentGroup group, float growthMultiplier)
        {
            List<LineRendererMesh> lineRenderers = renderers[lightningBolt];
            LineRendererMesh lineRenderer = lineRenderers[lineRenderers.Count - 1];
            float timeStart = Time.time + group.Delay + lightningBolt.Delay;
            Color c = new Color(timeStart, timeStart + group.PeakStart, timeStart + group.PeakEnd, timeStart + group.LifeTime);
            float radius = group.LineWidth * 0.5f;
            int lineCount = group.Segments.Count - group.StartIndex;
            float radiusStep = (radius - (radius * group.EndWidthMultiplier)) / (float)lineCount;

            // growth multiplier
            float timeStep, timeOffset;
            if (growthMultiplier > 0.0f)
            {
                timeStep = (group.LifeTime / (float)lineCount) * growthMultiplier;
                timeOffset = 0.0f;
            }
            else
            {
                timeStep = 0.0f;
                timeOffset = 0.0f;
            }

            if (!lineRenderer.PrepareForLines(lineCount))
            {
                lineRenderer = CreateLineRenderer(lightningBolt, lineRenderers);
            }

            lineRenderer.BeginLine(group.Segments[group.StartIndex].Start, group.Segments[group.StartIndex].End, radius, c, GlowWidthMultiplier, GlowIntensityMultiplier);
            for (int i = group.StartIndex + 1; i < group.Segments.Count; i++)
            {
                radius -= radiusStep;
                if (growthMultiplier < 1.0f)
                {
                    timeOffset += timeStep;
                    c = new Color(timeStart + timeOffset, timeStart + group.PeakStart + timeOffset, timeStart + group.PeakEnd, timeStart + group.LifeTime);
                }
                lineRenderer.AppendLine(group.Segments[i].Start, group.Segments[i].End, radius, c, GlowWidthMultiplier, GlowIntensityMultiplier);
            }
        }

        public override void Render(LightningBolt lightningBolt)
        {
            
        }

        public override void Cleanup(LightningBolt lightningBolt)
        {
            List<LineRendererMesh> lineRenderers;
            if (renderers.TryGetValue(lightningBolt, out lineRenderers))
            {
                renderers.Remove(lightningBolt);
                foreach (LineRendererMesh lineRenderer in lineRenderers)
                {
                    rendererCache.Add(lineRenderer);
                    lineRenderer.Reset();
                }
            }
        }

        public override int ShaderIndex { get { return 1; } }

        public override bool CanGlow { get { return true; } }

        public Material MaterialNoGlow { get; set; }
    }

    /// <summary>
    /// Parameters that control lightning bolt behavior
    /// </summary>
    [System.Serializable]
    public class LightningBoltParameters
    {
        /// <summary>
        /// Start of the bolt
        /// </summary>
        public Vector3 Start;

        /// <summary>
        /// End of the bolt
        /// </summary>
        public Vector3 End;

        /// <summary>
        /// Number of generations (0 for just a point light, otherwise 1 - 8). Higher generations have lightning with finer detail but more expensive to create.
        /// </summary>
        public int Generations;

        /// <summary>
        /// How long the bolt should live in seconds
        /// </summary>
        public float LifeTime;

        /// <summary>
        /// How long to wait in seconds before starting the lightning bolt
        /// </summary>
        public float Delay;

        /// <summary>
        /// How chaotic is the lightning? (0 - 1). Higher numbers create more chaotic lightning.
        /// </summary>
        public float ChaosFactor;

        /// <summary>
        /// The width of the trunk
        /// </summary>
        public float TrunkWidth;

        /// <summary>
        /// The ending width of a segment of lightning
        /// </summary>
        public float EndWidthMultiplier = 0.5f;

        /// <summary>
        /// Intensity of glow (0-1)
        /// </summary>
        public float GlowIntensity;

        /// <summary>
        /// Glow width multiplier
        /// </summary>
        public float GlowWidthMultiplier;

        /// <summary>
        /// How forked the lightning should be, 0 for none, 1 for LOTS of forks
        /// </summary>
        public float Forkedness;

        /// <summary>
        /// Used to generate random numbers, can be null. Passing a random with the same seed and parameters will result in the same lightning.
        /// </summary>
        public System.Random Random;

        /// <summary>
        /// Controls the point light intensity generated for the lightning bolt. Range is 0 - 8, 0 for none.
        /// </summary>
        public float PointLightIntensity;

        /// <summary>
        /// The percent of time the lightning should fade in and out (0 - 1). Example: 0.2 would fade in for 20% of the lifetime and fade out for 20% of the lifetime. Set to 0 for no fade.
        /// </summary>
        public float FadePercent;

        /// <summary>
        /// A value between 0 and 1 that determines how fast the lightning should grow over the lifetime. A value of 1 grows slowest, 0 grows instantly
        /// </summary>
        public float GrowthMultiplier;
    }

    /// <summary>
    /// A group of lightning bolt segments, such as the main trunk of the lightning bolt
    /// </summary>
    public class LightningBoltSegmentGroup
    {
        /// <summary>
        /// Custom object for state tracking
        /// </summary>
        public object Tag;

#if ENABLE_LINE_RENDERER

        /// <summary>
        /// Color
        /// </summary>
        public Color Color;

#endif

        /// <summary>
        /// Width
        /// </summary>
        public float LineWidth;

        /// <summary>
        /// Start index of the segment to render (for performance, some segments are not rendered and only used for calculations)
        /// </summary>
        public int StartIndex;

        /// <summary>
        /// Generation
        /// </summary>
        public int Generation;

        /// <summary>
        /// Delay before rendering should start
        /// </summary>
        public float Delay;

        /// <summary>
        /// Peak start, the segments should be fully visible at this point
        /// </summary>
        public float PeakStart;

        /// <summary>
        /// Peak end, the segments should start to go away after this point
        /// </summary>
        public float PeakEnd;

        /// <summary>
        /// Total life time the group will be alive in seconds
        /// </summary>
        public float LifeTime;

        /// <summary>
        /// The width can be scaled down to the last segment by this amount if desired
        /// </summary>
        public float EndWidthMultiplier;

        /// <summary>
        /// Segments
        /// </summary>
        public readonly List<LightningBoltSegment> Segments = new List<LightningBoltSegment>();
    }

    /// <summary>
    /// A single segment of a lightning bolt
    /// </summary>
    public struct LightningBoltSegment
    {
        public Vector3 Start;
        public Vector3 End;

        public override string ToString()
        {
            return Start.ToString() + ", " + End.ToString();
        }
    }

    /// <summary>
    /// Lightning bolt
    /// </summary>
    public class LightningBolt
    {
        /// <summary>
        /// This is subtracted from the initial generations value, and any generation below that cannot have a fork
        /// </summary>
        public static int GenerationWhereForksStopSubtractor = 5;

        public GameObject Parent { get; private set; }
        public float LifeTime { get; private set; }
        public float ElapsedTime { get; private set; }
        public float Delay { get; private set; }
        public float CurrentDelayAmount { get; private set; }
        public bool HasGlow { get; private set; }
        public List<LightningBoltSegmentGroup> Groups { get { return segmentGroups; } }

        private static readonly List<LightningBoltSegmentGroup> groupCache = new List<LightningBoltSegmentGroup>();
        private int generationWhereForksStop;
        private LightningBoltRenderer lightningBoltRenderer;
        private LightningBoltScript script;
        private readonly List<LightningBoltSegmentGroup> segmentGroups = new List<LightningBoltSegmentGroup>();

        private GameObject lightningLightObject;
        private Light lightningLight;
        private float lightningLightIntensity;

        public LightningBolt(LightningBoltRenderer lightningBoltRenderer, GameObject parent, LightningBoltScript script,
            ParticleSystem originParticleSystem, ParticleSystem destParticleSystem,
            
#if ENABLE_LINE_RENDERER

            ParticleSystem glowParticleSystem,

#endif

            IEnumerable<LightningBoltParameters> parameters)
        {
            // setup properties
            if (parameters == null || lightningBoltRenderer == null)
            {
                return;
            }

            this.lightningBoltRenderer = lightningBoltRenderer;
            this.Parent = parent;
            this.script = script;

            // we need to know if there is glow so we can choose the glow or non-glow setting in the renderer
            foreach (LightningBoltParameters p in parameters)
            {
                HasGlow = ((

#if ENABLE_LINE_RENDERER

                    glowParticleSystem != null ||
                    
#endif
                    lightningBoltRenderer.CanGlow) && p.GlowIntensity > 0.0001f && p.GlowWidthMultiplier >= 0.0001f);

                if (HasGlow)
                {
                    break;
                }
            }

            // begin renderer
            lightningBoltRenderer.Begin(this);

            // add each parameter as part of one giant lightning bolt
            foreach (LightningBoltParameters p in parameters)
            {
                SetupLightningLight(p.PointLightIntensity);
                CurrentDelayAmount = 0.0f;
                Delay = p.Delay;
                generationWhereForksStop = p.Generations - GenerationWhereForksStopSubtractor;

                p.GlowIntensity = Mathf.Clamp(p.GlowIntensity, 0.0f, 1.0f);
                p.GlowWidthMultiplier = Mathf.Clamp(p.GlowWidthMultiplier, 0.0f, 64.0f);
                p.Random = (p.Random ?? new System.Random(System.Environment.TickCount));
                p.GrowthMultiplier = Mathf.Clamp(p.GrowthMultiplier, 0.0f, 0.999f);
                this.LifeTime = Mathf.Max(p.LifeTime, this.LifeTime);

                if (p.Generations > 0)
                {
                    // no more than 8 generations allowed, much higher then that it is computationally too expensive to calculate the bolt
                    p.Generations = Mathf.Clamp(p.Generations, 1, 8);
                    int forkedness = (int)(p.Forkedness * (float)p.Generations);
                    GenerateLightningBolt(p.Start, p.End, p.Generations, p.Generations, 0.0f, 0.0f, forkedness, p);
                    RenderLightningBolt(p.Start, p.End, originParticleSystem, destParticleSystem,
                        
#if ENABLE_LINE_RENDERER

                        glowParticleSystem,
                        
#endif

                        p);
                }
            }
        }

        private void SetupLightningLight(float intensity)
        {
            if (lightningLightObject == null && intensity > 0.0f)
            {
                lightningLightIntensity = intensity;

                // set up game object for the lightning and the point light
                lightningLightObject = new GameObject();
                lightningLightObject.name = "LightningBoltLight";
                lightningLight = lightningLightObject.AddComponent<Light>();
                lightningLight.type = LightType.Point;
                lightningLight.color = Color.white;
                lightningLight.range = 5000.0f;
                lightningLight.bounceIntensity = 1.0f;
                lightningLight.intensity = 0.0f;
                lightningLight.enabled = (Delay <= 0.0f);
                lightningLightObject.transform.parent = Parent.transform;
            }
        }

        private void RenderLightningBolt(Vector3 start, Vector3 end, ParticleSystem originParticleSystem,
            ParticleSystem destParticleSystem,
            
#if ENABLE_LINE_RENDERER
            
            ParticleSystem glowParticleSystem,
            
#endif
            
            LightningBoltParameters parameters)
        {
            if (segmentGroups.Count == 0)
            {
                return;
            }

            float delayBase = parameters.LifeTime / (float)segmentGroups.Count;
            float minDelayValue = delayBase * 0.9f;
            float maxDelayValue = delayBase * 1.1f;
            float delayDiff = maxDelayValue - minDelayValue;
            parameters.FadePercent = Mathf.Clamp(parameters.FadePercent, 0.0f, 0.5f);

            if (originParticleSystem != null)
            {
                // we have a strike, create a particle where the lightning is coming from
                script.StartCoroutine(GenerateParticle(originParticleSystem, start, Delay));
            }
            if (destParticleSystem != null)
            {
                script.StartCoroutine(GenerateParticle(destParticleSystem, end, Delay * 1.1f));
            }

#if ENABLE_LINE_RENDERER

            // setup the renderering for each group
            bool useParticleGlow = (HasGlow && !lightningBoltRenderer.CanGlow);

#endif

            if (HasGlow)
            {
                lightningBoltRenderer.GlowIntensityMultiplier = parameters.GlowIntensity;
                lightningBoltRenderer.GlowWidthMultiplier = parameters.GlowWidthMultiplier;
            }

            foreach (LightningBoltSegmentGroup group in segmentGroups)
            {
                // don't render anything that is one segment long
                if (group.Segments.Count < 2)
                {
                    continue;
                }

                // setup group parameters
                group.Delay = CurrentDelayAmount;
                group.LifeTime = LifeTime - CurrentDelayAmount;
                group.PeakStart = group.LifeTime * parameters.FadePercent;
                group.PeakEnd = group.LifeTime - group.PeakStart;

#if ENABLE_LINE_RENDERER

                // setup glow if needed
                if (useParticleGlow)
                {
                    script.StartCoroutine(RenderGroupGlow(group, glowParticleSystem, LifeTime - CurrentDelayAmount, Delay + CurrentDelayAmount, parameters.GlowIntensity, parameters.GlowWidthMultiplier));
                }

#endif

                lightningBoltRenderer.AddGroup(this, group, parameters.GrowthMultiplier);
                CurrentDelayAmount += ((float)parameters.Random.NextDouble() * minDelayValue) + delayDiff;
            }
        }

        private IEnumerator RenderGroupGlow(LightningBoltSegmentGroup g, ParticleSystem glowParticleSystem, float glowTime, float delayTime, float glowIntensity, float glowWidthMultiplier)
        {
            yield return new WaitForSeconds(delayTime);

#if ENABLE_LINE_RENDERER

            Camera camera = (Camera.current ?? Camera.main);
            const float overlap = 2.0f;
            float cameraTangent = Mathf.Tan(camera.fieldOfView * 0.5f * Mathf.Deg2Rad) * 2.0f;
            float oneOverPixelHeight = 1.0f / camera.pixelHeight;
            Color32 color = g.Color;
            color.a = (byte)((float)color.a * glowIntensity);

            for (int i = g.StartIndex; i < g.Segments.Count; i++)
            {
                Vector3 screenStart = camera.WorldToScreenPoint(g.Segments[i].Start);
                Vector3 screenEnd = camera.WorldToScreenPoint(g.Segments[i].End);
                screenStart.z = screenEnd.z = 0.0f;
                Vector3 worldMiddle = (g.Segments[i].Start + g.Segments[i].End) * 0.5f;
                float frustumHeight = Vector3.Distance(worldMiddle, camera.transform.position) * cameraTangent;
                float frustumPercent = frustumHeight * oneOverPixelHeight;
                float lineWidthScreen = g.LineWidth * (camera.pixelHeight / frustumHeight);
                float particleSize = (Vector2.Distance(screenStart, screenEnd) * overlap) + (lineWidthScreen * glowWidthMultiplier);
                particleSize = particleSize * frustumPercent;
                glowParticleSystem.Emit(worldMiddle, Vector3.zero, particleSize, glowTime, color);
            }

#endif

        }

        public void Cleanup()
        {
            foreach (LightningBoltSegmentGroup g in segmentGroups)
            {
                lightningBoltRenderer.RemoveGroup(g);
                g.Segments.Clear();
                g.StartIndex = 0;
                groupCache.Add(g);
            }

            if (lightningLightObject != null)
            {
                GameObject.Destroy(lightningLightObject);
            }

            lightningBoltRenderer.Cleanup(this);
        }

        public bool Update()
        {
            if (Delay > 0.0f)
            {
                // if we have a delay, check if it's expired, if not we return out without updating anything
                Delay -= Time.deltaTime;
                if (Delay <= 0)
                {
                    if (lightningLight != null)
                    {
                        lightningLight.enabled = true;
                    }
                    ElapsedTime = -Delay;
                }
                else
                {
                    return true;
                }
            }
            else
            {
                ElapsedTime += Time.deltaTime;
            }

            // check if we are expired
            if (ElapsedTime > LifeTime)
            {
                Cleanup();
                return false;
            }
            else if (lightningLight != null)
            {
                // depending on whether we have hit the mid point of our lifetime, fade the light in or out
                float lerp, multiplier1, multiplier2;
                float peakLifeTimeStart = LifeTime * 0.2f;
                float peakLifeTimeEnd = LifeTime * 0.8f;
                if (ElapsedTime >= peakLifeTimeStart)
                {
                    if (ElapsedTime <= peakLifeTimeEnd)
                    {
                        lerp = 1.0f;
                        multiplier1 = 1.0f;
                        multiplier2 = 1.0f;
                    }
                    else
                    {
                        lerp = (ElapsedTime - peakLifeTimeEnd) / (LifeTime - peakLifeTimeEnd);
                        multiplier1 = 1.0f;
                        multiplier2 = 0.0f;
                    }
                }
                else
                {
                    lerp = ElapsedTime / peakLifeTimeStart;
                    multiplier1 = 0.0f;
                    multiplier2 = 1.0f;
                }
                lightningLight.intensity = Mathf.Lerp(lightningLightIntensity * multiplier1, lightningLightIntensity * multiplier2, lerp);
            }

            lightningBoltRenderer.Render(this);

            return true;
        }

        private LightningBoltSegmentGroup CreateGroup()
        {
            LightningBoltSegmentGroup g;
            if (groupCache.Count == 0)
            {
                g = new LightningBoltSegmentGroup();
            }
            else
            {
                int index = groupCache.Count - 1;
                g = groupCache[index];
                groupCache.RemoveAt(index);
            }
            segmentGroups.Add(g);
            return g;
        }

        private IEnumerator GenerateParticle(ParticleSystem p, Vector3 pos, float delay)
        {
            yield return new WaitForSeconds(delay);

            p.transform.position = pos;
            p.Emit((int)p.emissionRate);
        }

        private void GenerateLightningBolt(Vector3 start, Vector3 end, int generation, int totalGenerations, float offsetAmount,
            float lineWidth, int forkedness, LightningBoltParameters parameters)
        {
            if (generation < 1)
            {
                return;
            }

            LightningBoltSegmentGroup group = CreateGroup();
            group.EndWidthMultiplier = parameters.EndWidthMultiplier;
            group.Segments.Add(new LightningBoltSegment { Start = start, End = end });

            // every generation, get the percentage we have gone down and square it, this makes lines thinner
            // and colors dimmer as we go down generations
            float widthAndColorMultiplier = (float)generation / (float)totalGenerations;
            widthAndColorMultiplier *= widthAndColorMultiplier;

            if (offsetAmount <= 0.0f)
            {
                offsetAmount = (end - start).magnitude * parameters.ChaosFactor;
            }
            if (lineWidth <= 0.0f)
            {
                group.LineWidth = parameters.TrunkWidth;
            }
            else
            {
                group.LineWidth = lineWidth * widthAndColorMultiplier;
            }

            group.LineWidth *= widthAndColorMultiplier;

#if ENABLE_LINE_RENDERER

            group.Color = new Color(1, 1, 1, widthAndColorMultiplier);

#endif

            group.Generation = generation - 1;

            while (generation-- > 0)
            {
                int previousStartIndex = group.StartIndex;
                group.StartIndex = group.Segments.Count;
                for (int i = previousStartIndex; i < group.StartIndex; i++)
                {
                    start = group.Segments[i].Start;
                    end = group.Segments[i].End;

                    // determine a new direction for the split
                    Vector3 midPoint = (start + end) * 0.5f;

                    // adjust the mid point to be the new location
                    midPoint += RandomVector(start, end, offsetAmount, parameters.Random);

                    // add two new segments
                    group.Segments.Add(new LightningBoltSegment { Start = start, End = midPoint });
                    group.Segments.Add(new LightningBoltSegment { Start = midPoint, End = end });

                    if (generation > generationWhereForksStop && generation >= totalGenerations - forkedness)
                    {
                        int branchChance = parameters.Random.Next(0, generation);
                        if (branchChance < forkedness)
                        {
                            // create a branch in the direction of this segment
                            Vector3 branchVector = (midPoint - start);
                            float multiplier = ((float)parameters.Random.NextDouble() * 0.2f) + 0.6f;
                            Vector3 splitEnd = midPoint + (branchVector * multiplier);
                            GenerateLightningBolt(midPoint, splitEnd, generation, totalGenerations, 0.0f, lineWidth, forkedness, parameters);
                        }
                    }
                }

                // halve the distance the lightning can deviate for each generation down
                offsetAmount *= 0.5f;
            }
        }

        private void GetSideVector(ref Vector3 directionNormalized, out Vector3 side)
        {
            if (directionNormalized == Vector3.zero)
            {
                side = Vector3.right;
            }
            else
            {
                // use cross product to find any perpendicular vector around directionNormalized:
                // 0 = x * px + y * py + z * pz; => pz = -(x * px + y * py) / z
                // for computational stability use the component farthest from 0 to divide by
                float x = directionNormalized.x;
                float y = directionNormalized.y;
                float z = directionNormalized.z;
                float px, py, pz;
                float ax = Mathf.Abs(x);
                float ay = Mathf.Abs(y);
                float az = Mathf.Abs(z);

                if (ax >= ay && ay >= az)
                {
                    // x is the max, so we can pick (py, pz) arbitrarily at (1, 1):
                    py = 1.0f;
                    pz = 1.0f;
                    px = -(y * py + z * pz) / x;
                }
                else if (ay >= az)
                {
                    // y is the max, so we can pick (px, pz) arbitrarily at (1, 1):
                    px = 1.0f;
                    pz = 1.0f;
                    py = -(x * px + z * pz) / y;
                }
                else
                {
                    // z is the max, so we can pick (px, py) arbitrarily at (1, 1):
                    px = 1.0f;
                    py = 1.0f;
                    pz = -(x * px + y * py) / z;
                }
                side = new Vector3(px, py, pz).normalized;
            }
        }

        private Vector3 RandomVector(Vector3 start, Vector3 end, float offsetAmount, System.Random random)
        {
            if ((Camera.current ?? Camera.main).orthographic)
            {
                // ensure that lightning moves in only x, y giving it a more varied appearance than 3D which can move in the z direction too
                Vector3 directionNormalized = (end - start).normalized;
                Vector3 side = new Vector3(-directionNormalized.y, directionNormalized.x, directionNormalized.z);
                float distance = ((float)random.NextDouble() * offsetAmount * 2.0f) - offsetAmount;
                return side * distance;
            }
            else
            {
                // get unit vector for direction
                Vector3 directionNormalized = (end - start).normalized;

                // get perpendicular vector
                Vector3 side = Vector3.Cross(start, end).normalized;

                // generate random distance
                float distance = (((float)random.NextDouble() + 0.1f) * offsetAmount);

                // get random rotation angle to rotate around the current direction
                float rotationAngle = ((float)random.NextDouble() * 360.0f);

                // rotate around the direction and then offset by the perpendicular vector
                return Quaternion.AngleAxis(rotationAngle, directionNormalized) * side * distance;
            }
        }
    }

    /// <summary>
    /// Rendering mode
    /// </summary>
    public enum LightningBoltRenderMode
    {

#if ENABLE_LINE_RENDERER

        /// <summary>
        /// Use legacy line renderer
        /// </summary>
        LineRenderer,

#endif

        /// <summary>
        /// Renderer with a square billboard glow
        /// </summary>
        MeshRendererSquareBillboardGlow,

        /// <summary>
        /// Renderer with a glow that follows the shape of the line more
        /// </summary>
        MeshRendererLineGlow
    }

    public class LightningBoltScript : MonoBehaviour
    {
        private const LightningBoltRenderMode defaultRenderMode = LightningBoltRenderMode.MeshRendererLineGlow;
        private LightningBoltRenderer lightningBoltRenderer;

        [Tooltip("Whether to use the mesh renderer or the line renderer to draw the lightning.")]
        public LightningBoltRenderMode RenderMode = defaultRenderMode;

#if ENABLE_LINE_RENDERER

        [Tooltip("Lightning material for line renderer")]
        public Material LightningMaterialLineRenderer;

#endif

        [Tooltip("Lightning material for mesh renderer")]
        public Material LightningMaterialMesh;

        [Tooltip("Lightning material for mesh renderer, without glow")]
        public Material LightningMaterialMeshNoGlow;

        [Tooltip("The texture to use for the lightning bolts, or null for the material default texture.")]
        public Texture2D LightningTexture;

        [Tooltip("Particle system to play at the point of emission (start). Emission rate particles will be emitted all at once.")]
        public ParticleSystem LightningOriginParticleSystem;

        [Tooltip("Particle system to play at the point of impact (end). Emission rate particles will be emitted all at once.")]
        public ParticleSystem LightningDestinationParticleSystem;

        [Tooltip("Tint color for the lightning")]
        public Color LightningTintColor = Color.white;

        [Tooltip("Tint color for the lightning glow")]
        public Color GlowTintColor = new Color(0.1f, 0.2f, 1.0f, 1.0f);

        [Tooltip("Jitter multiplier to randomize lightning size. Jitter depends on trunk width and will make the lightning move rapidly and jaggedly " +
            "giving a more life-like feel.")]
        public float JitterMultiplier = 0.0f;

#if ENABLE_LINE_RENDERER

        [Tooltip("Lightning glow particle system")]
        public ParticleSystem LightningGlowParticleSystem;

         private Material lightningMaterialLineRendererInternal;

#endif

        private Material lightningMaterialMeshInternal;
        private Material lightningMaterialMeshNoGlowInternal;
        private Texture2D lastLightningTexture;
        private LightningBoltRenderMode lastRenderMode = (LightningBoltRenderMode)0x7FFFFFFF;
        private readonly List<LightningBolt> bolts = new List<LightningBolt>();
        private readonly LightningBoltParameters[] oneParameterArray = new LightningBoltParameters[1];

        protected virtual void Start()
        {
            if (
                
#if ENABLE_LINE_RENDERER
                
                LightningMaterialLineRenderer == null ||
                
#endif
                
                LightningMaterialMesh == null || LightningMaterialMeshNoGlow == null)
            {
                Debug.LogError("Must assign all lightning materials");
            }

            UpdateMaterialsForLastTexture();
        }

        private void UpdateMaterialsForLastTexture()
        {
            if (!Application.isPlaying)
            {
                return;
            }

#if ENABLE_LINE_RENDERER

            lightningMaterialLineRendererInternal = new Material(LightningMaterialLineRenderer);

#endif

            lightningMaterialMeshInternal = new Material(LightningMaterialMesh);
            lightningMaterialMeshNoGlowInternal = new Material(LightningMaterialMeshNoGlow);

            if (LightningTexture != null)
            {

#if ENABLE_LINE_RENDERER

                lightningMaterialLineRendererInternal.SetTexture("_MainTex", LightningTexture);

#endif

                lightningMaterialMeshInternal = new Material(LightningMaterialMesh);
                lightningMaterialMeshInternal.SetTexture("_MainTex", LightningTexture);
                lightningMaterialMeshNoGlowInternal = new Material(LightningMaterialMeshNoGlow);
                lightningMaterialMeshNoGlowInternal.SetTexture("_MainTex", LightningTexture);
            }

            lastRenderMode = (LightningBoltRenderMode)0x7FFFFFFF;

            CreateRenderer();
        }

        private void CreateRenderer()
        {
            if (RenderMode == lastRenderMode)
            {
                return;
            }
            lastRenderMode = RenderMode;

#if ENABLE_LINE_RENDERER

            if (RenderMode == LightningBoltRenderMode.LineRenderer)
            {
                lightningBoltRenderer = new LightningBoltLineRenderer();
            }
            else

#endif

            {
                lightningBoltRenderer = new LightningBoltMeshRenderer();
                (lightningBoltRenderer as LightningBoltMeshRenderer).MaterialNoGlow = lightningMaterialMeshNoGlowInternal;

                if (RenderMode == LightningBoltRenderMode.MeshRendererSquareBillboardGlow)
                {
                    lightningMaterialMeshInternal.DisableKeyword("USE_LINE_GLOW");
                }
                else
                {
                    lightningMaterialMeshInternal.EnableKeyword("USE_LINE_GLOW");                    
                }
            }

            lightningBoltRenderer.Script = this;

#if ENABLE_LINE_RENDERER

            lightningBoltRenderer.Material = (lightningBoltRenderer.ShaderIndex == 0 ? LightningMaterialLineRenderer : LightningMaterialMesh);

#else

            lightningBoltRenderer.Material = lightningMaterialMeshInternal;

#endif

        }

        private void UpdateTexture()
        {
            if (LightningTexture != null && LightningTexture != lastLightningTexture)
            {
                lastLightningTexture = LightningTexture;
                UpdateMaterialsForLastTexture();
            }
            else
            {
                CreateRenderer();
            }
        }

        private void UpdateShaderParameters()
        {
            lightningMaterialMeshInternal.SetColor("_TintColor", LightningTintColor);
            lightningMaterialMeshInternal.SetColor("_GlowTintColor", GlowTintColor);
            lightningMaterialMeshInternal.SetFloat("_JitterMultiplier", JitterMultiplier);
            lightningMaterialMeshNoGlowInternal.SetColor("_TintColor", LightningTintColor);
            lightningMaterialMeshNoGlowInternal.SetFloat("_JitterMultiplier", JitterMultiplier);
        }

        protected virtual void Update()
        {
            if (bolts.Count == 0)
            {
                return;
            }

            UpdateShaderParameters();
            for (int i = bolts.Count - 1; i >= 0; i--)
            {
                // update each lightning bolt, removing it if update returns false
                if (!bolts[i].Update())
                {
                    bolts.RemoveAt(i);
                }
            }
        }

        /// <summary>
        /// Create a lightning bolt
        /// </summary>
        /// <param name="p">Lightning bolt creation parameters</param>
        public virtual void BatchLightningBolt(LightningBoltParameters p)
        {
            if (p != null)
            {
                UpdateTexture();
                oneParameterArray[0] = p;
                LightningBolt bolt = new LightningBolt(lightningBoltRenderer, gameObject, this, LightningOriginParticleSystem,
                    LightningDestinationParticleSystem,
                    
#if ENABLE_LINE_RENDERER
                    
                    LightningGlowParticleSystem,
                    
#endif
                    
                    oneParameterArray);
                bolts.Add(bolt);
            }
        }

        /// <summary>
        /// Create multiple lightning bolts, attempting to batch them into as few draw calls as possible
        /// </summary>
        /// <param name="parameters">Lightning bolt creation parameters</param>
        public void CreateLightningBolts(IEnumerable<LightningBoltParameters> parameters)
        {
            if (parameters != null)
            {
                UpdateTexture();
                LightningBolt bolt = new LightningBolt(lightningBoltRenderer, gameObject, this, LightningOriginParticleSystem,
                    LightningDestinationParticleSystem,
                    
#if ENABLE_LINE_RENDERER
                    
                    LightningGlowParticleSystem,
                    
#endif
                    
                    parameters);
                bolts.Add(bolt);
            }
        }
    }
}