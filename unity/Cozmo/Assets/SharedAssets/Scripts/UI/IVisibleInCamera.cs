using UnityEngine;

public delegate void VizRectChangedHandler(IVisibleInCamera reticleFocus, Rect newVizRect);
public interface IVisibleInCamera {
  Vector3 WorldPosition { get; }
  Rect VizRect { get; }
  bool IsInFieldOfView { get; }
  event VizRectChangedHandler OnVizRectChanged;
  string ReticleLabelLocKey { get; }
  string ReticleLabelStringArg { get; }
}
