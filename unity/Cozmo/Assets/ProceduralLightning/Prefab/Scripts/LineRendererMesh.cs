//
// Procedural Lightning for Unity
// (c) 2015 Digital Ruby, LLC
// Source code may be used for personal or commercial projects.
// Source code may NOT be redistributed or sold.
// 

using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DigitalRuby.ThunderAndLightning
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
				//Debug.Log("LightningBoltMeshRenderer Update meshRenderer.enabled("+meshRenderer.enabled+")");
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
			//Debug.Log("LightningBoltMeshRenderer Begin meshRenderer.enabled("+meshRenderer.enabled+")");
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

		public int VertCount
		{
			get { return vertices.Count; }
		}
    }

}