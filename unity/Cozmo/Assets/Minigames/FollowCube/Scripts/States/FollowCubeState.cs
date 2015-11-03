using UnityEngine;
using System.Collections;

public class FollowCubeState : State {

  private float distanceMin_ = 30.0f;
  private float distanceMax_ = 40.0f;

  public override void Enter() {
    base.Update();
  }

  public override void Update() {
    base.Update();
    FollowClosest();
  }

  void FollowClosest() {
    Robot robot = stateMachine_.GetGame().robot;
    ObservedObject closest = null;
    float dist = float.MaxValue;
    foreach (ObservedObject obj in robot.VisibleObjects) {
      float d = Vector3.Distance(robot.WorldPosition, obj.WorldPosition);
      if (d < dist) {
        dist = d;
        closest = obj;
      }
    }
    if (closest != null) {
      
    }
  }
}
