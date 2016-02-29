using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Anki.Cozmo.Viz {

  public class VizQuad : MonoBehaviour {

    [SerializeField]
    private float borderWidth = -0.002f;

    [SerializeField]
    private MeshRenderer _Renderer;

    [SerializeField]
    private MeshFilter _Filter;

    private Mesh _Mesh;

    private const int _kVerticesPerQuad = 6;
    private const int _kVerticesPerQuadBorder = 24;
    private const int _kTrainglesPerBorderedQuad = 10;

    // Used for rendering objects so we don't have regenerate meshes all the time.
    public Vector3 DrawScale { get; set; }
    public Color DrawColor { get; set; }

    private int _Index = 0;

    private void Awake() {
      _Mesh = new Mesh() { name = "VizQuadMesh", hideFlags = HideFlags.HideAndDontSave };
      _Filter.sharedMesh = _Mesh;
    }

    private void OnDestroy() {
      Destroy(_Mesh);
    }

    public void Initialize(string name) {
      _Mesh.name = "VizQuadMesh_" + name;
      gameObject.name = name;

    }

    private List<Vector3> _Vertices = new List<Vector3>();
    private List<int> _Triangles = new List<int>();
    private List<Vector3> _Normals = new List<Vector3>();
    private List<Color> _Colors = new List<Color>();

    public void UpdateQuad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color innerColor, Color outerColor) {    
      // A single quad is just a list with one quad
      StartUpdateQuadList();
      AddToQuadList(a, b, c, d, innerColor, outerColor);
      EndUpdateQuadList();
    }

    public void StartUpdateQuadList() {
      _Index = 0;
    }    

    public void AddToQuadList(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color innerColor, Color outerColor) {

      UpdateVertices(a, b, c, d, innerColor, outerColor);
    }

    private void EnsureSize(int size) {
      // _Vertices, _Normals, and _Colors must match
      for(int i = _Vertices.Count; i < _Index + size; i++) {
        _Vertices.Add(Vector3.zero);
        _Normals.Add(Vector3.zero);
        _Colors.Add(Color.clear);
        _Triangles.Add(i);
      }
    }

    public void EndUpdateQuadList() {

      if (_Index < _Vertices.Count) {
        _Vertices.RemoveRange(_Index, _Vertices.Count - _Index);
        _Normals.RemoveRange(_Index, _Normals.Count - _Index);
        _Colors.RemoveRange(_Index, _Colors.Count - _Index);
        _Triangles.RemoveRange(_Index, _Triangles.Count - _Index);
      }

      // clear triangles before updating vertices in case our count went down
      _Mesh.SetTriangles(Empty<int>.Instance, 0);
      _Mesh.SetVertices(_Vertices);
      _Mesh.SetNormals(_Normals);
      _Mesh.SetColors(_Colors);
      _Mesh.SetTriangles(_Triangles, 0);
      _Mesh.RecalculateBounds();
      _Mesh.UploadMeshData(false);
    }

    private void UpdateVertices(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color innerColor, Color outerColor) {

      int vi = _Index;
      var ab = (a - b).normalized * borderWidth;
      var bc = (b - c).normalized * borderWidth;
      var cd = (c - d).normalized * borderWidth;
      var da = (d - a).normalized * borderWidth;


      // assume vertices are coplanar
      var normal = Vector3.Cross(a - b, b - c).normalized;


      if (innerColor != Color.clear) {
        EnsureSize(_kVerticesPerQuad);

        // assume vertices are clockwise
        _Vertices[vi++] = a;
        _Vertices[vi++] = b;
        _Vertices[vi++] = c;

        _Vertices[vi++] = c;
        _Vertices[vi++] = d;
        _Vertices[vi++] = a;

        for (int i = 0; i < _kVerticesPerQuad; i++) {
          _Colors[i + _Index] = innerColor;
          _Normals[i + _Index] = normal;
        }

        _Index += _kVerticesPerQuad;
      }

      if (outerColor != Color.clear) {
        EnsureSize(_kVerticesPerQuadBorder);

        // top border;
        _Vertices[vi++] = a + ab - da;
        _Vertices[vi++] = b - ab + bc;
        _Vertices[vi++] = b;

        _Vertices[vi++] = b;
        _Vertices[vi++] = a;
        _Vertices[vi++] = a + ab - da;


        // right border;
        _Vertices[vi++] = b + bc - ab;
        _Vertices[vi++] = c - bc + cd;
        _Vertices[vi++] = c;

        _Vertices[vi++] = c;
        _Vertices[vi++] = b;
        _Vertices[vi++] = b + bc - ab;


        // bottom border;
        _Vertices[vi++] = c + cd - bc;
        _Vertices[vi++] = d - cd + da;
        _Vertices[vi++] = d;

        _Vertices[vi++] = d;
        _Vertices[vi++] = c;
        _Vertices[vi++] = c + cd - bc;


        // left border;
        _Vertices[vi++] = d + da - cd;
        _Vertices[vi++] = a - da + ab;
        _Vertices[vi++] = a;

        _Vertices[vi++] = a;
        _Vertices[vi++] = d;
        _Vertices[vi++] = d + da - cd;

        for (int i = 0; i < _kVerticesPerQuadBorder; i++) {
          _Colors[i + _Index] = outerColor;
          _Normals[i + _Index] = normal;
        }
        _Index += _kVerticesPerQuadBorder;
      }
    }
  }
}