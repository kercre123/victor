// Version 4.2
// ©2015 Starscene Software. All rights reserved. Redistribution of source code without permission not allowed.

using UnityEngine;
using System.Collections.Generic;

namespace Vectrosity {

  public enum LineType {
    Continuous,
    Discrete

  }

  public enum Joins {
    Fill,
    Weld,
    None

  }

  public enum EndCap {
    Front,
    Both,
    Mirror,
    Back,
    None

  }

  public enum Visibility {
    Dynamic,
    Static,
    Always,
    None

  }

  public enum Brightness {
    Fog,
    None

  }

  [System.Serializable]
  public class VectorLine {
    public static string Version() {
      return "Vectrosity version 4.2";
    }

    UIVertex[] m_UIVertices;
    UIVertex[] m_capVertices;
    UIVertex[] m_fillVertices;
    GameObject m_vectorObject;
    CanvasRenderer m_canvasRenderer;
    CanvasRenderer m_capRenderer;
    CanvasRenderer m_fillRenderer;
    RectTransform m_rectTransform;

    public RectTransform rectTransform {
      get {
        if (m_vectorObject != null) {
          return m_rectTransform;
        }
        return null;
      }
    }

    bool m_on2DCanvas;
    int adjustEnd;
    Color32 m_color;

    public Color32 color {
      get { return m_color; }
      set {
        m_color = value;
        SetColor(value);
      }
    }

    List<Vector2> m_points2;

    public List<Vector2> points2 {
      get {
        if (!m_is2D) {
          Debug.LogError("Line \"" + name + "\" uses points3 rather than points2");
          return null;
        }
        return m_points2;
      }
    }

    List<Vector3> m_points3;

    public List<Vector3> points3 {
      get {
        if (m_is2D) {
          Debug.LogError("Line \"" + name + "\" uses points2 rather than points3");
          return null;
        }
        return m_points3;
      }
    }

    int m_pointsCount;

    int pointsCount {
      get {
        return m_is2D ? m_points2.Count : m_points3.Count;
      }
    }

    bool m_is2D;
    Vector3[] m_screenPoints;
    float[] m_lineWidths;
    float m_lineWidth;

    public float lineWidth {
      get { return m_lineWidth; }
      set {
        m_lineWidth = value;
        float thisWidth = value * .5f;
        for (int i = 0; i < m_lineWidths.Length; i++) {
          m_lineWidths[i] = thisWidth;
        }
        m_maxWeldDistance = (value * 2) * (value * 2);
      }
    }

    float m_maxWeldDistance;

    public float maxWeldDistance {
      get { return Mathf.Sqrt(m_maxWeldDistance); }
      set { m_maxWeldDistance = value * value; }
    }

    float[] m_distances;
    string m_name;

    public string name {
      get { return m_name; }
      set {
        m_name = value;
        if (m_vectorObject != null) {
          m_name += "(" + m_vectorObject.GetInstanceID() + ")";
          m_vectorObject.name = value;
        }
        if (m_capRenderer != null) {
          m_capRenderer.gameObject.name = value + " cap";
        }
        if (m_fillRenderer != null) {
          m_fillRenderer.gameObject.name = value + " fill";
        }
      }
    }

    Material m_material;

    public Material material {
      get { return m_material; }
      set {
        m_material = value;
        if (m_vectorObject != null) {
          m_canvasRenderer.SetMaterial(m_material, null);
        }
        if (m_fillObjectSet) {
          m_fillRenderer.SetMaterial(m_material, null);
        }
      }
    }

    int m_fillVertexCount;
    bool m_active = true;

    public bool active {
      get { return m_active; }
      set {
        m_active = value;
        if (m_canvasRenderer != null) {
          m_canvasRenderer.SetVertices(m_UIVertices, m_active ? GetVertexCount() : 0);
        }
        if (m_capRenderer != null) {
          m_capRenderer.SetVertices(m_capVertices, m_active ? 8 : 0);
        }
        if (m_fillRenderer != null) {
          m_fillRenderer.SetVertices(m_fillVertices, m_active ? m_fillVertexCount : 0);
        }
      }
    }

    float m_capLength;

    public float capLength {
      get { return m_capLength; }
      set {
        if (m_isPoints) {
          Debug.LogError("VectorPoints can't use capLength");
          return;
        }
        m_capLength = value;
      }
    }

    bool m_smoothWidth = false;

    public bool smoothWidth {
      get { return m_smoothWidth; }
      set {
        m_smoothWidth = m_isPoints ? false : value;
      }
    }

    bool m_smoothColor = false;

    public bool smoothColor {
      get { return m_smoothColor; }
      set {
        m_smoothColor = m_isPoints ? false : value;
      }
    }

    bool m_continuous;

    public bool continuous {
      get { return m_continuous; }
    }

    bool m_fillObjectSet = false;
    Joins m_joins;

    public Joins joins {
      get { return m_joins; }
      set {
        if (m_isPoints || (!m_continuous && value == Joins.Fill))
          return;
        m_joins = value;
        if (m_joins == Joins.Fill && !m_fillObjectSet) {
          SetupFillObject();
          return;
        }
        if (m_joins == Joins.Fill && m_fillVertices.Length < m_UIVertices.Length) {
          System.Array.Resize(ref m_fillVertices, m_UIVertices.Length);
        }
        if (m_joins != Joins.Fill && m_fillObjectSet) {
          m_fillRenderer.SetVertices(m_fillVertices, 0);
        }
        if (m_joins == Joins.Fill && m_fillObjectSet) {
          m_fillRenderer.SetVertices(m_fillVertices, m_fillVertexCount);
        }
      }
    }

    bool m_isPoints;
    bool m_isAutoDrawing = false;

    public bool isAutoDrawing {
      get { return m_isAutoDrawing; }  
    }

    int m_drawStart = 0;

    public int drawStart {
      get { return m_drawStart; }
      set {
        if (!m_continuous && (value & 1) != 0) {  // No odd numbers for discrete lines
          value++;
        }
        m_drawStart = Mathf.Clamp(value, 0, pointsCount - 1);
      }
    }

    int m_drawEnd = 0;

    public int drawEnd {
      get { return m_drawEnd; }
      set {
        if (!m_continuous && value != 0 && (value & 1) == 0) {  // No even numbers for discrete lines (except 0)
          value++;
        }
        m_drawEnd = Mathf.Clamp(value, 0, pointsCount - 1);
      }
    }

    int m_endPointsUpdate;

    public int endPointsUpdate {
      get {
        if (m_continuous) {
          return m_endPointsUpdate;
        }
        // Actually works with odd numbers for discrete lines (except 0), but makes more intuitive sense to work with even numbers, so we fake it
        return (m_endPointsUpdate == 0) ? 0 : m_endPointsUpdate + 1;
      }
      set {
        if (!m_continuous) {
          if (value > 1 && (value & 1) == 0) { // No even numbers for discrete lines
            value--;
          }
        }
        m_endPointsUpdate = Mathf.Max(0, value);
      }
    }

    bool m_useNormals = false;
    bool m_useTangents = false;
    bool m_normalsCalculated = false;
    bool m_tangentsCalculated = false;
    int m_vertexCount;
    EndCap m_capType = EndCap.None;
    string m_endCap;

    public string endCap {
      get { return m_endCap; }
      set {
        if (m_isPoints) {
          Debug.LogError("VectorPoints can't use end caps");
          return;
        }
        if (value == null || value == "") {
          m_endCap = null;
          m_capType = EndCap.None;
          RemoveEndCap();
          return;        
        }
        if (capDictionary == null || !capDictionary.ContainsKey(value)) {
          Debug.LogError("End cap \"" + value + "\" is not set up");
          return;
        }
        m_endCap = value;
        m_capType = capDictionary[value].capType;
        if (m_capType != EndCap.None) {
          SetupEndCap();
        }
      }
    }

    bool m_continuousTexture = false;

    public bool continuousTexture {
      get { return m_continuousTexture; }
      set {
        m_continuousTexture = value;
        if (value == false) {
          ResetTextureScale();
        }
      }
    }

    Transform m_drawTransform;

    public Transform drawTransform {
      get { return m_drawTransform; }
      set { m_drawTransform = value; }
    }

    bool m_viewportDraw;

    public bool useViewportCoords {
      get { return m_viewportDraw; }
      set {
        if (m_is2D) {
          m_viewportDraw = value;
        }
        else {
          Debug.LogWarning("Line must be 2D in order to use viewport coords");
        }
      }    
    }

    float m_textureScale;
    bool m_useTextureScale = false;

    public float textureScale {
      get { return m_textureScale; }
      set {
        m_textureScale = value;
        if (m_textureScale == 0.0f) {
          m_useTextureScale = false;
          ResetTextureScale();
        }
        else {
          m_useTextureScale = true;
        }
      }
    }

    float m_textureOffset;

    public float textureOffset {
      get { return m_textureOffset; }
      set {
        m_textureOffset = value;
        SetTextureScale(true);
      }  
    }

    bool m_useMatrix = false;
    Matrix4x4 m_matrix;

    public Matrix4x4 matrix {
      get { return m_matrix; }
      set {
        m_matrix = value;
        m_useMatrix = (m_matrix != Matrix4x4.identity);
      }
    }

    public int drawDepth {
      get { return m_vectorObject.transform.GetSiblingIndex(); }
      set { m_vectorObject.transform.SetSiblingIndex(value); }
    }

    bool m_collider = false;

    public bool collider {
      get { return m_collider; }
      set {
        m_collider = value;
        AddColliderIfNeeded();
        m_vectorObject.GetComponent<Collider2D>().enabled = value;
      }
    }

    bool m_trigger = false;

    public bool trigger {
      get { return m_trigger; }
      set {
        m_trigger = value;
        if (m_vectorObject.GetComponent<Collider2D>() != null) {
          m_vectorObject.GetComponent<Collider2D>().isTrigger = value;
        }
      }
    }

    PhysicsMaterial2D m_physicsMaterial;

    public PhysicsMaterial2D physicsMaterial {
      get { return m_physicsMaterial; }
      set {
        AddColliderIfNeeded();
        m_physicsMaterial = value;
        m_vectorObject.GetComponent<Collider2D>().sharedMaterial = value;
      }
    }

    Mesh m_mesh;
    int m_canvasID = 0;

    public int canvasID {
      get { return m_canvasID; }
      set {
        if (value < 0) {
          Debug.LogError("CanvasID must be >= 0");
          return;
        }
        if (m_on2DCanvas) {
          SetCanvas(value);
          m_vectorObject.transform.SetParent(m_canvases[value].transform, false);
        }
        else {
          SetCanvas3D(value);
          m_vectorObject.transform.SetParent(m_canvases3D[value].transform, false);
        }
        m_canvasID = value;
      }
    }
  
    // Static VectorLine variables
    static Vector3 v3zero = Vector3.zero;
    // Faster than using Vector3.zero since that returns a new instance
    static List<Canvas> m_canvases;
    static List<Canvas> m_canvases3D;

    public static List<Canvas> canvases {
      get {
        if (m_canvases == null || m_canvases[0] == null) {
          SetCanvas(0);
        }
        return m_canvases;
      }
    }

    public static List<Canvas> canvases3D {
      get {
        if (m_canvases3D == null || m_canvases3D[0] == null) {
          SetCanvas3D(0);
        }
        return m_canvases3D;
      }
    }

    public static Canvas canvas {
      get {
        if (m_canvases == null || m_canvases[0] == null) {
          SetCanvas(0);
        }
        return m_canvases[0];
      }
    }

    public static Canvas canvas3D {
      get {
        if (m_canvases3D == null || m_canvases3D[0] == null) {
          SetCanvas3D(0);
        }
        return m_canvases3D[0];
      }
    }

    static Material defaultMaterial;
    static Transform camTransform;
    static Camera cam3D;
    static Vector3 oldPosition;
    static Vector3 oldRotation;

    public static Vector3 camTransformPosition {
      get { return camTransform.position; }
    }

    public static bool camTransformExists {
      get { return camTransform != null; }
    }

    static bool lineManagerCreated = false;
    static LineManager _lineManager;

    public static LineManager lineManager {
      get {
        // This prevents OnDestroy functions that reference VectorManager from creating LineManager again when editor play mode is stopped
        // Checking _lineManager == null can randomly fail, since the order of objects being Destroyed is undefined
        if (!lineManagerCreated) {
          lineManagerCreated = true;
          var lineManagerGO = new GameObject("LineManager");
          _lineManager = lineManagerGO.AddComponent<LineManager>();
          _lineManager.enabled = false;
          MonoBehaviour.DontDestroyOnLoad(_lineManager);
        }
        return _lineManager;
      }
    }

    static Dictionary<string, CapInfo> capDictionary;

    void AddColliderIfNeeded() {
      if (m_vectorObject.GetComponent<Collider2D>() == null) {
        m_vectorObject.AddComponent(m_continuous ? typeof(EdgeCollider2D) : typeof(PolygonCollider2D));
        m_vectorObject.GetComponent<Collider2D>().isTrigger = m_trigger;
      }
    }
  
    // Vector3 constructors
    public VectorLine(string lineName, Vector3[] linePoints, Material lineMaterial, float width) {
      m_points3 = new List<Vector3>(linePoints);
      SetupLine(lineName, lineMaterial, width, LineType.Discrete, Joins.None, false, false, m_points3.Count);
    }

    public VectorLine(string lineName, List<Vector3> linePoints, Material lineMaterial, float width) {
      m_points3 = linePoints;
      SetupLine(lineName, lineMaterial, width, LineType.Discrete, Joins.None, false, false, m_points3.Count);
    }

    public VectorLine(string lineName, Vector3[] linePoints, Material lineMaterial, float width, LineType lineType) {
      m_points3 = new List<Vector3>(linePoints);
      SetupLine(lineName, lineMaterial, width, lineType, Joins.None, false, false, m_points3.Count);
    }

    public VectorLine(string lineName, List<Vector3> linePoints, Material lineMaterial, float width, LineType lineType) {
      m_points3 = linePoints;
      SetupLine(lineName, lineMaterial, width, lineType, Joins.None, false, false, m_points3.Count);
    }

    public VectorLine(string lineName, Vector3[] linePoints, Material lineMaterial, float width, LineType lineType, Joins joins) {
      m_points3 = new List<Vector3>(linePoints);
      SetupLine(lineName, lineMaterial, width, lineType, joins, false, false, m_points3.Count);
    }

    public VectorLine(string lineName, List<Vector3> linePoints, Material lineMaterial, float width, LineType lineType, Joins joins) {
      m_points3 = linePoints;
      SetupLine(lineName, lineMaterial, width, lineType, joins, false, false, m_points3.Count);
    }

    // Vector2 constructors
    public VectorLine(string lineName, Vector2[] linePoints, Material lineMaterial, float width) {
      m_points2 = new List<Vector2>(linePoints);
      SetupLine(lineName, lineMaterial, width, LineType.Discrete, Joins.None, true, false, m_points2.Count);
    }

    public VectorLine(string lineName, List<Vector2> linePoints, Material lineMaterial, float width) {
      m_points2 = linePoints;
      SetupLine(lineName, lineMaterial, width, LineType.Discrete, Joins.None, true, false, m_points2.Count);
    }

    public VectorLine(string lineName, Vector2[] linePoints, Material lineMaterial, float width, LineType lineType) {
      m_points2 = new List<Vector2>(linePoints);
      SetupLine(lineName, lineMaterial, width, lineType, Joins.None, true, false, m_points2.Count);
    }

    public VectorLine(string lineName, List<Vector2> linePoints, Material lineMaterial, float width, LineType lineType) {
      m_points2 = linePoints;
      SetupLine(lineName, lineMaterial, width, lineType, Joins.None, true, false, m_points2.Count);
    }

    public VectorLine(string lineName, Vector2[] linePoints, Material lineMaterial, float width, LineType lineType, Joins joins) {
      m_points2 = new List<Vector2>(linePoints);
      SetupLine(lineName, lineMaterial, width, lineType, joins, true, false, m_points2.Count);
    }

    public VectorLine(string lineName, List<Vector2> linePoints, Material lineMaterial, float width, LineType lineType, Joins joins) {
      m_points2 = linePoints;
      SetupLine(lineName, lineMaterial, width, lineType, joins, true, false, m_points2.Count);
    }

    // Points constructors
    protected VectorLine(bool usePoints, string lineName, Vector3[] linePoints, Material lineMaterial, float width) {
      m_points3 = new List<Vector3>(linePoints);
      SetupLine(lineName, lineMaterial, width, LineType.Continuous, Joins.None, false, true, m_points3.Count);
    }

    protected VectorLine(bool usePoints, string lineName, List<Vector3> linePoints, Material lineMaterial, float width) {
      m_points3 = linePoints;
      SetupLine(lineName, lineMaterial, width, LineType.Continuous, Joins.None, false, true, m_points3.Count);
    }

    protected VectorLine(bool usePoints, string lineName, Vector2[] linePoints, Material lineMaterial, float width) {
      m_points2 = new List<Vector2>(linePoints);
      SetupLine(lineName, lineMaterial, width, LineType.Continuous, Joins.None, true, true, m_points2.Count);
    }

    protected VectorLine(bool usePoints, string lineName, List<Vector2> linePoints, Material lineMaterial, float width) {
      m_points2 = linePoints;
      SetupLine(lineName, lineMaterial, width, LineType.Continuous, Joins.None, true, true, m_points2.Count);
    }

    protected void SetupLine(string lineName, Material useMaterial, float width, LineType lineType, Joins joins, bool use2D, bool usePoints, int count) {  
      m_continuous = (lineType == LineType.Continuous);
      m_is2D = use2D;
      m_isPoints = usePoints;
      if (joins == Joins.Fill && !m_continuous) {
        Debug.LogError("VectorLine: Must use LineType.Continuous if using Joins.Fill for \"" + lineName + "\"");
        return;
      }
      if ((m_is2D && m_points2 == null) || (!m_is2D && m_points3 == null)) {
        Debug.LogError("VectorLine: the points array is null for \"" + lineName + "\"");
        return;
      }
      m_pointsCount = count;
      name = lineName;
      if (!CheckPointCount(count))
        return;
    
      m_maxWeldDistance = (width * 2) * (width * 2);
      m_joins = joins;
    
      if (useMaterial == null) {
        if (defaultMaterial == null) {
          defaultMaterial = new Material(Shader.Find("UI/Default"));
        }
        m_material = defaultMaterial;
      }
      else {
        m_material = useMaterial;
      }
    
      if (m_canvases == null || m_canvases[0] == null) {
        SetCanvas(0);
      }
      m_vectorObject = new GameObject(name);
      m_vectorObject.transform.SetParent(m_canvases[0].transform, false);
      m_on2DCanvas = true;


      m_canvasRenderer = m_vectorObject.AddComponent<CanvasRenderer>();
      m_canvasRenderer.SetMaterial(m_material, null);
      m_rectTransform = m_vectorObject.AddComponent<RectTransform>();
      SetupTransform(m_rectTransform);
    
      if (!SetVertexCount())
        return;
    
      m_UIVertices = new UIVertex[m_vertexCount];
      SetUVs(0, MaxSegmentIndex());
      color = Color.white;
      m_lineWidths = new float[1];
      m_lineWidths[0] = width * .5f;
      m_lineWidth = width;
      if (!m_is2D) {
        m_screenPoints = new Vector3[m_vertexCount];
      }
      m_drawStart = 0;
      m_drawEnd = m_pointsCount - 1;
    
      if (joins == Joins.Fill) {
        SetupFillObject();
      }

      name = lineName;
    }

