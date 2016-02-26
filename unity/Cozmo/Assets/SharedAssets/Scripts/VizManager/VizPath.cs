using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Cozmo.Util;

namespace Anki.Cozmo.Viz {

  public class VizPath : MonoBehaviour {

    [SerializeField]
    private float pathWidth = 0.001f;

    [SerializeField]
    private MeshRenderer _Renderer;

    [SerializeField]
    private MeshFilter _Filter;

    private Mesh _Mesh;

    private void Awake() {
      _Mesh = new Mesh() { name = "VizPathMesh", hideFlags = HideFlags.HideAndDontSave };
      _Filter.sharedMesh = _Mesh;
    }

    private void OnDestroy() {
      Destroy(_Mesh);
    }

    public void Initialize(string name) {
      _Mesh.name = "VizPathMesh_" + name;
      gameObject.name = name;

    }
    private int _Index = 0;

    private List<Vector3> _Vertices = new List<Vector3>();
    private List<int> _Triangles = new List<int>();
    private List<Vector3> _Normals = new List<Vector3>();
    private List<Color> _Colors = new List<Color>();

    private Color _Color = Color.red;
    public Color Color { 
      get { return _Color; }
      set {
        _Color = value;
        _Colors.Fill(_Color);
      }
    }

    public void SetSegment(Vector3 a, Vector3 b, Color color) {
      Color = color;
      BeginPath();
      AppendSegment(a, b, color);
    }

    public void BeginPath() {
      _Index = 0;
    }

    public void AppendLine(Vector3 a, Vector3 b) {    
      AppendSegment(a, b, Color);
    }

    public void AppendSegment(Vector3 a, Vector3 b, Color color) {
      Vector3 dir = (b - a).normalized;

      var cross = Vector3.Cross(Vector3.forward, dir);

      // if the line is straight up, make our up axis -x. 
      if (cross.sqrMagnitude < Mathf.Epsilon) {
        cross = Vector3.Cross(Vector3.left, dir);
      }
      var perp = cross.normalized;

      // if our line is on the ground, this will be (0,0,1)
      var normal = Vector3.Cross(dir, perp);

      var offset = perp * pathWidth;

      for (int i = _Vertices.Count; i < _Index + 6; i++) {
        // permanent
        _Triangles.Add(i);

        // temporary
        _Colors.Add(color);
        _Vertices.Add(Vector3.zero);
        _Normals.Add(Vector3.zero);
      }

      _Vertices[_Index] = (a + offset);
      _Vertices[_Index + 1] = (b + offset);
      _Vertices[_Index + 2] = (b - offset);

      _Vertices[_Index + 3] = (b - offset);
      _Vertices[_Index + 4] = (a - offset);
      _Vertices[_Index + 5] = (a + offset);

      for (int i = 0; i < 6; i++) {
        _Normals[_Index + i] = normal;
        _Colors[_Index + i] = color;
      }
      _Index += 6;

      UpdateMesh();
    }

    public void AppendArc(Vector2 center, float radius, float arcStartRad, float arcSweepRad) {    

      var normal = Vector3.forward;

      //lets make our step 10 deg, or
      float step = Mathf.PI / 18f;

      float mult = -1f;
      if (arcSweepRad < 0) {
        mult = 1f;
        step = -step;      
      }
       
      int count = Mathf.CeilToInt(arcSweepRad / step);

      // prefill
      for (int i = _Vertices.Count; i < _Index + count * 6; i++) {
        // permanent
        _Colors.Add(Color);
        _Triangles.Add(i);

        // temporary
        _Vertices.Add(Vector3.zero);
        _Normals.Add(Vector3.zero);
      }

      Vector2 lastAngleVector = new Vector2(Mathf.Cos(arcStartRad), Mathf.Sin(arcStartRad));
      for (int i = 1; i <= count; i++) {

        float angleB = arcStartRad + step * i;
        if (i == count) {
          angleB = arcStartRad + arcSweepRad;
        }
          
        Vector2 va = lastAngleVector;
        Vector2 vb = new Vector2(Mathf.Cos(angleB), Mathf.Sin(angleB));
        lastAngleVector = vb;

        var a = center + va * radius;
        var b = center + vb * radius;
        var offsetA = mult * va * pathWidth;
        var offsetB = mult * vb * pathWidth;

        _Vertices[_Index] = (a + offsetA);
        _Vertices[_Index + 1] = (b + offsetB);
        _Vertices[_Index + 2] = (b - offsetB);

        _Vertices[_Index + 3] = (b - offsetB);
        _Vertices[_Index + 4] = (a - offsetA);
        _Vertices[_Index + 5] = (a + offsetA);

        _Normals[_Index] = normal;
        _Normals[_Index + 1] = normal;
        _Normals[_Index + 2] = normal;

        _Normals[_Index + 3] = normal;
        _Normals[_Index + 4] = normal;
        _Normals[_Index + 5] = normal;
        _Index += 6;
      }

      UpdateMesh();
    }

    public void UpdateMesh() {
      if (_Index < _Vertices.Count) {
        _Vertices.RemoveRange(_Index, _Vertices.Count - _Index);
        _Normals.RemoveRange(_Index, _Normals.Count - _Index);
        _Colors.RemoveRange(_Index, _Colors.Count - _Index);
        _Triangles.RemoveRange(_Index, _Triangles.Count - _Index);
      }

      _Mesh.SetVertices(_Vertices);
      _Mesh.SetNormals(_Normals);
      _Mesh.SetColors(_Colors);
      _Mesh.SetTriangles(_Triangles, 0);
      _Mesh.RecalculateBounds();
      _Mesh.UploadMeshData(false);
    }
  }
}
