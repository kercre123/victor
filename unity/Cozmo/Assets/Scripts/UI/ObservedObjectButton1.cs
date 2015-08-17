using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Vectrosity;

public class ObservedObjectButton1 : ObservedObjectBox1 {
  [SerializeField] protected Material lineMaterial;
  [SerializeField] protected float lineWidth;
  [SerializeField] protected Color lineColor;

  [System.NonSerialized] public ObservedObjectBox1 box;

  protected override void Initialize() { 
    if (initialized)
      return;

    _line = new VectorLine(transform.name + "Line", new List<Vector2>(), lineMaterial, lineWidth);
    _line.SetColor(lineColor);
    _line.useViewportCoords = false;

    base.Initialize();
  }

  private VectorLine _line;

  public VectorLine line {
    get {
      if (!initialized)
        Initialize();

      return _line;
    }
  }

  public override Vector3 position {
    get {
      if (!initialized)
        Initialize();

      rectTransform.GetWorldCorners(corners);

      /*if( corners.Length > 2 )
      {
        return ( corners[0] + corners[2] ) * 0.5f;
      }*/
      Vector3 center = Vector3.zero;

      if (corners.Length > 3) {
        center = corners[0];
        //center.x += 5f; // offset so the line reaches the button
        center.y = (corners[0].y + corners[1].y) * 0.5f;
      }

      return center;
    }
  }

  public override void Selection() {
    base.Selection();

    AudioManager.PlayOneShot(select);
  }

  public override void SetColor(Color color) {
    base.SetColor(color);

    line.SetColor(color);
  }
}