    private void SetupFillObject() {    
      m_fillVertices = new UIVertex[m_vertexCount];
    
      if (m_fillRenderer == null) {
        var fillObject = new GameObject(name + " fill");
        m_fillRenderer = fillObject.AddComponent<CanvasRenderer>();
        m_fillRenderer.SetMaterial(m_material, null);
        var rectTransform = fillObject.AddComponent<RectTransform>();
        SetupTransform(rectTransform);
        fillObject.transform.SetParent(m_vectorObject.transform, false);
      }
      m_fillObjectSet = true;
    }

    private void SetupEndCap() {    
      if (m_UIVertices == null)
        return;
      m_capVertices = new UIVertex[8];
      Color32 endColor = color;
    
      if (m_UIVertices.Length > 0) {
        endColor = m_UIVertices[m_vertexCount - 1].color;
      }
      for (int i = 0; i < 4; i++) {
        m_capVertices[i].color = color;
        m_capVertices[i + 4].color = endColor;
      }
    
      m_capVertices[0].uv0 = new Vector2(0.0f, .25f);
      m_capVertices[3].uv0 = new Vector2(0.0f, 0.0f);
      m_capVertices[2].uv0 = new Vector2(1.0f, 0.0f);
      m_capVertices[1].uv0 = new Vector2(1.0f, .25f);
      if (capDictionary[m_endCap].capType == EndCap.Mirror) {
        m_capVertices[4].uv0 = new Vector2(1.0f, .25f);
        m_capVertices[7].uv0 = new Vector2(1.0f, 0.0f);
        m_capVertices[6].uv0 = new Vector2(0.0f, 0.0f);
        m_capVertices[5].uv0 = new Vector2(0.0f, .25f);
      }
      else {
        m_capVertices[4].uv0 = new Vector2(0.0f, 1.0f);
        m_capVertices[7].uv0 = new Vector2(0.0f, .75f);
        m_capVertices[6].uv0 = new Vector2(1.0f, .75f);
        m_capVertices[5].uv0 = new Vector2(1.0f, 1.0f);
      }
    
      if (m_capRenderer == null) {
        var capObject = new GameObject(name + " cap");
        m_capRenderer = capObject.AddComponent<CanvasRenderer>();
        m_capRenderer.SetMaterial(capDictionary[m_endCap].material, null);
        var rectTransform = capObject.AddComponent<RectTransform>();
        SetupTransform(rectTransform);
        capObject.transform.SetParent(m_vectorObject.transform, false);
      }
    }

    private bool CheckPointCount(int count) {
      if (!m_continuous && count % 2 != 0) {
        Debug.LogError("VectorLine: Must have an even points array count for \"" + name + "\" when using LineType.Discrete");
        return false;
      }
      return true;
    }

    private int GetVertexCount() {
      int count = m_vertexCount - adjustEnd * 4;
      if (count < 0) {
        count = 0;
      }
      return count;
    }

    private static void SetupTransform(RectTransform rectTransform) {
      rectTransform.offsetMin = Vector2.zero;
      rectTransform.offsetMax = Vector2.zero;
      rectTransform.anchorMin = Vector2.zero;
      rectTransform.anchorMax = Vector2.zero;
      rectTransform.pivot = Vector2.zero;
      rectTransform.anchoredPosition = Vector2.zero;
    }

    public void Resize(int newCount) {
      if (newCount < 0) {
        Debug.LogError("VectorLine.Resize: the new count must be >= 0");
        return;
      }
      if (!CheckPointCount(newCount))
        return;
    
      if (m_is2D) {
        if (newCount > m_pointsCount) {
          for (int i = 0; i < newCount - m_pointsCount; i++) {
            m_points2.Add(Vector2.zero);
          }
        }
        else {
          m_points2.RemoveRange(newCount, m_pointsCount - newCount);
        }
      }
      else {
        if (newCount > m_pointsCount) {
          for (int i = 0; i < newCount - m_pointsCount; i++) {
            m_points3.Add(v3zero);
          }
        }
        else {
          m_points3.RemoveRange(newCount, m_pointsCount - newCount);
        }
      }
      Resize();
    }

    private void Resize() {
      int originalCount = m_pointsCount;
      if (!m_isPoints) {
        originalCount = m_continuous ? Mathf.Max(0, m_pointsCount - 1) : m_pointsCount / 2;
      }
      bool adjustDrawEnd = (m_drawEnd == m_pointsCount - 1 || m_drawEnd < 1);
      if (!SetVertexCount())
        return;
    
      m_pointsCount = pointsCount;
      int baseArrayLength = m_UIVertices.Length;
      if (baseArrayLength < m_vertexCount) {
        if (baseArrayLength == 0) {
          baseArrayLength = 4;
        }
        while (baseArrayLength < m_pointsCount) {
          baseArrayLength *= 2;
        }
        baseArrayLength = Mathf.Min(baseArrayLength, MaxPoints());
        System.Array.Resize(ref m_UIVertices, baseArrayLength * 4);
        if (m_joins == Joins.Fill) {
          System.Array.Resize(ref m_fillVertices, baseArrayLength * 4);
        }
        if (!m_is2D) {
          System.Array.Resize(ref m_screenPoints, baseArrayLength * 4);
        }
      }
    
      if (m_lineWidths.Length > 1) {
        if (!m_isPoints) {
          baseArrayLength = m_continuous ? baseArrayLength - 1 : baseArrayLength / 2;
        }
        if (baseArrayLength > m_lineWidths.Length) {
          System.Array.Resize(ref m_lineWidths, baseArrayLength);
        }
      }
    
      if (adjustDrawEnd) {
        m_drawEnd = m_pointsCount - 1;
      }
      m_drawStart = Mathf.Clamp(m_drawStart, 0, m_pointsCount - 1);
      m_drawEnd = Mathf.Clamp(m_drawEnd, 0, m_pointsCount - 1);
      if (m_pointsCount > originalCount) {
        SetColor(m_color, originalCount, MaxSegmentIndex());
        SetUVs(originalCount, MaxSegmentIndex());
        if (m_lineWidths.Length > 1) {
          SetWidth(m_lineWidth, originalCount, MaxSegmentIndex());
        }
      }
    }

    private void SetUVs(int startIndex, int endIndex) {
      int idx = startIndex * 4;
      for (int i = startIndex; i < endIndex; i++) {
        m_UIVertices[idx].uv0 = new Vector2(0.0f, 1.0f);
        m_UIVertices[idx + 3].uv0 = new Vector2(0.0f, 0.0f);
        m_UIVertices[idx + 2].uv0 = new Vector2(1.0f, 0.0f);
        m_UIVertices[idx + 1].uv0 = new Vector2(1.0f, 1.0f);
        idx += 4;
      }
    }

    private bool SetVertexCount() {
      m_vertexCount = Mathf.Max(0, MaxSegmentIndex() * 4);
      if (m_vertexCount > 65534) {
        Debug.LogError("VectorLine: exceeded maximum vertex count of 65534 for \"" + name + "\"...use fewer points (maximum is 16383 points for continuous lines and points, and 32767 points for discrete lines)");
        return false;
      }
      return true;
    }

    private int MaxSegmentIndex() {
      if (m_isPoints) {
        return pointsCount;
      }
      return m_continuous ? pointsCount - 1 : pointsCount / 2;
    }

    private int MaxPoints() {
      if (m_isPoints || m_continuous) {
        return 32767;
      }
      return 16383;
    }

    public void AddNormals() {
      m_useNormals = true;
      m_normalsCalculated = false;
    }

    public void AddTangents() {
      m_useTangents = true;
      m_tangentsCalculated = false;
    }

    private void CalculateNormals() {
      if (m_mesh == null) {
        m_mesh = new Mesh();
      }
      var verts = new Vector3[m_vertexCount];
      for (int i = 0; i < m_vertexCount; i++) {
        verts[i] = m_UIVertices[i].position;
      }
      m_mesh.vertices = verts;
      m_mesh.triangles = GetTriangles();
      m_mesh.RecalculateNormals();
      var normals = m_mesh.normals;
      for (int i = 0; i < m_vertexCount; i++) {
        m_UIVertices[i].normal = normals[i];
      }
    }

