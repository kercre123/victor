using UnityEngine;
using System.Collections.Generic;

namespace Anki.Cozmo.Viz {

  public class MemoryMapNode {

    public MemoryMapNode(int depth, float size_m, Vector3 center) {
      Depth = depth;
      Size_m = size_m;
      Center = center;
      NextChild = 0;
    }

    private int Depth { get; set; }
    private float Size_m { get; set; }
    private Vector3 Center { get; set; }
    private int NextChild { get; set; }
    private List<MemoryMapNode> Children = new List<MemoryMapNode>();

    public bool AddChild(VizQuad vizQuad, ExternalInterface.ENodeContentTypeDebugVizEnum content, int depth) {
      if (depth == Depth) {
        // create color based on content
        Color color = Color.white;
        switch (content) {
        case ExternalInterface.ENodeContentTypeDebugVizEnum.Unknown: { color.r = 0.3f; color.g = 0.3f; color.b = 0.3f; color.a = 0.2f; break; }  // dark gray
        case ExternalInterface.ENodeContentTypeDebugVizEnum.ClearOfObstacle: { color = Color.green; color.a = 0.5f; break; }
        case ExternalInterface.ENodeContentTypeDebugVizEnum.ClearOfCliff: { color.r = 0.0f; color.g = 0.5f; color.b = 0.0f; color.a = 0.8f; break; }  // dark green
        case ExternalInterface.ENodeContentTypeDebugVizEnum.ObstacleCube: { color = Color.red; color.a = 0.5f; break; }
        case ExternalInterface.ENodeContentTypeDebugVizEnum.ObstacleCubeRemoved: { color = Color.white; color.a = 1.0f; break; } // not stored, it clears ObstacleCube
        case ExternalInterface.ENodeContentTypeDebugVizEnum.ObstacleCharger: { color.r = 1.0f; color.g = 0.5f; color.b = 0.0f; ; color.a = 0.5f; break; } // ORANGE
        case ExternalInterface.ENodeContentTypeDebugVizEnum.ObstacleChargerRemoved: { color = Color.white; color.a = 1.0f; break; } // not stored, it clears ObstacleCharge
        case ExternalInterface.ENodeContentTypeDebugVizEnum.ObstacleUnrecognized: { color = Color.magenta; color.a = 0.5f; break; }
        case ExternalInterface.ENodeContentTypeDebugVizEnum.Cliff: { color = Color.black; color.a = 0.8f; break; }
        case ExternalInterface.ENodeContentTypeDebugVizEnum.InterestingEdge: { color = Color.blue; color.a = 0.5f; break; }
        case ExternalInterface.ENodeContentTypeDebugVizEnum.NotInterestingEdge: { color = Color.yellow; color.a = 0.5f; break; }
        }
        float r = Size_m * 0.5f;
        var a = Center + new Vector3(-r, r, 0.0f);
        var b = Center + new Vector3(r, r, 0.0f);
        var c = Center + new Vector3(r, -r, 0.0f);
        var d = Center + new Vector3(-r, -r, 0.0f);
        vizQuad.AddToQuadList(a, b, c, d, color, Color.clear);
        return true;
      }

      if (Children.Count == 0) {
        int nextDepth = Depth - 1;
        float nextSize = Size_m * 0.5f;
        float offset = nextSize * 0.5f;
        var center1 = new Vector3(Center.x + offset, Center.y + offset, Center.z);
        var center2 = new Vector3(Center.x + offset, Center.y - offset, Center.z);
        var center3 = new Vector3(Center.x - offset, Center.y + offset, Center.z);
        var center4 = new Vector3(Center.x - offset, Center.y - offset, Center.z);

        Children.Add(new MemoryMapNode(nextDepth, nextSize, center1));
        Children.Add(new MemoryMapNode(nextDepth, nextSize, center2));
        Children.Add(new MemoryMapNode(nextDepth, nextSize, center3));
        Children.Add(new MemoryMapNode(nextDepth, nextSize, center4));
      }

      if (Children[NextChild].AddChild(vizQuad, content, depth)) {
        // All children below this child have been processed
        NextChild++;
      }

      return (NextChild > 3);
    }
  }
}
