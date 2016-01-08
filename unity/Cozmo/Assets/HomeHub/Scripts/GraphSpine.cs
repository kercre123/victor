using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using System.Linq;

public class GraphSpine : MonoBehaviour {

  [SerializeField]
  private CanvasRenderer _CanvasRenderer;
  [SerializeField]
  private Material _Material;

  private Mesh _Mesh;

  private List<float> _Points;

  private void Awake() {
    _Mesh = new Mesh();
    _Mesh.name = "Graph Mesh";
  }

  public void Initialize(List<float> points) {
    _Points = points;
    RebuildMesh();
  }

  private void RebuildMesh() {
    if (_Points == null || _Points.Count < 2) {
      return;
    }

    float max = _Points.Max();
    float yMultiplier = Mathf.Approximately(max, 0f) ? 0f : 1f / max;
    float deltaX = 1f / (_Points.Count + 1);

    float bezierOffset = 0.7f * deltaX;

    List<Vector2> splinePoints = new List<Vector2>();

    Vector2 start = new Vector2(deltaX * 0.5f, _Points[0] * yMultiplier);

    splinePoints.Add(start);

    splinePoints.Add(start + (new Vector2(deltaX, (_Points[1] - _Points[0]) * yMultiplier)).normalized * bezierOffset);

    for (int i = 1; i < _Points.Count - 1; i++) {
      Vector2 point = new Vector2((i + 0.5f) * deltaX, _Points[i] * yMultiplier);

      Vector2 delta = (new Vector2(deltaX * 2, (_Points[i + 1] - _Points[i - 1]) * yMultiplier)).normalized * bezierOffset;

      splinePoints.Add(point - delta);
      splinePoints.Add(point);
      splinePoints.Add(point);
      splinePoints.Add(point + delta);
    }
    Vector2 end = new Vector2(deltaX * (_Points.Count - 0.5f), _Points[_Points.Count - 1] * yMultiplier);

    splinePoints.Add(end - (new Vector2(deltaX, (_Points[_Points.Count - 1] - _Points[_Points.Count - 2]) * yMultiplier)).normalized * bezierOffset);

    splinePoints.Add(end);

    List<Vector2> evaluatedPoints = new List<Vector2>();

    for (int i = 0; i < splinePoints.Count; i += 4) {
      EvaluateSplinePoints(splinePoints[i], splinePoints[i + 1], splinePoints[i + 2], splinePoints[i + 3], evaluatedPoints);
    }
      
    Vector3[] vertices = new Vector3[evaluatedPoints.Count * 2];    
    Vector3[] normals = new Vector3[evaluatedPoints.Count * 2]; 
    Vector2[] uvs = new Vector2[evaluatedPoints.Count * 2];
    int[] triangles = new int[(evaluatedPoints.Count - 1) * 6];


    var rt = GetComponent<RectTransform>();

    Vector3 scale = rt.rect.size;

    vertices[0] = evaluatedPoints[0];
    vertices[0].Scale(scale);
    vertices[1] = vertices[0] - vertices[0].y * Vector3.up;
    uvs[0] = evaluatedPoints[0];
    uvs[1] = uvs[0] - uvs[0].y * Vector2.up;
    normals[0] = Vector3.back;
    normals[1] = Vector3.back;


    for (int i = 1; i < evaluatedPoints.Count; i++) {
      vertices[i * 2] = evaluatedPoints[i];
      vertices[i * 2].Scale(scale);
      vertices[i * 2 + 1] = vertices[i * 2] - vertices[i * 2].y * Vector3.up;

      uvs[i * 2] = evaluatedPoints[i];
      uvs[i * 2 + 1] = new Vector2(evaluatedPoints[i].x, 0);

      normals[i * 2] = Vector3.back;
      normals[i * 2 + 1] = Vector3.back;

      triangles[i * 6 - 6] = i * 2;
      triangles[i * 6 - 5] = i * 2 + 1;
      triangles[i * 6 - 4] = i * 2 - 1;
      triangles[i * 6 - 3] = i * 2;
      triangles[i * 6 - 2] = i * 2 - 1;
      triangles[i * 6 - 1] = i * 2 - 2;
    }

    _Mesh.vertices = vertices;
    _Mesh.triangles = triangles;
    _Mesh.normals = normals;
    _Mesh.uv = uvs;

    _Mesh.UploadMeshData(false);
    _Mesh.RecalculateBounds();
  }

  private void Update() {
    _CanvasRenderer.Clear();
    _CanvasRenderer.materialCount = 1;
    _CanvasRenderer.SetMaterial(_Material, 0);
    _CanvasRenderer.SetMesh(_Mesh);
  }

  private void EvaluateSplinePoints(Vector2 a, Vector2 b, Vector2 c, Vector2 d, List<Vector2> evaluatedPoints) {

    const int steps = 10;
    const float inv = 1f / steps;

    for (int i = 0; i < steps; i++) {
      float t = i * inv;

      var ab = Vector2.Lerp(a,b,t);
      var bc = Vector2.Lerp(b,c,t);
      var cd = Vector2.Lerp(c,d,t);

      var abbc = Vector2.Lerp(ab,bc,t);
      var bccd = Vector2.Lerp(bc,cd,t);

      evaluatedPoints.Add(Vector2.Lerp(abbc, bccd, t));
    }
  }

}