    private void CalculateTangents() {
      if (!m_useNormals) {
        AddNormals();
        CalculateNormals();
        m_normalsCalculated = true;
      }
      var tan1 = new Vector3[m_vertexCount];
      var tan2 = new Vector3[m_vertexCount];
      int[] triangles = GetTriangles();
      int triCount = triangles.Length;
    
      for (int i = 0; i < triCount; i += 3) {
        int i1 = triangles[i];
        int i2 = triangles[i + 1];
        int i3 = triangles[i + 2];
      
        Vector3 v1 = m_UIVertices[i1].position;
        Vector3 v2 = m_UIVertices[i2].position;
        Vector3 v3 = m_UIVertices[i3].position;
      
        Vector2 w1 = m_UIVertices[i1].uv0;
        Vector2 w2 = m_UIVertices[i2].uv0;
        Vector2 w3 = m_UIVertices[i3].uv0;
      
        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;
      
        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;
      
        float r = 1.0f / (s1 * t2 - s2 * t1);
        Vector3 sdir = new Vector3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
        Vector3 tdir = new Vector3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
      
        tan1[i1] += sdir;
        tan1[i2] += sdir;
        tan1[i3] += sdir;
      
        tan2[i1] += tdir;
        tan2[i2] += tdir;
        tan2[i3] += tdir;
      }
    
      for (int i = 0; i < m_vertexCount; i++) {
        Vector3 n = m_UIVertices[i].normal;
        Vector3 t = tan1[i];
        m_UIVertices[i].tangent = (t - n * Vector3.Dot(n, t)).normalized;
        m_UIVertices[i].tangent.w = (Vector3.Dot(Vector3.Cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
      }
    }

    private int[] GetTriangles() {
      var triangles = new int[(int)(m_vertexCount + m_vertexCount / 2)];
      int idx = 0;
      for (int i = 0; i < triangles.Length; i += 6) {
        triangles[i] = idx;
        triangles[i + 1] = idx + 1;
        triangles[i + 2] = idx + 3;
        triangles[i + 3] = idx + 2;
        triangles[i + 4] = idx + 3;
        triangles[i + 5] = idx + 1;
        idx += 4;      
      }
      return triangles;
    }

    private void RemoveEndCap() {
      if (m_capRenderer != null) {
        MonoBehaviour.Destroy(m_capRenderer.gameObject);
      }
    }

    static void SetCanvas(int id) {
      if (m_canvases == null) {
        m_canvases = new List<Canvas>();
      }
      // See if existing canvases are null, which can happen after loading a new level
      for (int i = 0; i < m_canvases.Count; i++) {
        if (m_canvases[i] == null) {
          m_canvases = new List<Canvas>();
          break;
        }
      }
      while (m_canvases.Count < id + 1) {
        var go = new GameObject((m_canvases.Count == 0) ? "VectorCanvas" : "VectorCanvas_" + (m_canvases.Count));
        go.layer = LayerMask.NameToLayer("UI");
        var pos = go.transform.position;
        go.transform.position = pos;
        var canvas = go.AddComponent<Canvas>();
        canvas.renderMode = RenderMode.ScreenSpaceOverlay;
        canvas.sortingOrder = 1;
        m_canvases.Add(canvas);
      }
    }

    static void SetCanvas3D(int id) {
      if (!cam3D) {
        SetCamera3D();
        if (!cam3D) {
          Debug.LogError("No camera available...use VectorLine.SetCamera3D to assign a camera");
          return;
        }
      }
    
      if (m_canvases3D == null) {
        m_canvases3D = new List<Canvas>();
      }
      // See if existing canvases are null, which can happen after loading a new level
      for (int i = 0; i < m_canvases3D.Count; i++) {
        if (m_canvases3D[i] == null) {
          m_canvases3D = new List<Canvas>();
          break;
        }
      }
    
      while (m_canvases3D.Count < id + 1) {
        var go = new GameObject((m_canvases3D.Count == 0) ? "VectorCanvas3D" : "VectorCanvas3D_" + (m_canvases.Count));
        go.layer = LayerMask.NameToLayer("UI");
        var canvas = go.AddComponent<Canvas>();
        canvas.renderMode = RenderMode.WorldSpace;
        canvas.worldCamera = cam3D;
        var rt = go.GetComponent<RectTransform>();
        SetupTransform(rt);
        m_canvases3D.Add(canvas);
      }
    }

    public static void SetCanvasCamera(Camera cam) {
      SetCanvasCamera(cam, 0);
    }

    public static void SetCanvasCamera(Camera cam, int id) {
      if (id < 0) {
        Debug.LogError("VectorLine.SetCanvasCamera: id must be >= 0");
        return;
      }
      if (m_canvases == null || m_canvases.Count < id + 1 || m_canvases[id] == null) {
        SetCanvas(id);
      }
      m_canvases[id].renderMode = RenderMode.ScreenSpaceCamera;
      m_canvases[id].worldCamera = cam;
    }

    public static void SetCamera3D() {
      if (Camera.main == null) {
        Debug.LogError("VectorLine.SetCamera3D: no camera tagged \"Main Camera\" found. Please call SetCamera3D with a specific camera instead.");
        return;
      }
      SetCamera3D(Camera.main);
    }

    public static void SetCamera3D(Camera thisCamera) {
      camTransform = thisCamera.transform;
      cam3D = thisCamera;
      oldPosition = camTransform.position + Vector3.one;
      oldRotation = camTransform.eulerAngles + Vector3.one;
      if (m_canvases3D != null) {
        for (int i = 0; i < m_canvases3D.Count; i++) {
          if (m_canvases3D[i] == null) {
            break;
          }
          m_canvases3D[i].worldCamera = cam3D;
        }
      }
    }

    public static bool CameraHasMoved() {
      return oldPosition != camTransform.position || oldRotation != camTransform.eulerAngles;
    }

    public static void UpdateCameraInfo() {
      oldPosition = camTransform.position;
      oldRotation = camTransform.eulerAngles;  
    }

    public int GetSegmentNumber() {
      if (m_isPoints) {
        return pointsCount;  
      }
      if (m_continuous) {
        return pointsCount - 1;
      }
      return pointsCount / 2;
    }

    static string[] functionNames = {
      "VectorLine.SetColors: Length of color",
      "VectorLine.SetWidths: Length of line widths",
      "MakeCurve",
      "MakeSpline",
      "MakeEllipse"
    };

    enum FunctionName {
      SetColors,
      SetWidths,
      MakeCurve,
      MakeSpline,
      MakeEllipse

    }

    bool WrongArrayLength(int arrayLength, FunctionName functionName) {
      if (m_continuous) {
        if (arrayLength != m_pointsCount - 1) {
          Debug.LogError(functionNames[(int)functionName] + " array for \"" + name + "\" must be length of points array minus one for a continuous line (one entry per line segment)");
          return true;
        }
      }
      else if (arrayLength != m_pointsCount / 2) {
        Debug.LogError(functionNames[(int)functionName] + " array in \"" + name + "\" must be exactly half the length of points array for a discrete line (one entry per line segment)");
        return true;
      }
      return false;
    }

    bool CheckArrayLength(FunctionName functionName, int segments, int index) {
      if (segments < 1) {
        Debug.LogError("VectorLine." + functionNames[(int)functionName] + " needs at least 1 segment");
        return false;
      }

      if (m_isPoints) {
        if (index + segments > m_pointsCount) {
          if (index == 0) {
            Debug.LogError("VectorLine." + functionNames[(int)functionName] + ": The number of segments cannot exceed the number of points in the array for \"" + name + "\"");
            return false;
          }
          Debug.LogError("VectorLine: Calling " + functionNames[(int)functionName] + " with an index of " + index + " would exceed the length of the Vector array for \"" + name + "\"");
          return false;
        }
        return true;
      }

      if (m_continuous) {
        if (index + (segments + 1) > m_pointsCount) {
          if (index == 0) {
            Debug.LogError("VectorLine." + functionNames[(int)functionName] + ": The length of the array for continuous lines needs to be at least the number of segments plus one for \"" + name + "\"");
            return false;
          }
          Debug.LogError("VectorLine: Calling " + functionNames[(int)functionName] + " with an index of " + index + " would exceed the length of the Vector array for \"" + name + "\"");
          return false;
        }
      }
      else {
        if (index + segments * 2 > m_pointsCount) {
          if (index == 0) {
            Debug.LogError("VectorLine." + functionNames[(int)functionName] + ": The length of the array for discrete lines needs to be at least twice the number of segments for \"" + name + "\"");
            return false;
          }
          Debug.LogError("VectorLine: Calling " + functionNames[(int)functionName] + " with an index of " + index + " would exceed the length of the Vector array for \"" + name + "\"");
          return false;
        }
      }
      return true;
    }

    private void SetEndCapColors() {
      if (m_UIVertices.Length < 4)
        return;
    
      if (m_capType <= EndCap.Mirror) {
        int vIndex = m_continuous ? m_drawStart * 4 : m_drawStart * 2;
        for (int i = 0; i < 4; i++) {
          m_capVertices[i].color = m_UIVertices[vIndex].color;
        }
      }
      if (m_capType >= EndCap.Both) {
        int end = m_drawEnd;
        if (m_continuous) {
          if (m_drawEnd == pointsCount)
            end--;
        }
        else {
          if (end < pointsCount)
            end++;
        }
        int vIndex = end * (m_continuous ? 4 : 2) - 8;
        if (vIndex < -4) {
          vIndex = -4;
        }
        for (int i = 4; i < 8; i++) {
          m_capVertices[i].color = m_UIVertices[5 + vIndex].color;
        }
      }
      m_capRenderer.SetVertices(m_capVertices, m_active ? 8 : 0);
    }

    public void SetColor(Color32 color) {
      SetColor(color, 0, pointsCount);
    }

    public void SetColor(Color32 color, int index) {
      SetColor(color, index, index);
    }

    public void SetColor(Color32 color, int startIndex, int endIndex) {
      int max = MaxSegmentIndex();
      startIndex = Mathf.Clamp(startIndex * 4 + (smoothColor ? 2 : 0), 0, max * 4);
      endIndex = Mathf.Clamp((endIndex + 1) * 4 + (smoothColor ? 2 : 0), 0, max * 4);
    
      if (pointsCount != m_pointsCount) {
        Resize();
      }
    
      for (int i = startIndex; i < endIndex; i++) {
        m_UIVertices[i].color = color;
      }
    
      if (m_capType != EndCap.None && (startIndex <= 0 || endIndex >= max - 1)) {
        SetEndCapColors();
      }
      m_canvasRenderer.SetVertices(m_UIVertices, m_active ? GetVertexCount() : 0);
      if (m_joins == Joins.Fill) {
        SetFillColors();
      }
    }

    public void SetColors(List<Color32> lineColors) {
      SetColors(lineColors.ToArray());
    }

    public void SetColors(Color32[] lineColors) {
      if (lineColors == null) {
        Debug.LogError("VectorLine.SetColors: lineColors array must not be null");
        return;
      }
      if (!m_isPoints) {
        if (WrongArrayLength(lineColors.Length, FunctionName.SetColors)) {
          return;
        }
      }
      else if (lineColors.Length != pointsCount) {
        Debug.LogError("VectorLine.SetColors: Length of lineColors array in \"" + name + "\" must be same length as points array");
        return;
      }
    
      int start, end;
      SetSegmentStartEnd(out start, out end);
      if (start == 0 && end == 0)
        return;
    
      int idx = start * 4;
      if (m_isPoints) {
        end++;
      }
    
      if (smoothColor) {
        m_UIVertices[idx].color = lineColors[start];
        m_UIVertices[idx + 3].color = lineColors[start];
        m_UIVertices[idx + 2].color = lineColors[start];
        m_UIVertices[idx + 1].color = lineColors[start];
        idx += 4;
        for (int i = start + 1; i < end; i++) {
          m_UIVertices[idx].color = lineColors[i - 1];
          m_UIVertices[idx + 3].color = lineColors[i - 1];
          m_UIVertices[idx + 2].color = lineColors[i];
          m_UIVertices[idx + 1].color = lineColors[i];
          idx += 4;
        }
      }
      else {  // Not smooth Color
        for (int i = start; i < end; i++) {
          m_UIVertices[idx].color = lineColors[i];
          m_UIVertices[idx + 1].color = lineColors[i];
          m_UIVertices[idx + 2].color = lineColors[i];
          m_UIVertices[idx + 3].color = lineColors[i];
          idx += 4;
        }
      }

      if (m_capType != EndCap.None) {
        SetEndCapColors();
      }
      m_canvasRenderer.SetVertices(m_UIVertices, m_active ? GetVertexCount() : 0);
      if (m_joins == Joins.Fill) {
        SetFillColors();
      }
    }

    private void SetSegmentStartEnd(out int start, out int end) {
      start = (m_continuous) ? m_drawStart : m_drawStart / 2;
      end = m_drawEnd;
      if (!m_continuous) {
        end = m_drawEnd / 2;
        if (m_drawEnd % 2 != 0) {
          end++;
        }
      }
    }

    private void SetFillColors() {
      if (m_UIVertices.Length < 8)
        return;
    
      int start, end = 0;
      SetupDrawStartEnd(out start, out end, false);
      start = Mathf.Max(0, --start);
      if (start == 0 && end == 0)
        return;
    
      bool connectFirstAndLast = false;
      if (start != end && ((m_is2D && Approximately(m_points2[start], m_points2[end])) || (!m_is2D && Approximately(m_points3[start], m_points3[end])))) {
        connectFirstAndLast = true;
      }
      start *= 4;
      end *= 4;
      int idx = 0;
    
      for (int i = start; i < end - 4; i += 4) {
        m_fillVertices[idx].color = m_UIVertices[i + 3].color;
        m_fillVertices[idx + 1].color = m_UIVertices[i + 5].color;
        m_fillVertices[idx + 2].color = m_UIVertices[i + 2].color;
        m_fillVertices[idx + 3].color = m_UIVertices[i + 4].color;
        idx += 4;
      }
      if (connectFirstAndLast) {
        m_fillVertices[idx].color = m_UIVertices[end - 1].color;
        m_fillVertices[idx + 1].color = m_UIVertices[start + 1].color;
        m_fillVertices[idx + 2].color = m_UIVertices[end - 2].color;
        m_fillVertices[idx + 3].color = m_UIVertices[start].color;
      }
    
      m_fillVertexCount = m_vertexCount - adjustEnd * 4 - (connectFirstAndLast ? 0 : 4);
      m_fillRenderer.SetVertices(m_fillVertices, m_active ? m_fillVertexCount : 0);
    }

    public Color GetColor(int index) {
      index = index * 4 + 2;
      if (index < 0 || index >= m_vertexCount) {
        Debug.LogError("VectorLine.GetColor: index out of range");
        return Color.clear;
      }    
      return m_UIVertices[index].color;    
    }

    public void SetWidth(float width) {
      m_lineWidth = width;
      SetWidth(width, 0, pointsCount);
    }

    public void SetWidth(float width, int index) {
      SetWidth(width, index, index);
    }

    public void SetWidth(float width, int startIndex, int endIndex) {
      int max = MaxSegmentIndex();
      if (max >= 2 && m_lineWidths.Length == 1) {
        System.Array.Resize(ref m_lineWidths, max);
        for (int i = 0; i < max; i++) {
          m_lineWidths[i] = m_lineWidth * .5f;
        }
      }
      startIndex = Mathf.Clamp(startIndex, 0, Mathf.Max(max - 1, 0));
      endIndex = Mathf.Clamp(endIndex, 0, Mathf.Max(max - 1, 0));
      for (int i = startIndex; i <= endIndex; i++) {
        m_lineWidths[i] = width * .5f;
      }
    }

    private void SetWidthArray(int size) {
    
    }

    public void SetWidths(List<float> lineWidths) {
      SetWidths(lineWidths.ToArray(), null, lineWidths.Count, true);
    }

    public void SetWidths(List<int> lineWidths) {
      SetWidths(null, lineWidths.ToArray(), lineWidths.Count, false);
    }

    public void SetWidths(float[] lineWidths) {
      SetWidths(lineWidths, null, lineWidths.Length, true);
    }

    public void SetWidths(int[] lineWidths) {
      SetWidths(null, lineWidths, lineWidths.Length, false);
    }

    private void SetWidths(float[] lineWidthsFloat, int[] lineWidthsInt, int arrayLength, bool doFloat) {
      if ((doFloat && lineWidthsFloat == null) || (!doFloat && lineWidthsInt == null)) {
        Debug.LogError("VectorLine.SetWidths: line widths array must not be null");
        return;
      }
      if (m_isPoints) {
        if (arrayLength != pointsCount) {
          Debug.LogError("VectorLine.SetWidths: line widths array must be the same length as the points array for \"" + name + "\"");
          return;
        }
      }
      else if (WrongArrayLength(arrayLength, FunctionName.SetWidths)) {
        return;
      }
    
      if (m_lineWidths.Length != arrayLength) {
        System.Array.Resize(ref m_lineWidths, arrayLength);
      }
    
      if (doFloat) {
        for (int i = 0; i < arrayLength; i++) {
          m_lineWidths[i] = lineWidthsFloat[i] * .5f;
        }
      }
      else {
        for (int i = 0; i < arrayLength; i++) {
          m_lineWidths[i] = (float)lineWidthsInt[i] * .5f;
        }
      }
    }

    public float GetWidth(int index) {
      int max = MaxSegmentIndex();
      if (index < 0 || index >= max) {
        Debug.LogError("VectorLine.GetWidth: index out of range...must be >= 0 and < " + max);
        return 0;
      }
      return m_lineWidths[index] * 2;
    }

    public static VectorLine SetLine(Color color, params Vector2[] points) {
      return SetLine(color, 0.0f, points);
    }

    public static VectorLine SetLine(Color color, float time, params Vector2[] points) {
      if (points.Length < 2) {
        Debug.LogError("VectorLine.SetLine needs at least two points");
        return null;
      }
      var line = new VectorLine("Line", points, null, 1.0f, LineType.Continuous, Joins.None);
      line.color = color;
      if (time > 0.0f) {
        lineManager.DisableLine(line, time);
      }
      line.Draw();
      return line;
    }

    public static VectorLine SetLine(Color color, params Vector3[] points) {
      return SetLine(color, 0.0f, points);
    }

    public static VectorLine SetLine(Color color, float time, params Vector3[] points) {
      if (points.Length < 2) {
        Debug.LogError("VectorLine.SetLine needs at least two points");
        return null;
      }
      var line = new VectorLine("SetLine", points, null, 1.0f, LineType.Continuous, Joins.None);
      line.color = color;
      if (time > 0.0f) {
        lineManager.DisableLine(line, time);
      }
      line.Draw();
      return line;
    }

    public static VectorLine SetLine3D(Color color, params Vector3[] points) {
      return SetLine3D(color, 0.0f, points);
    }

    public static VectorLine SetLine3D(Color color, float time, params Vector3[] points) {
      if (points.Length < 2) {
        Debug.LogError("VectorLine.SetLine3D needs at least two points");
        return null;
      }
      var line = new VectorLine("SetLine3D", points, null, 1.0f, LineType.Continuous, Joins.None);
      line.color = color;
      line.Draw3DAuto(time);
      return line;
    }

    public static VectorLine SetRay(Color color, Vector3 origin, Vector3 direction) {
      return SetRay(color, 0.0f, origin, direction);
    }

    public static VectorLine SetRay(Color color, float time, Vector3 origin, Vector3 direction) {
      var line = new VectorLine("SetRay", new Vector3[] {
        origin,
        new Ray(origin, direction).GetPoint(direction.magnitude)
      }, null, 1.0f, LineType.Continuous, Joins.None);
      line.color = color;
      if (time > 0.0f) {
        lineManager.DisableLine(line, time);
      }
      line.Draw();
      return line;
    }

    public static VectorLine SetRay3D(Color color, Vector3 origin, Vector3 direction) {
      return SetRay3D(color, 0.0f, origin, direction);
    }

    public static VectorLine SetRay3D(Color color, float time, Vector3 origin, Vector3 direction) {
      var line = new VectorLine("SetRay3D", new Vector3[] {
        origin,
        new Ray(origin, direction).GetPoint(direction.magnitude)
      }, null, 1.0f, LineType.Continuous, Joins.None);
      line.color = color;
      line.Draw3DAuto(time);
      return line;
    }

    private bool CheckLine(bool draw3D) {
      if (m_joins == Joins.Fill) {
        DrawFill();
      }
      if (m_capType != EndCap.None) {
        DrawEndCap(draw3D);
      }
      if (m_continuousTexture) {
        SetContinuousTexture();
      }
      return true;
    }

    private void DrawFill() {
      int start, end = 0;
      SetupDrawStartEnd(out start, out end, false);
      bool connectFirstAndLast = false;
      if (start != end && ((m_is2D && Approximately(m_points2[start], m_points2[end])) || (!m_is2D && Approximately(m_points3[start], m_points3[end])))) {
        connectFirstAndLast = true;
      }
      start = Mathf.Max(0, --start);
      start *= 4;
      end *= 4;
      int i;
      for (i = start; i < end - 4; i += 4) {
        if (m_UIVertices[i + 4].position.x == m_UIVertices[i + 7].position.x && m_UIVertices[i + 4].position.y == m_fillVertices[i + 7].position.y) {
          m_fillVertices[i].position = v3zero;
          m_fillVertices[i + 3].position = v3zero;
          m_fillVertices[i + 2].position = v3zero;
          m_fillVertices[i + 1].position = v3zero;
          i += 4;
          continue;
        }
        m_fillVertices[i].position = m_UIVertices[i + 1].position;
        m_fillVertices[i + 3].position = m_UIVertices[i + 7].position;
        m_fillVertices[i + 2].position = m_UIVertices[i + 2].position;
        m_fillVertices[i + 1].position = m_UIVertices[i + 4].position;
        m_fillVertices[i].color = m_UIVertices[i + 1].color;
        m_fillVertices[i + 3].color = m_UIVertices[i + 7].color;
        m_fillVertices[i + 2].color = m_UIVertices[i + 2].color;
        m_fillVertices[i + 1].color = m_UIVertices[i + 4].color;
      }
      if (connectFirstAndLast && end > start && i < m_fillVertices.Length) {
        if (m_UIVertices[start].position.x == m_UIVertices[start + 3].position.x && m_UIVertices[start].position.y == m_UIVertices[start + 3].position.y) {
          m_fillVertices[i].position = v3zero;
          m_fillVertices[i + 3].position = v3zero;
          m_fillVertices[i + 2].position = v3zero;
          m_fillVertices[i + 1].position = v3zero;
        }
        else {
          m_fillVertices[i].position = m_UIVertices[end - 3].position;
          m_fillVertices[i + 3].position = m_UIVertices[start + 3].position;
          m_fillVertices[i + 2].position = m_UIVertices[end - 2].position;
          m_fillVertices[i + 1].position = m_UIVertices[start].position;
          m_fillVertices[i].color = m_UIVertices[end - 3].color;
          m_fillVertices[i + 3].color = m_UIVertices[start + 3].color;
          m_fillVertices[i + 2].color = m_UIVertices[end - 2].color;
          m_fillVertices[i + 1].color = m_UIVertices[start].color;
        }
      }
  
      if (m_useNormals) {
        for (i = start; i < end - 4; i += 4) {
          m_fillVertices[i].normal = m_UIVertices[i + 1].normal;
          m_fillVertices[i + 3].normal = m_UIVertices[i + 7].normal;
          m_fillVertices[i + 2].normal = m_UIVertices[i + 2].normal;
          m_fillVertices[i + 1].normal = m_UIVertices[i + 4].normal;
        }
        if (connectFirstAndLast) {
          m_fillVertices[i].normal = m_UIVertices[end - 3].normal;
          m_fillVertices[i + 3].normal = m_UIVertices[start + 3].normal;
          m_fillVertices[i + 2].normal = m_UIVertices[end - 2].normal;
          m_fillVertices[i + 1].normal = m_UIVertices[start].normal;
        }
      }
      if (m_useTangents) {
        for (i = start; i < end - 4; i += 4) {
          m_fillVertices[i].tangent = m_UIVertices[i + 1].tangent;
          m_fillVertices[i + 3].tangent = m_UIVertices[i + 7].tangent;
          m_fillVertices[i + 2].tangent = m_UIVertices[i + 2].tangent;
          m_fillVertices[i + 1].tangent = m_UIVertices[i + 4].tangent;
        }
        if (connectFirstAndLast) {
          m_fillVertices[i].tangent = m_UIVertices[end - 3].tangent;
          m_fillVertices[i + 3].tangent = m_UIVertices[start + 3].tangent;
          m_fillVertices[i + 2].tangent = m_UIVertices[end - 2].tangent;
          m_fillVertices[i + 1].tangent = m_UIVertices[start].tangent;
        }
      }
    
      m_fillVertexCount = m_vertexCount - adjustEnd * 4 - (connectFirstAndLast ? 0 : 4);
      m_fillRenderer.SetVertices(m_fillVertices, m_active ? m_fillVertexCount : 0);
    }

    private void DrawEndCap(bool draw3D) {
      if (m_capType <= EndCap.Mirror) {
        int vIndex = m_drawStart * 4;
        int widthIndex = (m_lineWidths.Length > 1) ? m_drawStart : 0;
        if (!m_continuous) {
          widthIndex /= 2;
          vIndex /= 2;
        }
        if (!draw3D) {
          var d = (m_UIVertices[vIndex].position - m_UIVertices[vIndex + 1].position).normalized *
                  m_lineWidths[widthIndex] * 2.0f * capDictionary[m_endCap].ratio1;
          var d2 = d * capDictionary[m_endCap].offset;
          m_capVertices[0].position = m_UIVertices[vIndex].position + d + d2;
          m_capVertices[3].position = m_UIVertices[vIndex + 3].position + d + d2;
        
          m_UIVertices[vIndex].position += d2;
          m_UIVertices[vIndex + 3].position += d2;
        }
        else {
          var d = (m_screenPoints[vIndex] - m_screenPoints[vIndex + 1]).normalized *
                  m_lineWidths[widthIndex] * 2.0f * capDictionary[m_endCap].ratio1;
          var d2 = d * capDictionary[m_endCap].offset;
          m_capVertices[0].position = cam3D.ScreenToWorldPoint(m_screenPoints[vIndex] + d + d2);
          m_capVertices[3].position = cam3D.ScreenToWorldPoint(m_screenPoints[vIndex + 3] + d + d2);
        
          m_UIVertices[vIndex].position = cam3D.ScreenToWorldPoint(m_screenPoints[vIndex] + d2);
          m_UIVertices[vIndex + 3].position = cam3D.ScreenToWorldPoint(m_screenPoints[vIndex + 3] + d2);
        }
        m_capVertices[2].position = m_UIVertices[vIndex + 3].position;
        m_capVertices[1].position = m_UIVertices[vIndex].position;
      }
      if (m_capType >= EndCap.Both) {
        int end = m_drawEnd;
        if (m_continuous) {
          if (m_drawEnd == m_pointsCount)
            end--;
        }
        else {
          if (end < m_pointsCount)
            end++;
        }
        int vIndex = end * 4;
        int widthIndex = (m_lineWidths.Length > 1) ? end - 1 : 0;
        if (widthIndex < 0) {
          widthIndex = 0;
        }
        if (!m_continuous) {
          widthIndex /= 2;
          vIndex /= 2;
        }
        if (vIndex < 4) {
          vIndex = 4;
        }
        if (!draw3D) {
          var d = (m_UIVertices[vIndex - 2].position - m_UIVertices[vIndex - 1].position).normalized *
                  m_lineWidths[widthIndex] * 2.0f * capDictionary[m_endCap].ratio2;
          var d2 = d * capDictionary[m_endCap].offset;
          m_capVertices[6].position = m_UIVertices[vIndex - 2].position + d + d2;
          m_capVertices[5].position = m_UIVertices[vIndex - 3].position + d + d2;
        
          m_UIVertices[vIndex - 3].position += d2;
          m_UIVertices[vIndex - 2].position += d2;
        }
        else {
          var d = (m_screenPoints[vIndex - 2] - m_screenPoints[vIndex - 1]).normalized *
                  m_lineWidths[widthIndex] * 2.0f * capDictionary[m_endCap].ratio2;
          var d2 = d * capDictionary[m_endCap].offset;
          m_capVertices[6].position = cam3D.ScreenToWorldPoint(m_screenPoints[vIndex - 2] + d + d2);
          m_capVertices[5].position = cam3D.ScreenToWorldPoint(m_screenPoints[vIndex - 3] + d + d2);
        
          m_UIVertices[vIndex - 3].position = cam3D.ScreenToWorldPoint(m_screenPoints[vIndex - 3] + d2);
          m_UIVertices[vIndex - 2].position = cam3D.ScreenToWorldPoint(m_screenPoints[vIndex - 2] + d2);
        }
        m_capVertices[4].position = m_UIVertices[vIndex - 3].position;
        m_capVertices[7].position = m_UIVertices[vIndex - 2].position;
      }
    
      if (m_drawStart > 0 || m_drawEnd < m_pointsCount) {
        SetEndCapColors();
      }
    }

    private void SetContinuousTexture() {
      int idx = 0;
      float offset = 0.0f;
      SetDistances();
      int end = m_distances.Length - 1;
      float totalDistance = m_distances[end];
    
      for (int i = 0; i < end; i++) {
        m_UIVertices[idx].uv0.x = offset;
        m_UIVertices[idx + 1].uv0.x = offset;
        offset = 1.0f / (totalDistance / m_distances[i + 1]);
        m_UIVertices[idx + 2].uv0.x = offset;
        m_UIVertices[idx + 3].uv0.x = offset;
        idx += 4;
      }
    }

    private void CheckNormals() {
      if (m_useNormals && !m_normalsCalculated) {
        CalculateNormals();
        m_normalsCalculated = true;
      }
      if (m_useTangents && !m_tangentsCalculated) {
        CalculateTangents();
        m_tangentsCalculated = true;
      }
    }

    private bool UseMatrix(out Matrix4x4 thisMatrix) {
      if (m_drawTransform != null) {
        thisMatrix = m_drawTransform.localToWorldMatrix;
        return true;
      }
      else if (m_useMatrix) {
        thisMatrix = m_matrix;
        return true;
      }
      thisMatrix = Matrix4x4.identity;
      return false;
    }

    private bool CheckPointCount() {
      if (pointsCount < (m_isPoints ? 1 : 2)) {
        m_canvasRenderer.SetVertices(m_UIVertices, 0);
        if (m_capType != EndCap.None) {
          m_capRenderer.SetVertices(m_capVertices, 0);
        }
        if (m_joins == Joins.Fill) {
          m_fillRenderer.SetVertices(m_fillVertices, 0);
        }
        m_pointsCount = pointsCount;
        return false;
      }
      return true;
    }

    private void SetupDrawStartEnd(out int start, out int end, bool clearVertices) {
      adjustEnd = 0;
      start = 0;
      end = m_pointsCount - 1;
      if (m_drawStart > 0) {
        start = m_drawStart;
        if (clearVertices) {
          ZeroVertices(0, start);
        }
      }
      if (m_drawEnd < m_pointsCount - 1) {
        end = m_drawEnd;
        if (end < 0) {
          end = 0;
        }
        if (!m_continuous && !m_isPoints) {
          adjustEnd += (m_pointsCount - end) / 2;
        }
        else {
          adjustEnd += ((m_pointsCount - 1) - end);
        }
      }
      if (m_endPointsUpdate > 0) {
        start = Mathf.Max(0, end - m_endPointsUpdate);
      }
    }

    private void ZeroVertices(int startIndex, int endIndex) {
      if (m_continuous) {
        startIndex *= 4;
        endIndex *= 4;
        if (endIndex > m_vertexCount) {
          endIndex -= 4;
        }
        for (int i = startIndex; i < endIndex; i += 4) {
          m_UIVertices[i].position = v3zero;
          m_UIVertices[i + 1].position = v3zero;
          m_UIVertices[i + 2].position = v3zero;
          m_UIVertices[i + 3].position = v3zero;
        }
        if (m_joins == Joins.Fill && m_fillVertices != null) {
          for (int i = startIndex; i < endIndex; i += 4) {
            m_fillVertices[i].position = v3zero;
            m_fillVertices[i + 1].position = v3zero;
            m_fillVertices[i + 2].position = v3zero;
            m_fillVertices[i + 3].position = v3zero;
          }
        }
      }
      else {
        startIndex *= 2;
        endIndex *= 2;
        for (int i = startIndex; i < endIndex; i += 2) {
          m_UIVertices[i].position = v3zero;
          m_UIVertices[i + 1].position = v3zero;
        }
        if (m_joins == Joins.Fill) {
          for (int i = startIndex; i < endIndex; i += 2) {
            m_fillVertices[i].position = v3zero;
            m_fillVertices[i + 1].position = v3zero;
          }
        }
      }
    }

    public void Draw() {
      if (!m_on2DCanvas) {
        m_vectorObject.transform.SetParent(m_canvases[m_canvasID].transform, false);
        m_on2DCanvas = true;
      }
      if (!CheckPointCount() || m_lineWidths == null)
        return;
      if (pointsCount != m_pointsCount) {
        Resize();
      }
      if (m_isPoints) {
        DrawPoints();
        return;
      }
      if (smoothWidth && m_lineWidths.Length == 1 && pointsCount > 2) {
        Debug.LogError("VectorLine.Draw called with smooth line widths for \"" + name + "\", but VectorLine.SetWidths has not been used");
        return;
      }
    
      Matrix4x4 thisMatrix;
      bool useMatrix = UseMatrix(out thisMatrix);
      int start = 0, end = 0;
      SetupDrawStartEnd(out start, out end, true);
      if (m_is2D) {
        Line2D(start, end, thisMatrix, useMatrix);
      }
      else {
        Line3D(start, end, thisMatrix, useMatrix);
      }
    
      CheckNormals();

      if (!CheckLine(false))
        return;

      if (m_useTextureScale) {
        SetTextureScale(false);
      }

      if (m_canvasRenderer == null) {
        Debug.Log("frame(" + Time.frameCount + ") VectorLine.Draw m_name(" + m_name + ") m_canvasRenderer.SetVertices (m_UIVertices, m_active? GetVertexCount() : 0);");
      }

      m_canvasRenderer.SetVertices(m_UIVertices, m_active ? GetVertexCount() : 0);
      if (m_collider) {
        SetCollider(true);
      }
    }

    private void Line2D(int start, int end, Matrix4x4 thisMatrix, bool useTransformMatrix) {
      Vector3 p1 = v3zero, p2 = v3zero, v1 = v3zero, px = v3zero;
      Vector2 scaleFactor = new Vector2(Screen.width, Screen.height);
    
      int add = 0, idx = 0, widthIdx = 0;
      int widthIdxAdd = 0;
      if (m_lineWidths.Length > 1) {
        widthIdx = start;
        widthIdxAdd = 1;
      }
      if (m_continuous) {
        add = 1;
        idx = start * 4;
      }
      else {
        add = 2;
        widthIdx /= 2;
        idx = start * 2;
      }
    
      for (int i = start; i < end; i += add) {
        if (useTransformMatrix) {
          p1 = thisMatrix.MultiplyPoint3x4(m_points2[i]);
          p2 = thisMatrix.MultiplyPoint3x4(m_points2[i + 1]);
        }
        else {
          p1.x = m_points2[i].x;
          p1.y = m_points2[i].y;
          p2.x = m_points2[i + 1].x;
          p2.y = m_points2[i + 1].y;
        }
        if (m_viewportDraw) {
          p1.x *= scaleFactor.x;
          p1.y *= scaleFactor.y;
          p2.x *= scaleFactor.x;
          p2.y *= scaleFactor.y;
        }
        if (p1.x == p2.x && p1.y == p2.y) {
          SkipQuad(ref idx, ref widthIdx, ref widthIdxAdd);
          continue;
        }
      
        if (m_capLength == 0.0f) {
          px.x = p2.y - p1.y;
          px.y = p1.x - p2.x;
          float normalizedDistance = (1.0f / (float)System.Math.Sqrt((px.x * px.x) + (px.y * px.y)));
          px *= normalizedDistance * m_lineWidths[widthIdx];
          m_UIVertices[idx].position.x = p1.x - px.x;
          m_UIVertices[idx].position.y = p1.y - px.y; 
          m_UIVertices[idx + 3].position.x = p1.x + px.x;
          m_UIVertices[idx + 3].position.y = p1.y + px.y;
          if (smoothWidth && i < end - add) {
            px.x = p2.y - p1.y;
            px.y = p1.x - p2.x;
            px *= normalizedDistance * m_lineWidths[widthIdx + 1];
          }
        }
        else {
          px.x = p2.x - p1.x;
          px.y = p2.y - p1.y;
          px *= (1.0f / (float)System.Math.Sqrt((px.x * px.x) + (px.y * px.y)));
          p1 -= px * m_capLength;
          p2 += px * m_capLength;
        
          v1.x = px.y;
          v1.y = -px.x;
          px = v1 * m_lineWidths[widthIdx];
          m_UIVertices[idx].position.x = p1.x - px.x;
          m_UIVertices[idx].position.y = p1.y - px.y;
          m_UIVertices[idx + 3].position.x = p1.x + px.x;
          m_UIVertices[idx + 3].position.y = p1.y + px.y;
          if (smoothWidth && i < end - add) {
            px = v1 * m_lineWidths[widthIdx + 1];
          }
        }
        m_UIVertices[idx + 2].position.x = p2.x + px.x;
        m_UIVertices[idx + 2].position.y = p2.y + px.y;
        m_UIVertices[idx + 1].position.x = p2.x - px.x;
        m_UIVertices[idx + 1].position.y = p2.y - px.y;
      
        idx += 4;
        widthIdx += widthIdxAdd;
      }
      if (m_joins == Joins.Weld) {
        if (m_continuous) {
          WeldJoins(start * 4 + (start == 0 ? 4 : 0), end * 4, Approximately(m_points2[0], m_points2[m_pointsCount - 1]));
        }
        else {
          if ((end & 1) == 0) {  // end should be odd for discrete lines
            end--;
          }
          WeldJoinsDiscrete(start + 1, end, Approximately(m_points2[0], m_points2[m_pointsCount - 1]));
        }
      }
    }

    private void Line3D(int start, int end, Matrix4x4 thisMatrix, bool useTransformMatrix) {
      if (!cam3D) {
        SetCamera3D();
        if (!cam3D) {
          Debug.LogError("No camera available...use VectorLine.SetCamera3D to assign a camera");
          return;
        }
      }
    
      Vector3 pos1 = v3zero, pos2 = v3zero, px = v3zero;
      float normalizedDistance = 0.0f;
      int widthIdx = 0, widthIdxAdd = 0;
      if (m_lineWidths.Length > 1) {
        widthIdx = start;
        widthIdxAdd = 1;
      }
      int idx = start * 2;
      int add = 2;
    
      if (m_continuous) {
        pos2 = useTransformMatrix ? cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[start])) :
                     cam3D.WorldToScreenPoint(m_points3[start]);
        idx = start * 4;
        add = 1;
      }
      float sw = Screen.width * 2;
      float sh = Screen.height * 2;
    
      for (int i = start; i < end; i += add) {
        if (m_continuous) {
          pos1.x = pos2.x;
          pos1.y = pos2.y;
          pos1.z = pos2.z;
          pos2 = useTransformMatrix ? cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i + 1])) :
                       cam3D.WorldToScreenPoint(m_points3[i + 1]);
        }
        else {
          if (useTransformMatrix) {
            pos1 = cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i]));
            pos2 = cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i + 1]));
          }
          else {
            pos1 = cam3D.WorldToScreenPoint(m_points3[i]);
            pos2 = cam3D.WorldToScreenPoint(m_points3[i + 1]);
          }
        }
        if ((pos1.x == pos2.x && pos1.y == pos2.y) || (pos1.z < 0.0f && pos2.z < 0.0f) || ((pos1.x > sw && pos1.z < 0.0f) || (pos2.x > sw && pos2.z < 0.0f)) || ((pos1.y > sh && pos1.z < 0.0f) || (pos2.y > sh && pos2.z < 0.0f))) {
          SkipQuad(ref idx, ref widthIdx, ref widthIdxAdd);
          continue;
        }
      
        px.x = pos2.y - pos1.y;
        px.y = pos1.x - pos2.x;
        normalizedDistance = 1.0f / (float)System.Math.Sqrt((px.x * px.x) + (px.y * px.y));
        px.x *= normalizedDistance * m_lineWidths[widthIdx];
        px.y *= normalizedDistance * m_lineWidths[widthIdx];
        m_UIVertices[idx].position.x = pos1.x - px.x;
        m_UIVertices[idx].position.y = pos1.y - px.y;
        m_UIVertices[idx + 3].position.x = pos1.x + px.x;
        m_UIVertices[idx + 3].position.y = pos1.y + px.y;
        if (smoothWidth && i < end - add) {
          px.x = pos2.y - pos1.y;
          px.y = pos1.x - pos2.x;
          px.x *= normalizedDistance * m_lineWidths[widthIdx + 1];
          px.y *= normalizedDistance * m_lineWidths[widthIdx + 1];
        }
        m_UIVertices[idx + 2].position.x = pos2.x + px.x;
        m_UIVertices[idx + 2].position.y = pos2.y + px.y;
        m_UIVertices[idx + 1].position.x = pos2.x - px.x;
        m_UIVertices[idx + 1].position.y = pos2.y - px.y;
        idx += 4;
        widthIdx += widthIdxAdd;
      }
    
      if (m_joins == Joins.Weld) {
        if (m_continuous) {
          WeldJoins(start * 4 + 4, end * 4, Approximately(m_points3[0], m_points3[m_pointsCount - 1]));
        }
        else {
          if ((end & 1) == 0) {  // end should be odd for discrete lines
            end--;
          }
          WeldJoinsDiscrete(start + 1, end, Approximately(m_points3[0], m_points3[m_pointsCount - 1]));
        }
      }
    }

    public void Draw3D() {
      if (m_is2D) {
        Debug.LogError("VectorLine.Draw3D can only be used with a Vector3 array, which \"" + name + "\" doesn't have");
        return;
      }
      if (!CheckPointCount() || m_lineWidths == null)
        return;
      if (m_on2DCanvas) {
        SetCanvas3D(m_canvasID);
        m_vectorObject.transform.SetParent(m_canvases3D[m_canvasID].transform, false);
        m_on2DCanvas = false;
      }
      if (pointsCount != m_pointsCount) {
        Resize();
      }
      if (m_isPoints) {
        DrawPoints3D();
        return;
      }
      if (smoothWidth && m_lineWidths.Length == 1 && m_pointsCount > 2) {
        Debug.LogError("VectorLine.Draw3D called with smooth line widths for \"" + name + "\", but VectorLine.SetWidths has not been used");
        return;
      }
    
      int start = 0, end = 0, add = 0, widthIdx = 0;
      SetupDrawStartEnd(out start, out end, true);
      Matrix4x4 thisMatrix;
      bool useMatrix = UseMatrix(out thisMatrix);
    
      int idx = 0, widthIdxAdd = 0;
      if (m_lineWidths.Length > 1) {
        widthIdx = start;
        widthIdxAdd = 1;
      }
      if (m_continuous) {
        add = 1;
        idx = start * 4;
      }
      else {
        widthIdx /= 2;
        add = 2;
        idx = start * 2;
      }
      Vector3 thisLine = v3zero, px = v3zero, pos1 = v3zero, pos2 = v3zero;
      float sw = Screen.width * 2;
      float sh = Screen.height * 2;
      for (int i = start; i < end; i += add) {
        if (useMatrix) {
          pos1 = cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i]));
          pos2 = cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i + 1]));
        }
        else {
          pos1 = cam3D.WorldToScreenPoint(m_points3[i]);
          pos2 = cam3D.WorldToScreenPoint(m_points3[i + 1]);
        }
        if ((pos1.x == pos2.x && pos1.y == pos2.y) || (pos1.z < 0.0f && pos2.z < 0.0f) || ((pos1.x > sw && pos1.z < 0.0f) || (pos2.x > sw && pos2.z < 0.0f)) || ((pos1.y > sh && pos1.z < 0.0f) || (pos2.y > sh && pos2.z < 0.0f))) {
          SkipQuad3D(ref idx, ref widthIdx, ref widthIdxAdd);
          continue;
        }
      
        px.x = pos2.y - pos1.y;
        px.y = pos1.x - pos2.x;
        thisLine = px / (float)System.Math.Sqrt(px.x * px.x + px.y * px.y);
        px.x = thisLine.x * m_lineWidths[widthIdx];
        px.y = thisLine.y * m_lineWidths[widthIdx];
      
        // Screenpoints used for Joins.Weld and end caps
        m_screenPoints[idx].x = pos1.x - px.x;
        m_screenPoints[idx].y = pos1.y - px.y;
        m_screenPoints[idx].z = pos1.z - px.z;
        m_screenPoints[idx + 3].x = pos1.x + px.x;
        m_screenPoints[idx + 3].y = pos1.y + px.y;
        m_screenPoints[idx + 3].z = pos1.z + px.z; 
        m_UIVertices[idx].position = cam3D.ScreenToWorldPoint(m_screenPoints[idx]);
        m_UIVertices[idx + 3].position = cam3D.ScreenToWorldPoint(m_screenPoints[idx + 3]);
        if (smoothWidth && i < end - add) {
          px.x = thisLine.x * m_lineWidths[widthIdx + 1];
          px.y = thisLine.y * m_lineWidths[widthIdx + 1];
        }
        m_screenPoints[idx + 2].x = pos2.x + px.x;
        m_screenPoints[idx + 2].y = pos2.y + px.y;
        m_screenPoints[idx + 2].z = pos2.z + px.z;
        m_screenPoints[idx + 1].x = pos2.x - px.x;
        m_screenPoints[idx + 1].y = pos2.y - px.y;
        m_screenPoints[idx + 1].z = pos2.z - px.z;
        m_UIVertices[idx + 2].position = cam3D.ScreenToWorldPoint(m_screenPoints[idx + 2]);
        m_UIVertices[idx + 1].position = cam3D.ScreenToWorldPoint(m_screenPoints[idx + 1]);
      
        idx += 4;
        widthIdx += widthIdxAdd;
      }
    
      if (m_joins == Joins.Weld) {
        if (m_continuous) {
          WeldJoins3D(start * 4 + 4, end * 4, Approximately(m_points3[0], m_points3[m_pointsCount - 1]));
        }
        else {
          if ((end & 1) == 0) {  // end should be odd for discrete lines
            end--;
          }
          WeldJoinsDiscrete3D(start + 1, end, Approximately(m_points3[0], m_points3[m_pointsCount - 1]));
        }
      }
    
      CheckNormals();
      if (!CheckLine(true))
        return;
        
      if (m_useTextureScale) {
        SetTextureScale(false);
      }

      if (m_canvasRenderer == null) {
        Debug.Log("frame(" + Time.frameCount + ") VectorLine.Draw3D m_name(" + m_name + ") m_canvasRenderer.SetVertices (m_UIVertices, m_active? GetVertexCount() : 0);");
      }

      m_canvasRenderer.SetVertices(m_UIVertices, m_active ? GetVertexCount() : 0);
      if (m_collider) {
        SetCollider(false);
      }
    }

    private void DrawPoints() {
      if (!m_is2D && !cam3D) {
        SetCamera3D();
        if (!cam3D) {
          Debug.LogError("No camera available...use VectorLine.SetCamera3D to assign a camera");
          return;
        }
      }
    
      Matrix4x4 thisMatrix;
      bool useMatrix = UseMatrix(out thisMatrix);
    
      int start, end;
      SetupDrawStartEnd(out start, out end, true);
      Vector2 scaleFactor = new Vector2(Screen.width, Screen.height);
    
      Vector3 p1;
      int idx = start * 4;
      int widthIdxAdd = (m_lineWidths.Length > 1) ? 1 : 0;
      int widthIdx = start;
      var v1 = new Vector3(m_lineWidths[0], m_lineWidths[0], 0.0f);
      var v2 = new Vector3(-m_lineWidths[0], m_lineWidths[0], 0.0f);
    
      if (!m_is2D) {
        for (int i = start; i <= end; i++) {
          p1 = useMatrix ? cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i])) :
                cam3D.WorldToScreenPoint(m_points3[i]);
          if (p1.z < 0) {
            SkipQuad(ref idx, ref widthIdx, ref widthIdxAdd);
            continue;
          }
        
          if (widthIdxAdd != 0) {
            v1.x = v1.y = v2.y = m_lineWidths[widthIdx];
            v2.x = -m_lineWidths[widthIdx];
            widthIdx++;
          }
        
          m_UIVertices[idx].position.x = p1.x + v2.x;
          m_UIVertices[idx].position.y = p1.y + v2.y;
          m_UIVertices[idx + 3].position.x = p1.x - v1.x;
          m_UIVertices[idx + 3].position.y = p1.y - v1.y;
          m_UIVertices[idx + 1].position.x = p1.x + v1.x;
          m_UIVertices[idx + 1].position.y = p1.y + v1.y;
          m_UIVertices[idx + 2].position.x = p1.x - v2.x;
          m_UIVertices[idx + 2].position.y = p1.y - v2.y;
          idx += 4;
        }
      }
      else {
        for (int i = start; i <= end; i++) {
          p1 = useMatrix ? thisMatrix.MultiplyPoint3x4(m_points2[i]) : (Vector3)m_points2[i];
          if (m_viewportDraw) {
            p1.x *= scaleFactor.x;
            p1.y *= scaleFactor.y;
          }
        
          if (widthIdxAdd != 0) {
            v1.x = v1.y = v2.y = m_lineWidths[widthIdx];
            v2.x = -m_lineWidths[widthIdx];
            widthIdx++;
          }
        
          m_UIVertices[idx].position.x = p1.x + v2.x;
          m_UIVertices[idx].position.y = p1.y + v2.y;
          m_UIVertices[idx + 3].position.x = p1.x - v1.x;
          m_UIVertices[idx + 3].position.y = p1.y - v1.y;
          m_UIVertices[idx + 1].position.x = p1.x + v1.x;
          m_UIVertices[idx + 1].position.y = p1.y + v1.y;
          m_UIVertices[idx + 2].position.x = p1.x - v2.x;
          m_UIVertices[idx + 2].position.y = p1.y - v2.y;
          idx += 4;
        }
      }
    
      CheckNormals();
      m_canvasRenderer.SetVertices(m_UIVertices, m_active ? GetVertexCount() : 0);
    }

    private void DrawPoints3D() {
      Matrix4x4 thisMatrix;
      bool useMatrix = UseMatrix(out thisMatrix);
    
      int start = 0, end = 0, widthIdx = 0;
      SetupDrawStartEnd(out start, out end, true);
        
      int idx = start * 4;
      int widthIdxAdd = 0;
      if (m_lineWidths.Length > 1) {
        widthIdx = start;
        widthIdxAdd = 1;
      }
      Vector3 p1 = v3zero, v1 = v3zero, v2 = v3zero;
      for (int i = start; i <= end; i++) {
        p1 = useMatrix ? cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i])) :
              cam3D.WorldToScreenPoint(m_points3[i]);
        if (p1.z < 0) {
          SkipQuad(ref idx, ref widthIdx, ref widthIdxAdd);
          continue;
        }
        v1.x = v1.y = v2.y = m_lineWidths[widthIdx];
        v2.x = -m_lineWidths[widthIdx];
      
        m_UIVertices[idx].position = cam3D.ScreenToWorldPoint(p1 + v2);
        m_UIVertices[idx + 3].position = cam3D.ScreenToWorldPoint(p1 - v1);
        m_UIVertices[idx + 1].position = cam3D.ScreenToWorldPoint(p1 + v1);
        m_UIVertices[idx + 2].position = cam3D.ScreenToWorldPoint(p1 - v2);
        idx += 4;
        widthIdx += widthIdxAdd;
      }
    
      CheckNormals();
      m_canvasRenderer.SetVertices(m_UIVertices, m_active ? GetVertexCount() : 0);
    }

    private void SkipQuad(ref int idx, ref int widthIdx, ref int widthIdxAdd) {
      m_UIVertices[idx].position = v3zero;
      m_UIVertices[idx + 1].position = v3zero;
      m_UIVertices[idx + 2].position = v3zero;
      m_UIVertices[idx + 3].position = v3zero;
    
      idx += 4;
      widthIdx += widthIdxAdd;
    }

    private void SkipQuad3D(ref int idx, ref int widthIdx, ref int widthIdxAdd) {
      m_UIVertices[idx].position = v3zero;
      m_UIVertices[idx + 1].position = v3zero;
      m_UIVertices[idx + 2].position = v3zero;
      m_UIVertices[idx + 3].position = v3zero;
    
      m_screenPoints[idx] = v3zero;
      m_screenPoints[idx + 1] = v3zero;
      m_screenPoints[idx + 2] = v3zero;
      m_screenPoints[idx + 3] = v3zero;
    
      idx += 4;
      widthIdx += widthIdxAdd;
    }

    private void WeldJoins(int start, int end, bool connectFirstAndLast) {
      if (connectFirstAndLast) {
        SetIntersectionPoint(m_vertexCount - 4, m_vertexCount - 3, 0, 1);
        SetIntersectionPoint(m_vertexCount - 1, m_vertexCount - 2, 3, 2);
      }
      for (int i = start; i < end; i += 4) {
        SetIntersectionPoint(i - 4, i - 3, i, i + 1);
        SetIntersectionPoint(i - 1, i - 2, i + 3, i + 2);
      }
    }

    private void WeldJoinsDiscrete(int start, int end, bool connectFirstAndLast) {
      if (connectFirstAndLast) {
        SetIntersectionPoint(m_vertexCount - 4, m_vertexCount - 3, 0, 1);
        SetIntersectionPoint(m_vertexCount - 1, m_vertexCount - 2, 3, 2);
      }
      int idx = (start + 1) / 2 * 4;
      if (m_is2D) {
        for (int i = start; i < end; i += 2) {
          if (m_points2[i] == m_points2[i + 1]) {
            SetIntersectionPoint(idx - 4, idx - 3, idx, idx + 1);
            SetIntersectionPoint(idx - 1, idx - 2, idx + 3, idx + 2);
          }
          idx += 4;
        }
      }
      else {
        for (int i = start; i < end; i += 2) {
          if (m_points3[i] == m_points3[i + 1]) {
            SetIntersectionPoint(idx - 4, idx - 3, idx, idx + 1);
            SetIntersectionPoint(idx - 1, idx - 2, idx + 3, idx + 2);
          }
          idx += 4;
        }
      }
    }

    private void SetIntersectionPoint(int p1, int p2, int p3, int p4) {
      var l1a = m_UIVertices[p1].position;
      var l1b = m_UIVertices[p2].position;
      var l2a = m_UIVertices[p3].position;
      var l2b = m_UIVertices[p4].position;
      float d = (l2b.y - l2a.y) * (l1b.x - l1a.x) - (l2b.x - l2a.x) * (l1b.y - l1a.y);
      if (d == 0.0f)
        return;  // Parallel lines
      float n = ((l2b.x - l2a.x) * (l1a.y - l2a.y) - (l2b.y - l2a.y) * (l1a.x - l2a.x)) / d;
    
      var v3 = new Vector3(l1a.x + (n * (l1b.x - l1a.x)), l1a.y + (n * (l1b.y - l1a.y)), l1a.z);
      if ((v3 - l1b).sqrMagnitude > m_maxWeldDistance)
        return;
      m_UIVertices[p2].position = v3;
      m_UIVertices[p3].position = v3;
    }

    private void WeldJoins3D(int start, int end, bool connectFirstAndLast) {
      if (connectFirstAndLast) {
        SetIntersectionPoint3D(m_vertexCount - 4, m_vertexCount - 3, 0, 1);
        SetIntersectionPoint3D(m_vertexCount - 1, m_vertexCount - 2, 3, 2);
      }
      for (int i = start; i < end; i += 4) {
        SetIntersectionPoint3D(i - 4, i - 3, i, i + 1);
        SetIntersectionPoint3D(i - 1, i - 2, i + 3, i + 2);
      }
    }

    private void WeldJoinsDiscrete3D(int start, int end, bool connectFirstAndLast) {
      if (connectFirstAndLast) {
        SetIntersectionPoint3D(m_vertexCount - 4, m_vertexCount - 3, 0, 1);
        SetIntersectionPoint3D(m_vertexCount - 1, m_vertexCount - 2, 3, 2);
      }
      int idx = (start + 1) / 2 * 4;
      for (int i = start; i < end; i += 2) {
        if (m_points3[i] == m_points3[i + 1]) {
          SetIntersectionPoint3D(idx - 4, idx - 3, idx, idx + 1);
          SetIntersectionPoint3D(idx - 1, idx - 2, idx + 3, idx + 2);
        }
        idx += 4;
      }
    }

    private void SetIntersectionPoint3D(int p1, int p2, int p3, int p4) {
      var l1a = m_screenPoints[p1];
      var l1b = m_screenPoints[p2];
      var l2a = m_screenPoints[p3];
      var l2b = m_screenPoints[p4];
      float d = (l2b.y - l2a.y) * (l1b.x - l1a.x) - (l2b.x - l2a.x) * (l1b.y - l1a.y);
      if (d == 0.0f)
        return;  // Parallel lines
      float n = ((l2b.x - l2a.x) * (l1a.y - l2a.y) - (l2b.y - l2a.y) * (l1a.x - l2a.x)) / d;
    
      var v3 = new Vector3(l1a.x + (n * (l1b.x - l1a.x)), l1a.y + (n * (l1b.y - l1a.y)), l1a.z);
      if ((v3 - l1b).sqrMagnitude > m_maxWeldDistance)
        return;
      m_UIVertices[p2].position = cam3D.ScreenToWorldPoint(v3);
      m_UIVertices[p3].position = m_UIVertices[p2].position;
    }

    public static void LineManagerCheckDistance() {
      lineManager.StartCheckDistance();
    }

    public static void LineManagerDisable() {
      //Debug.Log ("LineManagerDisable");
      lineManager.DisableIfUnused();
    }

    public static void LineManagerEnable() {
      //Debug.Log ("LineManagerEnable");
      lineManager.EnableIfUsed();
    }

    public void Draw3DAuto() {
      Draw3DAuto(0.0f);
    }

    public void Draw3DAuto(float time) {
      if (time < 0.0f)
        time = 0.0f;
      lineManager.AddLine(this, m_drawTransform, time);
      m_isAutoDrawing = true;
      Draw3D();
    }

    public void StopDrawing3DAuto() {
      lineManager.RemoveLine(this);
      m_isAutoDrawing = false;
    }

    private void SetTextureScale(bool updateUIVertices) {
      if (pointsCount != m_pointsCount) {
        Resize();
      }  
      int end = m_continuous ? pointsCount - 1 : pointsCount;
      int add = m_continuous ? 1 : 2;
      int idx = 0;
      int widthIdx = 0;
      int widthIdxAdd = m_lineWidths.Length == 1 ? 0 : 1;
      float thisScale = 1.0f / m_textureScale;
      var useTransformMatrix = (m_drawTransform != null);
      var thisMatrix = useTransformMatrix ? m_drawTransform.localToWorldMatrix : Matrix4x4.identity;
      var p1 = Vector2.zero;
      var p2 = Vector2.zero;
      var px = Vector2.zero;
      float offset = m_textureOffset;
      float capAdd = m_capLength * 2;
    
      if (m_is2D) {
        for (int i = 0; i < end; i += add) {
          if (!m_viewportDraw) {
            if (useTransformMatrix) {
              p1 = thisMatrix.MultiplyPoint3x4(m_points2[i]);
              p2 = thisMatrix.MultiplyPoint3x4(m_points2[i + 1]);
            }
            else {
              p1.x = m_points2[i].x;
              p1.y = m_points2[i].y;
              p2.x = m_points2[i + 1].x;
              p2.y = m_points2[i + 1].y;
            }
          }
          else {
            if (useTransformMatrix) {
              p1 = thisMatrix.MultiplyPoint3x4(new Vector2(m_points2[i].x * Screen.width, m_points2[i].y * Screen.height));
              p2 = thisMatrix.MultiplyPoint3x4(new Vector2(m_points2[i + 1].x * Screen.width, m_points2[i + 1].y * Screen.height));
            }
            else {
              p1 = new Vector2(m_points2[i].x * Screen.width, m_points2[i].y * Screen.height);
              p2 = new Vector2(m_points2[i + 1].x * Screen.width, m_points2[i + 1].y * Screen.height);
            }
          }
          px.x = p2.x - p1.x;
          px.y = p2.y - p1.y;
          float xPos = thisScale / (m_lineWidths[widthIdx] * 2 / ((float)System.Math.Sqrt((px.x * px.x) + (px.y * px.y)) + capAdd));
          m_UIVertices[idx].uv0.x = offset;
          m_UIVertices[idx + 3].uv0.x = offset;
          m_UIVertices[idx + 2].uv0.x = xPos + offset;
          m_UIVertices[idx + 1].uv0.x = xPos + offset;
          idx += 4;
          offset = (offset + xPos) % 1;
          widthIdx += widthIdxAdd;
        }
      }
      else {
        for (int i = 0; i < end; i += add) {
          if (useTransformMatrix) {
            p1 = cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i]));
            p2 = cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i + 1]));
          }
          else {
            p1 = cam3D.WorldToScreenPoint(m_points3[i]);
            p2 = cam3D.WorldToScreenPoint(m_points3[i + 1]);
          }
          px.x = p1.x - p2.x;
          px.y = p1.y - p2.y;
          float xPos = thisScale / (m_lineWidths[widthIdx] * 2 / (float)System.Math.Sqrt(px.x * px.x + px.y * px.y));
          m_UIVertices[idx].uv0.x = offset;
          m_UIVertices[idx + 3].uv0.x = offset;
          m_UIVertices[idx + 2].uv0.x = xPos + offset;
          m_UIVertices[idx + 1].uv0.x = xPos + offset;
          idx += 4;
          offset = (offset + xPos) % 1;
          widthIdx += widthIdxAdd;
        }
      }
    
      if (updateUIVertices) {
        m_canvasRenderer.SetVertices(m_UIVertices, m_active ? GetVertexCount() : 0);
      }
    }

    private void ResetTextureScale() {    
      for (int i = 0; i < m_vertexCount; i += 4) {
        m_UIVertices[i].uv0.x = 0.0f;
        m_UIVertices[i + 3].uv0.x = 0.0f;
        m_UIVertices[i + 2].uv0.x = 1.0f;
        m_UIVertices[i + 1].uv0.x = 1.0f;
      }
    }

    private void SetCollider(bool convertToWorldSpace) {
      if (!cam3D) {
        SetCamera3D();
        if (!cam3D) {
          Debug.LogError("No camera available...use VectorLine.SetCamera3D to assign a camera");
          return;
        }
      }
      if (cam3D.transform.rotation != Quaternion.identity) {
        Debug.LogWarning("The line collider will not be correct if the camera is rotated");
      }
    
      var v3 = new Vector3(0.0f, 0.0f, -cam3D.transform.position.z);
      int min = drawStart;
      int max = drawEnd;
        
      if (m_continuous) {
        var collider = m_vectorObject.GetComponent(typeof(EdgeCollider2D)) as EdgeCollider2D;
        var path = new Vector2[(max - min) * 4 + 1];
      
        int startIdx = 0;
        int endIdx = path.Length - 2;
        if (convertToWorldSpace) {
          for (int i = min * 4; i < max * 4; i += 4) {
            v3.x = m_UIVertices[i].position.x;
            v3.y = m_UIVertices[i].position.y;
            path[startIdx] = cam3D.ScreenToWorldPoint(v3);
            v3.x = m_UIVertices[i + 1].position.x;
            v3.y = m_UIVertices[i + 1].position.y;
            path[startIdx + 1] = cam3D.ScreenToWorldPoint(v3);
            v3.x = m_UIVertices[i + 3].position.x;
            v3.y = m_UIVertices[i + 3].position.y;
            path[endIdx] = cam3D.ScreenToWorldPoint(v3);
            v3.x = m_UIVertices[i + 2].position.x;
            v3.y = m_UIVertices[i + 2].position.y;
            path[endIdx - 1] = cam3D.ScreenToWorldPoint(v3);
            startIdx += 2;
            endIdx -= 2;
          }
        }
        else {
          for (int i = min * 4; i < max * 4; i += 4) {
            path[startIdx].x = m_UIVertices[i].position.x;
            path[startIdx].y = m_UIVertices[i].position.y;
            path[startIdx + 1].x = m_UIVertices[i + 1].position.x;
            path[startIdx + 1].y = m_UIVertices[i + 1].position.y;
            path[endIdx].x = m_UIVertices[i + 3].position.x;
            path[endIdx].y = m_UIVertices[i + 3].position.y;
            path[endIdx - 1].x = m_UIVertices[i + 2].position.x;
            path[endIdx - 1].y = m_UIVertices[i + 2].position.y;
            startIdx += 2;
            endIdx -= 2;
          }
        }
        path[path.Length - 1] = path[0];
        collider.points = path;
      }
      else {  // Discrete line
        var collider = m_vectorObject.GetComponent(typeof(PolygonCollider2D)) as PolygonCollider2D;
        var path = new Vector2[4];
        collider.pathCount = ((max - min) + 1) / 2;

        int end = (max + 1) / 2 * 4;
        int pIdx = 0;
        if (convertToWorldSpace) {
          for (int i = min / 2 * 4; i < end; i += 4) {
            v3.x = m_UIVertices[i].position.x;
            v3.y = m_UIVertices[i].position.y;
            path[0] = cam3D.ScreenToWorldPoint(v3);
            v3.x = m_UIVertices[i + 3].position.x;
            v3.y = m_UIVertices[i + 3].position.y;
            path[1] = cam3D.ScreenToWorldPoint(v3);
            v3.x = m_UIVertices[i + 2].position.x;
            v3.y = m_UIVertices[i + 2].position.y;
            path[2] = cam3D.ScreenToWorldPoint(v3);
            v3.x = m_UIVertices[i + 1].position.x;
            v3.y = m_UIVertices[i + 1].position.y;
            path[3] = cam3D.ScreenToWorldPoint(v3);
            collider.SetPath(pIdx++, path);
          }
        }
        else {
          for (int i = min / 2 * 4; i < end; i += 4) {          
            path[0].x = m_UIVertices[i].position.x;
            path[0].y = m_UIVertices[i].position.y;
            path[1].x = m_UIVertices[i + 3].position.x;
            path[1].y = m_UIVertices[i + 3].position.y;
            path[2].x = m_UIVertices[i + 2].position.x;
            path[2].y = m_UIVertices[i + 2].position.y;
            path[3].x = m_UIVertices[i + 1].position.x;
            path[3].y = m_UIVertices[i + 1].position.y;
            collider.SetPath(pIdx++, path);
          }
        }
      }
    }

    static int endianDiff1;
    static int endianDiff2;
    static byte[] byteBlock;

    public static Vector3[] BytesToVector3Array(byte[] lineBytes) {
      if (lineBytes.Length % 12 != 0) {
        Debug.LogError("VectorLine.BytesToVector3Array: Incorrect input byte length...must be a multiple of 12");
        return null;
      }
    
      SetupByteBlock();
      Vector3[] points = new Vector3[lineBytes.Length / 12];
      int idx = 0;
      for (int i = 0; i < lineBytes.Length; i += 12) {
        points[idx++] = new Vector3(ConvertToFloat(lineBytes, i),
          ConvertToFloat(lineBytes, i + 4),
          ConvertToFloat(lineBytes, i + 8));
      }
      return points;
    }

    public static Vector2[] BytesToVector2Array(byte[] lineBytes) {
      if (lineBytes.Length % 8 != 0) {
        Debug.LogError("VectorLine.BytesToVector2Array: Incorrect input byte length...must be a multiple of 8");
        return null;
      }
    
      SetupByteBlock();
      Vector2[] points = new Vector2[lineBytes.Length / 8];
      int idx = 0;
      for (int i = 0; i < lineBytes.Length; i += 8) {
        points[idx++] = new Vector2(ConvertToFloat(lineBytes, i),
          ConvertToFloat(lineBytes, i + 4));
      }
      return points;
    }

    private static void SetupByteBlock() {
      if (byteBlock == null) {
        byteBlock = new byte[4];
      }
      if (System.BitConverter.IsLittleEndian) {
        endianDiff1 = 0;
        endianDiff2 = 0;
      }
      else {
        endianDiff1 = 3;
        endianDiff2 = 1;
      }  
    }
  
    // Unfortunately we can't just use System.BitConverter.ToSingle as-is...we need a function to handle both big-endian and little-endian systems
    private static float ConvertToFloat(byte[] bytes, int i) {
      byteBlock[endianDiff1] = bytes[i];
      byteBlock[1 + endianDiff2] = bytes[i + 1];
      byteBlock[2 - endianDiff2] = bytes[i + 2];
      byteBlock[3 - endianDiff1] = bytes[i + 3];
      return System.BitConverter.ToSingle(byteBlock, 0);
    }

    public static void Destroy(ref VectorLine line) {
      DestroyLine(ref line);
    }

    public static void Destroy(VectorLine[] lines) {
      for (int i = 0; i < lines.Length; i++) {
        DestroyLine(ref lines[i]);
      }
    }

    public static void Destroy(List<VectorLine> lines) {
      for (int i = 0; i < lines.Count; i++) {
        var line = lines[i];
        DestroyLine(ref line);
      }
    }

    private static void DestroyLine(ref VectorLine line) {
      //Debug.Log("frame("+Time.frameCount+") name("+line.name+") VectorLine.DestroyLine");    

      if (line != null) {
        Object.Destroy(line.m_vectorObject);
        if (line.isAutoDrawing) {
          line.StopDrawing3DAuto();
        }
        line = null;
      }
    }

    public static void Destroy(ref VectorPoints line) {
      DestroyPoints(ref line);
    }

    public static void Destroy(VectorPoints[] lines) {
      for (int i = 0; i < lines.Length; i++) {
        DestroyPoints(ref lines[i]);
      }
    }

    public static void Destroy(List<VectorPoints> lines) {
      for (int i = 0; i < lines.Count; i++) {
        var line = lines[i];
        DestroyPoints(ref line);
      }
    }

    private static void DestroyPoints(ref VectorPoints line) {
      if (line != null) {
        Object.Destroy(line.m_vectorObject);
        if (line.isAutoDrawing) {
          line.StopDrawing3DAuto();
        }
        line = null;
      }
    }

    public static void Destroy(ref VectorLine line, GameObject go) {
      Destroy(ref line);
      if (go != null) {
        Object.Destroy(go);
      }
    }

    public static void Destroy(ref VectorPoints line, GameObject go) {
      Destroy(ref line);
      if (go != null) {
        Object.Destroy(go);
      }
    }

    public void MakeRect(Rect rect) {
      MakeRect(new Vector2(rect.x, rect.y), new Vector2(rect.x + rect.width, rect.y + rect.height), 0);
    }

    public void MakeRect(Rect rect, int index) {
      MakeRect(new Vector2(rect.x, rect.y), new Vector2(rect.x + rect.width, rect.y + rect.height), index);
    }

    public void MakeRect(Vector3 topLeft, Vector3 bottomRight) {
      MakeRect(topLeft, bottomRight, 0);
    }

    public void MakeRect(Vector3 topLeft, Vector3 bottomRight, int index) {
      if (m_continuous) {
        if (index + 5 > pointsCount) {
          if (index == 0) {
            Debug.LogError("VectorLine.MakeRect: The length of the array for continuous lines needs to be at least 5 for \"" + name + "\"");
            return;
          }
          Debug.LogError("Calling VectorLine.MakeRect with an index of " + index + " would exceed the length of the Vector2 array for \"" + name + "\"");
          return;
        }
        if (m_is2D) {
          m_points2[index] = new Vector2(topLeft.x, topLeft.y);
          m_points2[index + 1] = new Vector2(bottomRight.x, topLeft.y);
          m_points2[index + 2] = new Vector2(bottomRight.x, bottomRight.y);
          m_points2[index + 3] = new Vector2(topLeft.x, bottomRight.y);
          m_points2[index + 4] = new Vector2(topLeft.x, topLeft.y);
        }
        else {
          m_points3[index] = new Vector3(topLeft.x, topLeft.y, topLeft.z);
          m_points3[index + 1] = new Vector3(bottomRight.x, topLeft.y, topLeft.z);
          m_points3[index + 2] = new Vector3(bottomRight.x, bottomRight.y, bottomRight.z);
          m_points3[index + 3] = new Vector3(topLeft.x, bottomRight.y, bottomRight.z);
          m_points3[index + 4] = new Vector3(topLeft.x, topLeft.y, topLeft.z);
        }
      }
      else {
        if (index + 8 > pointsCount) {
          if (index == 0) {
            Debug.LogError("VectorLine.MakeRect: The length of the array for discrete lines needs to be at least 8 for \"" + name + "\"");
            return;
          }
          Debug.LogError("Calling VectorLine.MakeRect with an index of " + index + " would exceed the length of the Vector2 array for \"" + name + "\"");
          return;
        }
        if (m_is2D) {
          m_points2[index] = new Vector2(topLeft.x, topLeft.y);
          m_points2[index + 1] = new Vector2(bottomRight.x, topLeft.y);
          m_points2[index + 2] = new Vector2(bottomRight.x, topLeft.y);
          m_points2[index + 3] = new Vector2(bottomRight.x, bottomRight.y);
          m_points2[index + 4] = new Vector2(bottomRight.x, bottomRight.y);
          m_points2[index + 5] = new Vector2(topLeft.x, bottomRight.y);
          m_points2[index + 6] = new Vector2(topLeft.x, bottomRight.y);
          m_points2[index + 7] = new Vector2(topLeft.x, topLeft.y);        
        }
        else {
          m_points3[index] = new Vector3(topLeft.x, topLeft.y, topLeft.z);
          m_points3[index + 1] = new Vector3(bottomRight.x, topLeft.y, topLeft.z);
          m_points3[index + 2] = new Vector3(bottomRight.x, topLeft.y, topLeft.z);
          m_points3[index + 3] = new Vector3(bottomRight.x, bottomRight.y, bottomRight.z);
          m_points3[index + 4] = new Vector3(bottomRight.x, bottomRight.y, bottomRight.z);
          m_points3[index + 5] = new Vector3(topLeft.x, bottomRight.y, bottomRight.z);
          m_points3[index + 6] = new Vector3(topLeft.x, bottomRight.y, bottomRight.z);
          m_points3[index + 7] = new Vector3(topLeft.x, topLeft.y, topLeft.z);        
        }
      }
    }

    public void MakeCircle(Vector3 origin, float radius) {
      MakeEllipse(origin, Vector3.forward, radius, radius, 0.0f, 0.0f, GetSegmentNumber(), 0.0f, 0);
    }

    public void MakeCircle(Vector3 origin, float radius, int segments) {
      MakeEllipse(origin, Vector3.forward, radius, radius, 0.0f, 0.0f, segments, 0.0f, 0);
    }

    public void MakeCircle(Vector3 origin, float radius, int segments, float pointRotation) {
      MakeEllipse(origin, Vector3.forward, radius, radius, 0.0f, 0.0f, segments, pointRotation, 0);
    }

    public void MakeCircle(Vector3 origin, float radius, int segments, int index) {
      MakeEllipse(origin, Vector3.forward, radius, radius, 0.0f, 0.0f, segments, 0.0f, index);
    }

    public void MakeCircle(Vector3 origin, float radius, int segments, float pointRotation, int index) {
      MakeEllipse(origin, Vector3.forward, radius, radius, 0.0f, 0.0f, segments, pointRotation, index);
    }

    public void MakeCircle(Vector3 origin, Vector3 upVector, float radius) {
      MakeEllipse(origin, upVector, radius, radius, 0.0f, 0.0f, GetSegmentNumber(), 0.0f, 0);
    }

    public void MakeCircle(Vector3 origin, Vector3 upVector, float radius, int segments) {
      MakeEllipse(origin, upVector, radius, radius, 0.0f, 0.0f, segments, 0.0f, 0);
    }

    public void MakeCircle(Vector3 origin, Vector3 upVector, float radius, int segments, float pointRotation) {
      MakeEllipse(origin, upVector, radius, radius, 0.0f, 0.0f, segments, pointRotation, 0);
    }

    public void MakeCircle(Vector3 origin, Vector3 upVector, float radius, int segments, int index) {
      MakeEllipse(origin, upVector, radius, radius, 0.0f, 0.0f, segments, 0.0f, index);
    }

    public void MakeCircle(Vector3 origin, Vector3 upVector, float radius, int segments, float pointRotation, int index) {
      MakeEllipse(origin, upVector, radius, radius, 0.0f, 0.0f, segments, pointRotation, index);
    }

    public void MakeEllipse(Vector3 origin, float xRadius, float yRadius) {
      MakeEllipse(origin, Vector3.forward, xRadius, yRadius, 0.0f, 0.0f, GetSegmentNumber(), 0.0f, 0);
    }

    public void MakeEllipse(Vector3 origin, float xRadius, float yRadius, int segments) {
      MakeEllipse(origin, Vector3.forward, xRadius, yRadius, 0.0f, 0.0f, segments, 0.0f, 0);
    }

    public void MakeEllipse(Vector3 origin, float xRadius, float yRadius, int segments, int index) {
      MakeEllipse(origin, Vector3.forward, xRadius, yRadius, 0.0f, 0.0f, segments, 0.0f, index);
    }

    public void MakeEllipse(Vector3 origin, float xRadius, float yRadius, int segments, float pointRotation) {
      MakeEllipse(origin, Vector3.forward, xRadius, yRadius, 0.0f, 0.0f, segments, pointRotation, 0);
    }

    public void MakeEllipse(Vector3 origin, Vector3 upVector, float xRadius, float yRadius) {
      MakeEllipse(origin, upVector, xRadius, yRadius, 0.0f, 0.0f, GetSegmentNumber(), 0.0f, 0);
    }

    public void MakeEllipse(Vector3 origin, Vector3 upVector, float xRadius, float yRadius, int segments) {
      MakeEllipse(origin, upVector, xRadius, yRadius, 0.0f, 0.0f, segments, 0.0f, 0);
    }

    public void MakeEllipse(Vector3 origin, Vector3 upVector, float xRadius, float yRadius, int segments, int index) {
      MakeEllipse(origin, upVector, xRadius, yRadius, 0.0f, 0.0f, segments, 0.0f, index);
    }

    public void MakeEllipse(Vector3 origin, Vector3 upVector, float xRadius, float yRadius, int segments, float pointRotation) {
      MakeEllipse(origin, upVector, xRadius, yRadius, 0.0f, 0.0f, segments, pointRotation, 0);
    }

    public void MakeEllipse(Vector3 origin, Vector3 upVector, float xRadius, float yRadius, int segments, float pointRotation, int index) {
      MakeEllipse(origin, upVector, xRadius, yRadius, 0.0f, 0.0f, segments, pointRotation, index);
    }

    public void MakeArc(Vector3 origin, float xRadius, float yRadius, float startDegrees, float endDegrees) {
      MakeEllipse(origin, Vector3.forward, xRadius, yRadius, startDegrees, endDegrees, GetSegmentNumber(), 0.0f, 0);
    }

    public void MakeArc(Vector3 origin, float xRadius, float yRadius, float startDegrees, float endDegrees, int segments) {
      MakeEllipse(origin, Vector3.forward, xRadius, yRadius, startDegrees, endDegrees, segments, 0.0f, 0);
    }

    public void MakeArc(Vector3 origin, float xRadius, float yRadius, float startDegrees, float endDegrees, int segments, int index) {
      MakeEllipse(origin, Vector3.forward, xRadius, yRadius, startDegrees, endDegrees, segments, 0.0f, index);
    }

    public void MakeArc(Vector3 origin, Vector3 upVector, float xRadius, float yRadius, float startDegrees, float endDegrees) {
      MakeEllipse(origin, upVector, xRadius, yRadius, startDegrees, endDegrees, GetSegmentNumber(), 0.0f, 0);
    }

    public void MakeArc(Vector3 origin, Vector3 upVector, float xRadius, float yRadius, float startDegrees, float endDegrees, int segments) {
      MakeEllipse(origin, upVector, xRadius, yRadius, startDegrees, endDegrees, segments, 0.0f, 0);
    }

    public void MakeArc(Vector3 origin, Vector3 upVector, float xRadius, float yRadius, float startDegrees, float endDegrees, int segments, int index) {
      MakeEllipse(origin, upVector, xRadius, yRadius, startDegrees, endDegrees, segments, 0.0f, index);
    }

    private void MakeEllipse(Vector3 origin, Vector3 upVector, float xRadius, float yRadius, float startDegrees, float endDegrees, int segments, float pointRotation, int index) {
      if (segments < 3) {
        Debug.LogError("VectorLine.MakeEllipse needs at least 3 segments");
        return;
      }
      if (!CheckArrayLength(FunctionName.MakeEllipse, segments, index)) {
        return;
      }
    
      float totalDegrees, p;
      startDegrees = Mathf.Repeat(startDegrees, 360.0f);
      endDegrees = Mathf.Repeat(endDegrees, 360.0f);
      if (startDegrees == endDegrees) {
        totalDegrees = 360.0f;
        p = -pointRotation * Mathf.Deg2Rad;
      }
      else {
        totalDegrees = (endDegrees > startDegrees) ? endDegrees - startDegrees : (360.0f - startDegrees) + endDegrees;
        p = startDegrees * Mathf.Deg2Rad;
      }
      float radians = (totalDegrees / segments) * Mathf.Deg2Rad;
    
      if (m_continuous) {
        if (startDegrees != endDegrees) {
          segments++;
        }
        int i = 0;
        if (m_is2D) {
          Vector2 v2Origin = origin;
          for (i = 0; i < segments; i++) {
            m_points2[index + i] = v2Origin + new Vector2(.5f + Mathf.Sin(p) * xRadius, .5f + Mathf.Cos(p) * yRadius);
            p += radians;
          }
          if (!m_isPoints && startDegrees == endDegrees) {  // Copy point when making an ellipse so the shape is closed
            m_points2[index + i] = m_points2[index + (i - segments)];
          }
        }
        else {
          var thisMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.LookRotation(-upVector, upVector), Vector3.one);
          for (i = 0; i < segments; i++) {
            m_points3[index + i] = origin + thisMatrix.MultiplyPoint3x4(new Vector3(Mathf.Sin(p) * xRadius, Mathf.Cos(p) * yRadius, 0.0f));
            p += radians;
          }
          if (!m_isPoints && startDegrees == endDegrees) {  // Copy point when making an ellipse so the shape is closed
            m_points3[index + i] = m_points3[index + (i - segments)];
          }
        }
      }
    // Discrete
    else {
        if (m_is2D) {
          Vector2 v2Origin = origin;
          for (int i = 0; i < segments * 2; i++) {
            m_points2[index + i] = v2Origin + new Vector2(.5f + Mathf.Sin(p) * xRadius, .5f + Mathf.Cos(p) * yRadius);
            p += radians;
            i++;
            m_points2[index + i] = v2Origin + new Vector2(.5f + Mathf.Sin(p) * xRadius, .5f + Mathf.Cos(p) * yRadius);
          }
        }
        else {
          var thisMatrix = Matrix4x4.TRS(Vector3.zero, Quaternion.LookRotation(-upVector, upVector), Vector3.one);
          for (int i = 0; i < segments * 2; i++) {
            m_points3[index + i] = origin + thisMatrix.MultiplyPoint3x4(new Vector3(Mathf.Sin(p) * xRadius, Mathf.Cos(p) * yRadius, 0.0f));
            p += radians;
            i++;
            m_points3[index + i] = origin + thisMatrix.MultiplyPoint3x4(new Vector3(Mathf.Sin(p) * xRadius, Mathf.Cos(p) * yRadius, 0.0f));
          }
        }
      }
    }

    public void MakeCurve(Vector2[] curvePoints) {
      MakeCurve(curvePoints, GetSegmentNumber(), 0);
    }

    public void MakeCurve(Vector2[] curvePoints, int segments) {
      MakeCurve(curvePoints, segments, 0);
    }

    public void MakeCurve(Vector2[] curvePoints, int segments, int index) {
      if (curvePoints.Length != 4) {
        Debug.LogError("VectorLine.MakeCurve needs exactly 4 points in the curve points array");
        return;
      }
      MakeCurve(curvePoints[0], curvePoints[1], curvePoints[2], curvePoints[3], segments, index);
    }

    public void MakeCurve(Vector3[] curvePoints) {
      MakeCurve(curvePoints, GetSegmentNumber(), 0);
    }

    public void MakeCurve(Vector3[] curvePoints, int segments) {
      MakeCurve(curvePoints, segments, 0);
    }

    public void MakeCurve(Vector3[] curvePoints, int segments, int index) {
      if (curvePoints.Length != 4) {
        Debug.LogError("VectorLine.MakeCurve needs exactly 4 points in the curve points array");
        return;
      }
      MakeCurve(curvePoints[0], curvePoints[1], curvePoints[2], curvePoints[3], segments, index);
    }

    public void MakeCurve(Vector3 anchor1, Vector3 control1, Vector3 anchor2, Vector3 control2) {
      MakeCurve(anchor1, control1, anchor2, control2, GetSegmentNumber(), 0);
    }

    public void MakeCurve(Vector3 anchor1, Vector3 control1, Vector3 anchor2, Vector3 control2, int segments) {
      MakeCurve(anchor1, control1, anchor2, control2, segments, 0);
    }

    public void MakeCurve(Vector3 anchor1, Vector3 control1, Vector3 anchor2, Vector3 control2, int segments, int index) {
      if (!CheckArrayLength(FunctionName.MakeCurve, segments, index)) {
        return;
      }
    
      if (m_continuous) {
        int end = m_isPoints ? segments : segments + 1;
        if (m_is2D) {
          Vector2 anchor1a = anchor1;
          Vector2 anchor2a = anchor2;
          Vector2 control1a = control1;
          Vector2 control2a = control2;
          for (int i = 0; i < end; i++) {
            m_points2[index + i] = GetBezierPoint(ref anchor1a, ref control1a, ref anchor2a, ref control2a, (float)i / segments);
          }
        }
        else {
          for (int i = 0; i < end; i++) {
            m_points3[index + i] = GetBezierPoint3D(ref anchor1, ref control1, ref anchor2, ref control2, (float)i / segments);
          }
        }
      }
      else {
        int idx = 0;
        if (m_is2D) {
          Vector2 anchor1a = anchor1;
          Vector2 anchor2a = anchor2;
          Vector2 control1a = control1;
          Vector2 control2a = control2;
          for (int i = 0; i < segments; i++) {
            m_points2[index + idx++] = GetBezierPoint(ref anchor1a, ref control1a, ref anchor2a, ref control2a, (float)i / segments);
            m_points2[index + idx++] = GetBezierPoint(ref anchor1a, ref control1a, ref anchor2a, ref control2a, (float)(i + 1) / segments);
          }
        }
        else {
          for (int i = 0; i < segments; i++) {
            m_points3[index + idx++] = GetBezierPoint3D(ref anchor1, ref control1, ref anchor2, ref control2, (float)i / segments);
            m_points3[index + idx++] = GetBezierPoint3D(ref anchor1, ref control1, ref anchor2, ref control2, (float)(i + 1) / segments);
          }
        }
      }
    }

    private static Vector2 GetBezierPoint(ref Vector2 anchor1, ref Vector2 control1, ref Vector2 anchor2, ref Vector2 control2, float t) {
      float cx = 3 * (control1.x - anchor1.x);
      float bx = 3 * (control2.x - control1.x) - cx;
      float ax = anchor2.x - anchor1.x - cx - bx;
      float cy = 3 * (control1.y - anchor1.y);
      float by = 3 * (control2.y - control1.y) - cy;
      float ay = anchor2.y - anchor1.y - cy - by;
    
      return new Vector2((ax * (t * t * t)) + (bx * (t * t)) + (cx * t) + anchor1.x,
        (ay * (t * t * t)) + (by * (t * t)) + (cy * t) + anchor1.y);
    }

    private static Vector3 GetBezierPoint3D(ref Vector3 anchor1, ref Vector3 control1, ref Vector3 anchor2, ref Vector3 control2, float t) {
      float cx = 3 * (control1.x - anchor1.x);
      float bx = 3 * (control2.x - control1.x) - cx;
      float ax = anchor2.x - anchor1.x - cx - bx;
      float cy = 3 * (control1.y - anchor1.y);
      float by = 3 * (control2.y - control1.y) - cy;
      float ay = anchor2.y - anchor1.y - cy - by;
      float cz = 3 * (control1.z - anchor1.z);
      float bz = 3 * (control2.z - control1.z) - cz;
      float az = anchor2.z - anchor1.z - cz - bz;
    
      return new Vector3((ax * (t * t * t)) + (bx * (t * t)) + (cx * t) + anchor1.x,
        (ay * (t * t * t)) + (by * (t * t)) + (cy * t) + anchor1.y,
        (az * (t * t * t)) + (bz * (t * t)) + (cz * t) + anchor1.z);
    }

    public void MakeSpline(Vector2[] splinePoints) {
      MakeSpline(splinePoints, null, GetSegmentNumber(), 0, false);
    }

    public void MakeSpline(Vector2[] splinePoints, bool loop) {
      MakeSpline(splinePoints, null, GetSegmentNumber(), 0, loop);
    }

    public void MakeSpline(Vector2[] splinePoints, int segments) {
      MakeSpline(splinePoints, null, segments, 0, false);
    }

    public void MakeSpline(Vector2[] splinePoints, int segments, bool loop) {
      MakeSpline(splinePoints, null, segments, 0, loop);
    }

    public void MakeSpline(Vector2[] splinePoints, int segments, int index) {
      MakeSpline(splinePoints, null, segments, index, false);
    }

    public void MakeSpline(Vector2[] splinePoints, int segments, int index, bool loop) {
      MakeSpline(splinePoints, null, segments, index, loop);
    }

    public void MakeSpline(Vector3[] splinePoints) {
      MakeSpline(null, splinePoints, GetSegmentNumber(), 0, false);
    }

    public void MakeSpline(Vector3[] splinePoints, bool loop) {
      MakeSpline(null, splinePoints, GetSegmentNumber(), 0, loop);
    }

    public void MakeSpline(Vector3[] splinePoints, int segments) {
      MakeSpline(null, splinePoints, segments, 0, false);
    }

    public void MakeSpline(Vector3[] splinePoints, int segments, bool loop) {
      MakeSpline(null, splinePoints, segments, 0, loop);
    }

    public void MakeSpline(Vector3[] splinePoints, int segments, int index) {
      MakeSpline(null, splinePoints, segments, index, false);
    }

    public void MakeSpline(Vector3[] splinePoints, int segments, int index, bool loop) {
      MakeSpline(null, splinePoints, segments, index, loop);
    }

    private void MakeSpline(Vector2[] splinePoints2, Vector3[] splinePoints3, int segments, int index, bool loop) {
      int pointsLength = (splinePoints2 != null) ? splinePoints2.Length : splinePoints3.Length;    
      if (pointsLength < 2) {
        Debug.LogError("VectorLine.MakeSpline needs at least 2 spline points");
        return;
      }
      if (splinePoints2 != null && !m_is2D) {
        Debug.LogError("VectorLine.MakeSpline was called with a Vector2 spline points array, but the line uses Vector3 points");
        return;
      }
      if (splinePoints3 != null && m_is2D) {
        Debug.LogError("VectorLine.MakeSpline was called with a Vector3 spline points array, but the line uses Vector2 points");
        return;
      }
      if (!CheckArrayLength(FunctionName.MakeSpline, segments, index)) {
        return;
      }

      var pointCount = index;
      var numberOfPoints = loop ? pointsLength : pointsLength - 1;
      var add = 1.0f / segments * numberOfPoints;
      float i, start = 0.0f;
      int j, p0 = 0, p2 = 0, p3 = 0;
    
      for (j = 0; j < numberOfPoints; j++) {
        p0 = j - 1;
        p2 = j + 1;
        p3 = j + 2;
        if (p0 < 0) {
          p0 = loop ? numberOfPoints - 1 : 0;
        }
        if (loop && p2 > numberOfPoints - 1) {
          p2 -= numberOfPoints;
        }
        if (p3 > numberOfPoints - 1) {
          p3 = loop ? p3 - numberOfPoints : numberOfPoints;
        }
        if (m_continuous) {
          if (m_is2D) {
            for (i = start; i <= 1.0f; i += add) {
              m_points2[pointCount++] = GetSplinePoint(ref splinePoints2[p0], ref splinePoints2[j], ref splinePoints2[p2], ref splinePoints2[p3], i);
            }
          }
          else {
            for (i = start; i <= 1.0f; i += add) {
              m_points3[pointCount++] = GetSplinePoint3D(ref splinePoints3[p0], ref splinePoints3[j], ref splinePoints3[p2], ref splinePoints3[p3], i);
            }
          }
        }
        else {
          if (m_is2D) {
            for (i = start; i <= 1.0f; i += add) {
              m_points2[pointCount++] = GetSplinePoint(ref splinePoints2[p0], ref splinePoints2[j], ref splinePoints2[p2], ref splinePoints2[p3], i);
              if (pointCount > index + 1 && pointCount < index + (segments * 2)) {
                m_points2[pointCount++] = m_points2[pointCount - 2];
              }
            }
          }
          else {
            for (i = start; i <= 1.0f; i += add) {
              m_points3[pointCount++] = GetSplinePoint3D(ref splinePoints3[p0], ref splinePoints3[j], ref splinePoints3[p2], ref splinePoints3[p3], i);
              if (pointCount > index + 1 && pointCount < index + (segments * 2)) {
                m_points3[pointCount++] = m_points3[pointCount - 2];
              }
            }
          }
        }
        start = i - 1.0f;
      }
      // The last point might not get done depending on number of splinePoints and segments, so ensure that it's done here
      if ((m_continuous && pointCount < index + (segments + 1)) || (!m_continuous && pointCount < index + (segments * 2))) {
        if (m_is2D) {
          m_points2[pointCount] = GetSplinePoint(ref splinePoints2[p0], ref splinePoints2[j - 1], ref splinePoints2[p2], ref splinePoints2[p3], 1.0f);
        }
        else {
          m_points3[pointCount] = GetSplinePoint3D(ref splinePoints3[p0], ref splinePoints3[j - 1], ref splinePoints3[p2], ref splinePoints3[p3], 1.0f);
        }
      }
    }

    private static Vector2 GetSplinePoint(ref Vector2 p0, ref Vector2 p1, ref Vector2 p2, ref Vector2 p3, float t) {
      var px = Vector4.zero;
      var py = Vector4.zero;
      float dt0 = Mathf.Pow(VectorDistanceSquared(ref p0, ref p1), 0.25f);
      float dt1 = Mathf.Pow(VectorDistanceSquared(ref p1, ref p2), 0.25f);
      float dt2 = Mathf.Pow(VectorDistanceSquared(ref p2, ref p3), 0.25f);
    
      if (dt1 < 0.0001f)
        dt1 = 1.0f;
      if (dt0 < 0.0001f)
        dt0 = dt1;
      if (dt2 < 0.0001f)
        dt2 = dt1;
    
      InitNonuniformCatmullRom(p0.x, p1.x, p2.x, p3.x, dt0, dt1, dt2, ref px);
      InitNonuniformCatmullRom(p0.y, p1.y, p2.y, p3.y, dt0, dt1, dt2, ref py);
    
      return new Vector2(EvalCubicPoly(ref px, t), EvalCubicPoly(ref py, t));
    }

    private static Vector3 GetSplinePoint3D(ref Vector3 p0, ref Vector3 p1, ref Vector3 p2, ref Vector3 p3, float t) {
      var px = Vector4.zero;
      var py = Vector4.zero;
      var pz = Vector4.zero;
      float dt0 = Mathf.Pow(VectorDistanceSquared(ref p0, ref p1), 0.25f);
      float dt1 = Mathf.Pow(VectorDistanceSquared(ref p1, ref p2), 0.25f);
      float dt2 = Mathf.Pow(VectorDistanceSquared(ref p2, ref p3), 0.25f);
    
      if (dt1 < 0.0001f)
        dt1 = 1.0f;
      if (dt0 < 0.0001f)
        dt0 = dt1;
      if (dt2 < 0.0001f)
        dt2 = dt1;
    
      InitNonuniformCatmullRom(p0.x, p1.x, p2.x, p3.x, dt0, dt1, dt2, ref px);
      InitNonuniformCatmullRom(p0.y, p1.y, p2.y, p3.y, dt0, dt1, dt2, ref py);
      InitNonuniformCatmullRom(p0.z, p1.z, p2.z, p3.z, dt0, dt1, dt2, ref pz);
    
      return new Vector3(EvalCubicPoly(ref px, t), EvalCubicPoly(ref py, t), EvalCubicPoly(ref pz, t));
    }

    private static float VectorDistanceSquared(ref Vector2 p, ref Vector2 q) {
      float dx = q.x - p.x;
      float dy = q.y - p.y;
      return dx * dx + dy * dy;
    }

    private static float VectorDistanceSquared(ref Vector3 p, ref Vector3 q) {
      float dx = q.x - p.x;
      float dy = q.y - p.y;
      float dz = q.z - p.z;
      return dx * dx + dy * dy + dz * dz;
    }

    private static void InitNonuniformCatmullRom(float x0, float x1, float x2, float x3, float dt0, float dt1, float dt2, ref Vector4 p) {
      float t1 = ((x1 - x0) / dt0 - (x2 - x0) / (dt0 + dt1) + (x2 - x1) / dt1) * dt1;
      float t2 = ((x2 - x1) / dt1 - (x3 - x1) / (dt1 + dt2) + (x3 - x2) / dt2) * dt1;
    
      // Initialize cubic poly
      p.x = x1;
      p.y = t1;
      p.z = -3 * x1 + 3 * x2 - 2 * t1 - t2;
      p.w = 2 * x1 - 2 * x2 + t1 + t2;
    }

    private static float EvalCubicPoly(ref Vector4 p, float t) {
      return p.x + p.y * t + p.z * (t * t) + p.w * (t * t * t);
    }

    public void MakeText(string text, Vector3 startPos, float size) {
      MakeText(text, startPos, size, 1.0f, 1.5f, true);
    }

    public void MakeText(string text, Vector3 startPos, float size, bool uppercaseOnly) {
      MakeText(text, startPos, size, 1.0f, 1.5f, uppercaseOnly);
    }

    public void MakeText(string text, Vector3 startPos, float size, float charSpacing, float lineSpacing) {
      MakeText(text, startPos, size, charSpacing, lineSpacing, true);
    }

    public void MakeText(string text, Vector3 startPos, float size, float charSpacing, float lineSpacing, bool uppercaseOnly) {
      if (m_continuous) {
        Debug.LogError("VectorLine.MakeText only works with a discrete line");
        return;
      }
      int charPointsLength = 0;
    
      // Get total number of points needed for all characters in the string
      for (int i = 0; i < text.Length; i++) {
        int charNum = System.Convert.ToInt32(text[i]);
        if (charNum < 0 || charNum > VectorChar.numberOfCharacters) {
          Debug.LogError("VectorLine.MakeText: Character '" + text[i] + "' is not valid");
          return;
        }
        if (uppercaseOnly && charNum >= 97 && charNum <= 122) {
          charNum -= 32;
        }
        if (VectorChar.data[charNum] != null) {
          charPointsLength += VectorChar.data[charNum].Length;
        }
      }
      if (charPointsLength != pointsCount) {
        Resize(charPointsLength);
      }
    
      float charPos = 0.0f, linePos = 0.0f;
      int idx = 0;
      var scaleVector = new Vector2(size, size);

      for (int i = 0; i < text.Length; i++) {
        int charNum = System.Convert.ToInt32(text[i]);
        // Newline
        if (charNum == 10) {
          linePos -= lineSpacing;
          charPos = 0.0f;
        }
      // Space
      else if (charNum == 32) {
          charPos += charSpacing;
        }
      // Character
      else {
          if (uppercaseOnly && charNum >= 97 && charNum <= 122) {
            charNum -= 32;
          }
          int end = 0;
          if (VectorChar.data[charNum] != null) {
            end = VectorChar.data[charNum].Length;
          }
          else {
            charPos += charSpacing;
            continue;
          }
          if (m_is2D) {
            for (int j = 0; j < end; j++) {
              m_points2[idx++] = Vector2.Scale(VectorChar.data[charNum][j] + new Vector2(charPos, linePos), scaleVector) + (Vector2)startPos;
            }
          }
          else {
            for (int j = 0; j < end; j++) {
              m_points3[idx++] = Vector3.Scale((Vector3)VectorChar.data[charNum][j] + new Vector3(charPos, linePos, 0.0f), scaleVector) + startPos;
            }
          }
          charPos += charSpacing;
        }
      }
    }

    public void MakeWireframe(Mesh mesh) {
      if (m_continuous) {
        Debug.LogError("VectorLine.MakeWireframe only works with a discrete line");
        return;
      }
      if (m_is2D) {
        Debug.LogError("VectorLine.MakeWireframe can only be used with Vector3 points, which \"" + name + "\" doesn't have");
        return;
      }
      if (mesh == null) {
        Debug.LogError("VectorLine.MakeWireframe can't use a null mesh");
        return;
      }
      var meshTris = mesh.triangles;
      var meshVertices = mesh.vertices;
      var pairs = new Dictionary<Vector3Pair, bool>();
      var linePoints = new List<Vector3>();
    
      for (int i = 0; i < meshTris.Length; i += 3) {
        CheckPairPoints(pairs, meshVertices[meshTris[i]], meshVertices[meshTris[i + 1]], linePoints);
        CheckPairPoints(pairs, meshVertices[meshTris[i + 1]], meshVertices[meshTris[i + 2]], linePoints);
        CheckPairPoints(pairs, meshVertices[meshTris[i + 2]], meshVertices[meshTris[i]], linePoints);
      }
    
      if (linePoints.Count != m_pointsCount) {
        Resize(linePoints.Count);
      }
      for (int i = 0; i < m_pointsCount; i++) {
        m_points3[i] = linePoints[i];
      }
    }

    private static void CheckPairPoints(Dictionary<Vector3Pair, bool> pairs, Vector3 p1, Vector3 p2, List<Vector3> linePoints) {
      var pair1 = new Vector3Pair(p1, p2);
      var pair2 = new Vector3Pair(p2, p1);
      if (!pairs.ContainsKey(pair1) && !pairs.ContainsKey(pair2)) {
        pairs[pair1] = true;
        pairs[pair2] = true;
        linePoints.Add(p1);
        linePoints.Add(p2);
      }
    }

    public void MakeCube(Vector3 position, float xSize, float ySize, float zSize) {
      MakeCube(position, xSize, ySize, zSize, 0);
    }

    public void MakeCube(Vector3 position, float xSize, float ySize, float zSize, int index) {
      if (m_continuous) {
        Debug.LogError("VectorLine.MakeCube only works with a discrete line");
        return;
      }
      if (m_is2D) {
        Debug.LogError("VectorLine.MakeCube can only be used with Vector3 points, which \"" + name + "\" doesn't have");
        return;
      }
      if (index + 24 > m_pointsCount) {
        if (index == 0) {
          Debug.LogError("VectorLine.MakeCube: The number of Vector3 points needs to be at least 24 for \"" + name + "\"");
          return;
        }
        Debug.LogError("Calling VectorLine.MakeCube with an index of " + index + " would exceed the length of the Vector3 points for \"" + name + "\"");
        return;
      }
    
      xSize /= 2;
      ySize /= 2;
      zSize /= 2;
      // Top
      m_points3[index] = position + new Vector3(-xSize, ySize, -zSize);
      m_points3[index + 1] = position + new Vector3(xSize, ySize, -zSize);
      m_points3[index + 2] = position + new Vector3(xSize, ySize, -zSize);
      m_points3[index + 3] = position + new Vector3(xSize, ySize, zSize);
      m_points3[index + 4] = position + new Vector3(xSize, ySize, zSize);
      m_points3[index + 5] = position + new Vector3(-xSize, ySize, zSize);
      m_points3[index + 6] = position + new Vector3(-xSize, ySize, zSize);
      m_points3[index + 7] = position + new Vector3(-xSize, ySize, -zSize);
      // Middle
      m_points3[index + 8] = position + new Vector3(-xSize, -ySize, -zSize);
      m_points3[index + 9] = position + new Vector3(-xSize, ySize, -zSize);
      m_points3[index + 10] = position + new Vector3(xSize, -ySize, -zSize);
      m_points3[index + 11] = position + new Vector3(xSize, ySize, -zSize);
      m_points3[index + 12] = position + new Vector3(-xSize, -ySize, zSize);
      m_points3[index + 13] = position + new Vector3(-xSize, ySize, zSize);
      m_points3[index + 14] = position + new Vector3(xSize, -ySize, zSize);
      m_points3[index + 15] = position + new Vector3(xSize, ySize, zSize);
      // Bottom
      m_points3[index + 16] = position + new Vector3(-xSize, -ySize, -zSize);
      m_points3[index + 17] = position + new Vector3(xSize, -ySize, -zSize);
      m_points3[index + 18] = position + new Vector3(xSize, -ySize, -zSize);
      m_points3[index + 19] = position + new Vector3(xSize, -ySize, zSize);
      m_points3[index + 20] = position + new Vector3(xSize, -ySize, zSize);
      m_points3[index + 21] = position + new Vector3(-xSize, -ySize, zSize);
      m_points3[index + 22] = position + new Vector3(-xSize, -ySize, zSize);
      m_points3[index + 23] = position + new Vector3(-xSize, -ySize, -zSize);
    }

    public void SetDistances() {
      if (m_distances == null || m_distances.Length != (m_continuous ? m_pointsCount : m_pointsCount / 2 + 1)) {
        m_distances = new float[m_continuous ? m_pointsCount : m_pointsCount / 2 + 1];
      }

      var totalDistance = 0.0d;
      int thisPointsLength = pointsCount - 1;
    
      if (m_points3 != null) {
        if (m_continuous) {
          for (int i = 0; i < thisPointsLength; i++) {
            Vector3 diff = m_points3[i] - m_points3[i + 1];
            totalDistance += System.Math.Sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z); // Same as Vector3.Distance, but with double instead of float
            m_distances[i + 1] = (float)totalDistance;
          }
        }
        else {
          int count = 1;
          for (int i = 0; i < thisPointsLength; i += 2) {
            Vector3 diff = m_points3[i] - m_points3[i + 1];
            totalDistance += System.Math.Sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
            m_distances[count++] = (float)totalDistance;
          }
        }
      }
      else {
        if (m_continuous) {
          for (int i = 0; i < thisPointsLength; i++) {
            Vector2 diff = m_points2[i] - m_points2[i + 1];
            totalDistance += System.Math.Sqrt(diff.x * diff.x + diff.y * diff.y); // Same as Vector2.Distance, but with double instead of float
            m_distances[i + 1] = (float)totalDistance;
          }
        }
        else {
          int count = 1;
          for (int i = 0; i < thisPointsLength; i += 2) {
            Vector2 diff = m_points2[i] - m_points2[i + 1];
            totalDistance += System.Math.Sqrt(diff.x * diff.x + diff.y * diff.y);
            m_distances[count++] = (float)totalDistance;
          }
        }
      }
    }

    public float GetLength() {
      if (m_distances == null || m_distances.Length != (m_continuous ? pointsCount : pointsCount / 2 + 1)) {
        SetDistances();
      }
      return m_distances[m_distances.Length - 1];
    }

    public Vector2 GetPoint01(float distance) {
      int index;
      return GetPoint(Mathf.Lerp(0.0f, GetLength(), distance), out index);
    }

    public Vector2 GetPoint01(float distance, out int index) {
      return GetPoint(Mathf.Lerp(0.0f, GetLength(), distance), out index);
    }

    public Vector2 GetPoint(float distance) {
      int index;
      return GetPoint(distance, out index);
    }

    public Vector2 GetPoint(float distance, out int index) {
      if (!m_is2D) {
        Debug.LogError("VectorLine.GetPoint only works with Vector2 points");
        index = 0;
        return Vector2.zero;
      }
    
      SetDistanceIndex(out index, distance);
      Vector2 point;
      if (m_continuous) {
        point = Vector2.Lerp(m_points2[index - 1], m_points2[index], Mathf.InverseLerp(m_distances[index - 1], m_distances[index], distance));
      }
      else {
        point = Vector2.Lerp(m_points2[(index - 1) * 2], m_points2[(index - 1) * 2 + 1], Mathf.InverseLerp(m_distances[index - 1], m_distances[index], distance));
      }
      if (m_drawTransform) {
        point += new Vector2(m_drawTransform.position.x, m_drawTransform.position.y);
      }
      index--;
      return point;
    }

    public Vector3 GetPoint3D01(float distance) {
      int index;
      return GetPoint3D(Mathf.Lerp(0.0f, GetLength(), distance), out index);
    }

    public Vector3 GetPoint3D01(float distance, out int index) {
      return GetPoint3D(Mathf.Lerp(0.0f, GetLength(), distance), out index);
    }

    public Vector3 GetPoint3D(float distance) {
      int index;
      return GetPoint3D(distance, out index);
    }

    public Vector3 GetPoint3D(float distance, out int index) {
      if (m_is2D) {
        Debug.LogError("VectorLine.GetPoint3D only works with Vector3 points");
        index = 0;
        return Vector3.zero;
      }
    
      SetDistanceIndex(out index, distance);
      Vector3 point;
      if (m_continuous) {
        point = Vector3.Lerp(m_points3[index - 1], m_points3[index], Mathf.InverseLerp(m_distances[index - 1], m_distances[index], distance));      
      }
      else {
        point = Vector3.Lerp(m_points3[(index - 1) * 2], m_points3[(index - 1) * 2 + 1], Mathf.InverseLerp(m_distances[index - 1], m_distances[index], distance));
      }
      if (m_drawTransform) {
        point += m_drawTransform.position;
      }
      index--;
      return point;
    }

    void SetDistanceIndex(out int i, float distance) {
      if (m_distances == null) {
        SetDistances();
      }
      i = m_drawStart + 1;
      if (!m_continuous) {
        i = (i + 1) / 2;
      }
      if (i >= m_distances.Length) {
        i = m_distances.Length - 1;
      }
      int end = m_drawEnd;
      if (!m_continuous) {
        end = (end + 1) / 2;
      }
      while (distance > m_distances[i] && i < end) {
        i++;
      }
    }

    public static void SetEndCap(string name, EndCap capType) {
      SetEndCap(name, capType, null, 0.0f, null);
    }

    public static void SetEndCap(string name, EndCap capType, Material material, params Texture2D[] textures) {
      SetEndCap(name, capType, material, 0.0f, textures);
    }

    public static void SetEndCap(string name, EndCap capType, Material material, float offset, params Texture2D[] textures) {
      if (capDictionary == null) {
        capDictionary = new Dictionary<string, CapInfo>();
      }
      if (name == null || name == "") {
        Debug.LogError("VectorLine: must supply a name for SetEndCap");
        return;
      }
      if (capDictionary.ContainsKey(name) && capType != EndCap.None) {
        Debug.LogError("VectorLine: end cap \"" + name + "\" has already been set up");
        return;
      }
    
      if (capType == EndCap.Both) {
        if (textures.Length < 2) {
          Debug.LogError("VectorLine: must supply two textures when using SetEndCap with EndCap.Both");
          return;
        }
        if (textures[0].width != textures[1].width || textures[0].height != textures[1].height) {
          Debug.LogError("VectorLine: when using SetEndCap with EndCap.Both, both textures must have the same width and height");
          return;
        }
      }
      if ((capType == EndCap.Front || capType == EndCap.Back || capType == EndCap.Mirror) && textures.Length < 1) {
        Debug.LogError("VectorLine: must supply a texture when using SetEndCap with EndCap.Front, EndCap.Back, or EndCap.Mirror");
        return;
      }
      if (capType == EndCap.None) {
        if (!capDictionary.ContainsKey(name)) {
          return;
        }
        RemoveEndCap(name);
        return;
      }
      if (material == null) {
        Debug.LogError("VectorLine: must supply a material when using SetEndCap with any EndCap type except EndCap.None");
        return;
      }
      if (!material.HasProperty("_MainTex")) {
        Debug.LogError("VectorLine: the material supplied when using SetEndCap must contain a shader that has a \"_MainTex\" property");
        return;
      }
    
      int width = textures[0].width;
      int height = textures[0].height;
      float ratio1 = 0.0f, ratio2 = 0.0f;
      Color[] cols1 = null, cols2 = null;
      if (capType == EndCap.Front) {
        cols1 = textures[0].GetPixels();
        cols2 = new Color[width * height];
        ratio1 = textures[0].width / (float)textures[0].height;
      }
      else if (capType == EndCap.Back) {
        cols1 = new Color[width * height];
        cols2 = textures[0].GetPixels();
        ratio2 = textures[0].width / (float)textures[0].height;
      }
      else if (capType == EndCap.Both) {
        cols1 = textures[0].GetPixels();
        cols2 = textures[1].GetPixels();
        ratio1 = textures[0].width / (float)textures[0].height;
        ratio2 = textures[1].width / (float)textures[1].height;
      }
      else if (capType == EndCap.Mirror) {
        cols1 = textures[0].GetPixels();
        cols2 = new Color[width * height];
        ratio1 = textures[0].width / (float)textures[0].height;
        ratio2 = ratio1;
      }
    
      var tex = new Texture2D(width, height * 4, TextureFormat.ARGB32, false);
      tex.wrapMode = TextureWrapMode.Clamp;
      tex.filterMode = textures[0].filterMode;
      tex.SetPixels(0, 0, width, height, cols1);
      tex.SetPixels(0, height * 3, width, height, cols2);
      // Add space to prevent top/bottom textures from potentially bleeding into each other when using bilinear filtering
      tex.SetPixels(0, height, width, height * 2, new Color[width * (height * 2)]);
      tex.Apply(false, true);
      var capMaterial = (Material)MonoBehaviour.Instantiate(material);
      capMaterial.name = material.name + " EndCap";
      capMaterial.mainTexture = tex;
    
      capDictionary.Add(name, new CapInfo(capType, capMaterial, tex, ratio1, ratio2, offset));
    }

    public static void RemoveEndCap(string name) {
      if (!capDictionary.ContainsKey(name)) {
        Debug.LogError("VectorLine: RemoveEndCap: \"" + name + "\" has not been set up");
        return;
      }
      MonoBehaviour.Destroy(capDictionary[name].texture);
      MonoBehaviour.Destroy(capDictionary[name].material);
      capDictionary.Remove(name);
    }

    public bool Selected(Vector2 p) {
      int temp;
      return Selected(p, 0, 0, out temp);
    }

    public bool Selected(Vector2 p, out int index) {
      return Selected(p, 0, 0, out index);
    }

    public bool Selected(Vector2 p, int extraDistance, out int index) {
      return Selected(p, extraDistance, 0, out index);
    }

    public bool Selected(Vector2 p, int extraDistance, int extraLength, out int index) {
      int wAdd = m_lineWidths.Length == 1 ? 0 : 1;
      int wIdx = m_continuous ? m_drawStart - wAdd : m_drawStart / 2 - wAdd;
      if (m_lineWidths.Length == 1) {
        wAdd = 0;
        wIdx = 0;
      }
      else {
        wAdd = 1;
      }
      int end = m_drawEnd;
      var useTransformMatrix = (m_drawTransform != null);
      var thisMatrix = useTransformMatrix ? m_drawTransform.localToWorldMatrix : Matrix4x4.identity;
      var scaleFactor = new Vector2(Screen.width, Screen.height);
    
      if (m_isPoints) {
        if (end == pointsCount) {
          end--;
        }
        Vector2 thisPoint;
      
        if (m_is2D) {
          for (int i = m_drawStart; i <= end; i++) {
            wIdx += wAdd;
            float size = m_lineWidths[wIdx] + extraDistance;
            thisPoint = useTransformMatrix ? (Vector2)thisMatrix.MultiplyPoint3x4(m_points2[i]) : m_points2[i];
            if (m_viewportDraw) {
              thisPoint.x *= scaleFactor.x;
              thisPoint.y *= scaleFactor.y;
            }
            if (p.x >= thisPoint.x - size && p.x <= thisPoint.x + size && p.y >= thisPoint.y - size && p.y <= thisPoint.y + size) {
              index = i;
              return true;
            }
          }
          index = -1;
          return false;
        }
      
        for (int i = m_drawStart; i <= end; i++) {
          wIdx += wAdd;
          float size = m_lineWidths[wIdx] + extraDistance;
          thisPoint = useTransformMatrix ? cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i])) : cam3D.WorldToScreenPoint(m_points3[i]);
          if (p.x >= thisPoint.x - size && p.x <= thisPoint.x + size && p.y >= thisPoint.y - size && p.y <= thisPoint.y + size) {
            index = i;
            return true;
          }
        }
        index = -1;
        return false;
      }
    
      float t = 0.0f;
      int add = m_continuous ? 1 : 2;
      Vector2 p1, p2, d = Vector2.zero;
      if (m_continuous && m_drawEnd == pointsCount) {
        end--;
      }
    
      if (m_is2D) {
        for (int i = m_drawStart; i < end; i += add) {
          wIdx += wAdd;
          if (useTransformMatrix) {
            p1 = thisMatrix.MultiplyPoint3x4(m_points2[i]);
            p2 = thisMatrix.MultiplyPoint3x4(m_points2[i + 1]);
          }
          else {
            p1.x = m_points2[i].x;
            p1.y = m_points2[i].y;
            p2.x = m_points2[i + 1].x;
            p2.y = m_points2[i + 1].y;
          }
          if (m_viewportDraw) {
            p1.x *= scaleFactor.x;
            p1.y *= scaleFactor.y;
            p2.x *= scaleFactor.x;
            p2.y *= scaleFactor.y;
          }
        
          // Extend line segment
          if (extraLength > 0) {
            d = (p1 - p2).normalized * extraLength;
            p1.x += d.x;
            p1.y += d.y;
            p2.x -= d.x;
            p2.y -= d.y;
          }
        
          // Do nothing if the point is beyond the line segment end points
          t = Vector2.Dot(p - p1, p2 - p1) / (p2 - p1).sqrMagnitude;
          if (t < 0.0f || t > 1.0f) {
            continue;
          }
        
          // If the distance of the point to the line is <= the line width
          if ((p - (p1 + t * (p2 - p1))).sqrMagnitude <= (m_lineWidths[wIdx] + extraDistance) * (m_lineWidths[wIdx] + extraDistance)) {
            index = m_continuous ? i : i / 2;
            return true;
          }
        }
        index = -1;
        return false;
      }
    
      Vector3 screenPoint1, screenPoint2 = v3zero;
      for (int i = m_drawStart; i < end; i += add) {
        wIdx += wAdd;
        if (useTransformMatrix) {
          screenPoint1 = cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i]));
          screenPoint2 = cam3D.WorldToScreenPoint(thisMatrix.MultiplyPoint3x4(m_points3[i + 1]));
        }
        else {
          screenPoint1 = cam3D.WorldToScreenPoint(m_points3[i]);
          screenPoint2 = cam3D.WorldToScreenPoint(m_points3[i + 1]);
        }
        if (screenPoint1.z < 0 || screenPoint2.z < 0) {
          continue;
        }
        p1.x = (int)screenPoint1.x;
        p2.x = (int)screenPoint2.x;
        p1.y = (int)screenPoint1.y;
        p2.y = (int)screenPoint2.y;
        if (p1.x == p2.x && p1.y == p2.y) {
          continue;
        }
      
        // Extend line segment
        if (extraLength > 0) {
          d = (p1 - p2).normalized * extraLength;
          p1.x += d.x;
          p1.y += d.y;
          p2.x -= d.x;
          p2.y -= d.y;
        }
      
        // Do nothing if the point is beyond the line segment end points
        t = Vector2.Dot(p - p1, p2 - p1) / (p2 - p1).sqrMagnitude;
        if (t < 0.0f || t > 1.0f) {
          continue;
        }
      
        // If the distance of the point to the line is <= the line width
        if ((p - (p1 + t * (p2 - p1))).sqrMagnitude <= (m_lineWidths[wIdx] + extraDistance) * (m_lineWidths[wIdx] + extraDistance)) {
          index = m_continuous ? i : i / 2;
          return true;
        }
      }
      index = -1;
      return false;
    }

    bool Approximately(Vector2 p1, Vector2 p2) {
      return Approximately(p1.x, p2.x) && Approximately(p1.y, p2.y);
    }

    bool Approximately(Vector3 p1, Vector3 p2) {
      return Approximately(p1.x, p2.x) && Approximately(p1.y, p2.y) && Approximately(p1.z, p2.z);
    }

    bool Approximately(float a, float b) {
      return Mathf.Round(a * 100) / 100 == Mathf.Round(b * 100) / 100;
    }
  }

  public class VectorPoints : VectorLine {
    public VectorPoints(string name, Vector2[] points, Material material, float width) : base(true, name, points, material, width) {
    }

    public VectorPoints(string name, List<Vector2> points, Material material, float width) : base(true, name, points, material, width) {
    }

    public VectorPoints(string name, Vector3[] points, Material material, float width) : base(true, name, points, material, width) {
    }

    public VectorPoints(string name, List<Vector3> points, Material material, float width) : base(true, name, points, material, width) {
    }
  }

  public struct Vector3Pair {
    public Vector3 p1;
    public Vector3 p2;

    public Vector3Pair(Vector3 point1, Vector3 point2) {
      p1 = point1;
      p2 = point2;
    }
  }

  public class CapInfo {
    public EndCap capType;
    public Material material;
    public Texture2D texture;
    public float ratio1;
    public float ratio2;
    public float offset;

    public CapInfo(EndCap capType, Material material, Texture2D texture, float ratio1, float ratio2, float offset) {
      this.capType = capType;
      this.material = material;
      this.texture = texture;
      this.ratio1 = ratio1;
      this.ratio2 = ratio2;
      this.offset = offset;
    }
  }

}