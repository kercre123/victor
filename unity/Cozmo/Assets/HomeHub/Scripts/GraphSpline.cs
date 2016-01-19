using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class GraphSpline : MonoBehaviour {

  [SerializeField]
  private CanvasRenderer _CanvasRenderer;
  [SerializeField]
  private Material _Material;

  [SerializeField]
  private Mesh _Mesh;

  private List<float> _Points;

  #if DEBUG_SPLINE
  private List<Vector2> _SplinePoints;
  #endif

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

    float deltaX = 1f / _Points.Count;

    const float armLength = 0.2f;
    const float pointOffset = 0.4f;

    List<Vector2> splinePoints = new List<Vector2>();

    Vector2 start = new Vector2(deltaX * 0.5f, _Points[0]);

    splinePoints.Add(start);
    splinePoints.Add(start);

    for (int i = 1; i < _Points.Count - 1; i++) {

      // go slightly down each arm to get a slope
      var a = Mathf.Lerp(_Points[i], _Points[i - 1], pointOffset);

      Vector2 pointA = new Vector2((i - pointOffset + 0.5f) * deltaX, a);
      Vector2 deltaA = (new Vector2(deltaX, (_Points[i] - _Points[i - 1]))) * armLength;

      var b = Mathf.Lerp(_Points[i], _Points[i + 1], pointOffset);

      Vector2 pointB = new Vector2((i + pointOffset + 0.5f) * deltaX, b);
      Vector2 deltaB = (new Vector2(deltaX, (_Points[i + 1] - _Points[i]))) * armLength;

      // end straight segment
      splinePoints.Add(pointA);
      splinePoints.Add(pointA);

      // add bezier curve
      splinePoints.Add(pointA);
      splinePoints.Add(pointA + deltaA);
      splinePoints.Add(pointB - deltaB);
      splinePoints.Add(pointB);

      // begin next straight segment
      splinePoints.Add(pointB);
      splinePoints.Add(pointB);
    }
    Vector2 end = new Vector2(deltaX * (_Points.Count - 0.5f), _Points[_Points.Count - 1]);

    splinePoints.Add(end);
    splinePoints.Add(end);

    List<Vector2> evaluatedPoints = new List<Vector2>();

    for (int i = 0; i < splinePoints.Count; i += 4) {
      EvaluateSplinePoints(splinePoints[i], splinePoints[i + 1], splinePoints[i + 2], splinePoints[i + 3], evaluatedPoints);
    }

    #if DEBUG_SPLINE
    _SplinePoints = splinePoints;
    #endif

    var rt = GetComponent<RectTransform>();
    Vector3 scale = rt.rect.size;
      
    Vector3[] vertices = new Vector3[evaluatedPoints.Count * 2];    
    Vector3[] normals = new Vector3[evaluatedPoints.Count * 2]; 
    Vector2[] uvs = new Vector2[evaluatedPoints.Count * 2];
    int[] triangles = new int[(evaluatedPoints.Count - 1) * 6];

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

    #if DEBUG_SPLINE

    var rt = GetComponent<RectTransform>();
    Vector3 scale = rt.rect.size;

    for(int i = 0; i < _SplinePoints.Count; i+=4) {

      var a = _SplinePoints[i];
      var b = _SplinePoints[i + 1];
      var c = _SplinePoints[i + 2];
      var d = _SplinePoints[i + 3];
      a.Scale(scale);
      b.Scale(scale);
      c.Scale(scale);
      d.Scale(scale);

      UnityEngine.Debug.DrawLine(a, b, Color.red);
      UnityEngine.Debug.DrawLine(b, c, Color.red);
      UnityEngine.Debug.DrawLine(c, d, Color.red);

    }

    float deltaX = 1f / _Points.Count;

    for(int i = 1; i < _Points.Count; i++) {

      var a = new Vector2(deltaX * (i - 0.5f), _Points[i - 1]);
      var b = new Vector2(deltaX * (i + 0.5f), _Points[i]);
      a.Scale(scale);
      b.Scale(scale);

      UnityEngine.Debug.DrawLine(a, b, Color.blue);

    }

    #endif
  }

  private void EvaluateSplinePoints(Vector2 a, Vector2 b, Vector2 c, Vector2 d, List<Vector2> evaluatedPoints) {

    // if our curve is a line, just add the end points
    if (Mathf.Approximately((a - b).sqrMagnitude, 0f) && Mathf.Approximately((c - d).sqrMagnitude, 0f)) {
      evaluatedPoints.Add(a);
      evaluatedPoints.Add(d);
      return;
    }

    const int steps = 10;
    const float inv = 1f / steps;

    for (int i = 0; i <= steps; i++) {
      float t = i * inv;

      var ab = Vector2.Lerp(a,b,t);
      var bc = Vector2.Lerp(b,c,t);
      var cd = Vector2.Lerp(c,d,t);

      var abbc = Vector2.Lerp(ab,bc,t);
      var bccd = Vector2.Lerp(bc,cd,t);

      var evaluatedPoint = Vector2.Lerp(abbc, bccd, t);
      // don't allow negative numbers
      evaluatedPoint.y = Mathf.Max(evaluatedPoint.y, 0);
      evaluatedPoints.Add(evaluatedPoint);
    }
  }

  private void OnRectTransformDimensionsChange() {
    RebuildMesh();
  }

}
