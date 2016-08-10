using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

/// <summary>
/// unity representation of cozmo's LightCubes
///   adds functionality for controlling the four LEDs
///   adds awareness of accelerometer messages to detect LightCube movements
/// </summary>
public class LightCube : ObservedObject {
  #region LightCube helpers

  public static bool TryFindCubesFurthestApart(List<LightCube> cubesToCompare, out LightCube cubeA, out LightCube cubeB) {
    bool success = false;
    cubeA = null;
    cubeB = null;
    if (cubesToCompare.Count >= 2) {
      float longestDistanceSquared = -1f;
      float distanceSquared = -1f;
      Vector3 distanceVector;
      // Check 0->1, 0->2, 0->3... 0->n then check 1->2, 1->3,...1->n all the way to (n-1)->n
      // Distance checks are communicable so there's no use checking 0->1 and 1->0
      for (int rootCube = 0; rootCube < cubesToCompare.Count - 1; rootCube++) {
        for (int otherCube = rootCube + 1; otherCube < cubesToCompare.Count; otherCube++) {
          distanceVector = cubesToCompare[rootCube].WorldPosition - cubesToCompare[otherCube].WorldPosition;
          distanceSquared = distanceVector.sqrMagnitude;
          if (distanceSquared > longestDistanceSquared) {
            longestDistanceSquared = distanceSquared;
            cubeA = cubesToCompare[rootCube];
            cubeB = cubesToCompare[otherCube];
          }
        }
      }
      success = true;
    }
    else {
      DAS.Error("CozmoUtil", string.Format("GetCubesFurthestApart: cubesToCompare has less than 2 cubes! cubesToCompare.Count: {0}",
        cubesToCompare.Count));
      if (cubesToCompare.Count == 1) {
        cubeA = cubesToCompare[0];
      }
    }
    return success;
  }

  #endregion

  public UpAxis UpAxis { get; private set; }

  public UpAxis LastAxisOfAccel { get; private set; }

  public float XAccel { get; private set; }

  public float YAccel { get; private set; }

  public float ZAccel { get; private set; }

  public event Action<LightCube> OnAxisChange;

  public override string ReticleLabelLocKey { get { return LocalizationKeys.kDroneModeCubeReticleLabel; } }
  public override string ReticleLabelStringArg { get { return ID.ToString(); } }

  /// <summary>
  /// TappedAction<ID, Tap Count, Timestamp>.
  /// </summary>
  public static Action<int, int, float> TappedAction;
  public static Action<int, float, float, float> OnMovedAction;
  public static Action<int> OnStoppedAction;

  public LightCube(int objectID, uint factoryID, ObjectFamily objectFamily, ObjectType objectType) : base(objectID, factoryID, objectFamily, objectType) {

    UpAxis = UpAxis.Unknown;
    XAccel = byte.MaxValue;
    YAccel = byte.MaxValue;
    ZAccel = byte.MaxValue;
  }

  public override void HandleStartedMoving(ObjectMoved message) {
    base.HandleStartedMoving(message);

    LastAxisOfAccel = message.axisOfAccel;
    XAccel = message.accel.x;
    YAccel = message.accel.y;
    ZAccel = message.accel.z;

    if (OnMovedAction != null) {
      OnMovedAction(ID, XAccel, YAccel, ZAccel);
    }
  }

  public override void HandleStoppedMoving(ObjectStoppedMoving message) {
    base.HandleStoppedMoving(message);

    if (OnStoppedAction != null) {
      OnStoppedAction(ID);
    }
  }

  public override void HandleUpAxisChanged(ObjectUpAxisChanged message) {
    base.HandleUpAxisChanged(message);

    UpAxis = message.upAxis;
    if (OnAxisChange != null)
      OnAxisChange(this);
  }

  public override void HandleObjectTapped(ObjectTapped message) {
    base.HandleObjectTapped(message);
    DAS.Debug(this, "Tapped Message Received for LightCube(" + ID + "): " + message.numTaps + " taps");
    if (TappedAction != null)
      TappedAction(ID, message.numTaps, message.timestamp);
  }
}
