using System;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft.Json;
using System.Linq;

public class GraphEvaluator2d {

  // By Default Vector2 serializes with a bunch of extra crap. This just serializes to x,y
  public struct Point2 {
    public float x;
    public float y;

    public Point2(float x, float y) {
      this.x = x;
      this.y = y;
    }

    public static implicit operator Point2(Vector2 vector) {
      return new Point2(vector.x, vector.y);
    }

    public static implicit operator Vector2(Point2 point) {
      return new Vector2(point.x, point.y);
    }

    public static Point2 operator+(Point2 a, Point2 b) {
      return new Point2(a.x + b.x, a.y + b.y);
    }
    public static Point2 operator-(Point2 a, Point2 b) {
      return new Point2(a.x - b.x, a.y - b.y);
    }
    public static Point2 operator-(Point2 a) {
      return new Point2(-a.x, -a.y);
    }
  }

  [JsonProperty("nodes")]
  public List<Point2> Nodes = new List<Point2>();

  public GraphEvaluator2d() {
  }


  public static implicit operator GraphEvaluator2d(AnimationCurve curve) {
    GraphEvaluator2d graph = new GraphEvaluator2d();
    graph.Nodes.AddRange(curve.keys.Select(x => new Point2(x.time, x.value)));
    return graph;
  }

  public static implicit operator AnimationCurve(GraphEvaluator2d graph) {
    AnimationCurve curve = new AnimationCurve();

    if (graph.Nodes.Count == 0) {
      return curve;
    }

    curve.AddKey(new Keyframe(graph.Nodes[0].x, graph.Nodes[0].y));
    for (int i = 1; i < graph.Nodes.Count; i++) {
      Keyframe keyA = curve.keys[i - 1];
      Keyframe keyB = new Keyframe(graph.Nodes[i].x, graph.Nodes[i].y);

      var delta = (graph.Nodes[i] - graph.Nodes[i - 1]);
      keyA.outTangent = keyB.inTangent = delta.y / delta.x;

      curve.MoveKey(i - 1, keyA);
      curve.AddKey(keyB);
    }

    return curve;
  }
}

