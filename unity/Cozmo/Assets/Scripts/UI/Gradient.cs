using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;


public enum GradientMode {
  Global,
  Local
}

public enum GradientDir {
  Vertical,
  Horizontal,
  DiagonalLeftToRight,
  DiagonalRightToLeft
  //Free
}
//enum color mode Additive, Multiply, Overwrite

[AddComponentMenu("UI/Effects/Gradient")]
public class Gradient : BaseVertexEffect {

  public GradientMode gradientMode = GradientMode.Global;
  public GradientDir gradientDir = GradientDir.Vertical;
  public bool overwriteAllColor = false;

  [SerializeField] Color[] colors = new Color[2];
  [SerializeField] Graphic targetGraphic;

  public Graphic TargetGraphic {
    get {
      if (targetGraphic == null)
        targetGraphic = GetComponent<Graphic>();
      return targetGraphic;
    }
  }

  public override void ModifyVertices(List<UIVertex> vertexList) {
    if (!IsActive() || vertexList.Count == 0) {
      return;
    }

    int count = vertexList.Count;
    UIVertex uiVertex = vertexList[0];

    if (gradientMode == GradientMode.Global) {

      float length = 1f;

      float minX = float.MaxValue;
      float maxX = 0f;
      float minY = float.MaxValue;
      float maxY = 0f;

      for (int i = 0; i < count; i++) {
        uiVertex = vertexList[i];
        minX = Mathf.Min(minX, vertexList[i].position.x);
        maxX = Mathf.Max(maxX, vertexList[i].position.x);
        minY = Mathf.Min(minY, vertexList[i].position.y);
        maxY = Mathf.Max(maxY, vertexList[i].position.y);
      }

      Vector2 start = new Vector2(minX, minY);
      Vector2 end = new Vector2(maxX, maxY);

      switch (gradientDir) {
      case GradientDir.Vertical:
        length = end.y - start.y;
        break;
      case GradientDir.Horizontal:
        length = end.x - start.x;
        break;
      case GradientDir.DiagonalLeftToRight:
        length = (end - start).magnitude;
        break;
      case GradientDir.DiagonalRightToLeft:
        start = new Vector2(maxX, minY);
        end = new Vector2(minX, maxY);
        length = (end - start).magnitude;
        break;
      }

      if (length == 0f) {
        DAS.Error("Gradient", "bad verts, gradient undefined!");
        return;
      }

      Vector2 direction = (end - start).normalized;

      for (int i = 0; i < count; i++) {
        uiVertex = vertexList[i];
        if (!overwriteAllColor && uiVertex.color != TargetGraphic.color)
          continue;

        float factor = 0f;
        switch (gradientDir) {
        case GradientDir.Vertical:
          factor = 1f - (uiVertex.position.y - start.y) / length;
          break;
        case GradientDir.Horizontal:
          factor = (uiVertex.position.x - start.x) / length;
          break;
        default:
          factor = Vector2.Dot(((Vector2)uiVertex.position - start), direction) / length;
          break;
        }

        uiVertex.color *= LerpColorArray(factor);
        vertexList[i] = uiVertex;
      }

    }
    else {

      for (int i = 0; i < count; i++) {

        uiVertex = vertexList[i];
        if (!overwriteAllColor && !CompareCarefully(uiVertex.color, TargetGraphic.color))
          continue;

        switch (gradientDir) {
        case GradientDir.Vertical:
          uiVertex.color *= (i % 4 == 0 || (i - 1) % 4 == 0) ? LerpColorArray(0) : LerpColorArray(1);
          break;
        case GradientDir.Horizontal:
          uiVertex.color *= (i % 4 == 0 || (i - 3) % 4 == 0) ? LerpColorArray(0) : LerpColorArray(1);
          break;
        case GradientDir.DiagonalLeftToRight:
          uiVertex.color *= (i % 4 == 0) ? LerpColorArray(0) : ((i - 2) % 4 == 0 ? LerpColorArray(1) : LerpColorArray(0.5f));
          break;
        case GradientDir.DiagonalRightToLeft:
          uiVertex.color *= ((i - 1) % 4 == 0) ? LerpColorArray(0) : ((i - 3) % 4 == 0 ? LerpColorArray(1) : LerpColorArray(0.5f));
          break;
        }

        vertexList[i] = uiVertex;
      }
    }
  }

  Color LerpColorArray(float t) {
    if (colors == null || colors.Length == 0)
      return Color.white;
    if (t == 0f || colors.Length == 1)
      return colors[0];
    if (t == 1f)
      return colors[colors.Length - 1];
    if (colors.Length == 2)
      return Color.Lerp(colors[0], colors[1], t);

    float stretch = 1f / (float)(colors.Length - 1);

    float t1 = t % stretch;
    int i = Mathf.Clamp(Mathf.FloorToInt(t / stretch), 0, colors.Length - 2);

    return Color.Lerp(colors[i], colors[i + 1], t1);
  }

  bool CompareCarefully(Color col1, Color col2) {
    if (Mathf.Abs(col1.r - col2.r) < 0.003f && Mathf.Abs(col1.g - col2.g) < 0.003f && Mathf.Abs(col1.b - col2.b) < 0.003f && Mathf.Abs(col1.a - col2.a) < 0.003f)
      return true;
    return false;
  }
}