using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

public enum CubeType {
  UNKNOWN = -1,
  LIGHT_CUBE,
  BULLS_EYE,
  FLAG,
  FACE,
  Count
}

/// <summary>
/// all objects that cozmo sees are transmitted across to unity and represented here as ObservedObjects
///   so far, we only both handling three types of cubes and the occasional human head
/// </summary>
public class ObservedObject {
  public CubeType cubeType { get; private set; }

  public uint RobotID { get; private set; }

  public ObjectFamily Family { get; private set; }

  public ObjectType ObjectType { get; private set; }

  public int ID { get; private set; }

  public bool MarkersVisible { get { return Time.time - TimeLastSeen < 0.25f; } }

  public Rect VizRect { get; private set; }

  public Vector3 WorldPosition { get; private set; }

  public Quaternion Rotation { get; private set; }

  public Vector3 Forward { get { return Rotation * Vector3.right; } }

  public Vector3 Right { get { return Rotation * -Vector3.up; } }

  public Vector3 Up { get { return Rotation * Vector3.forward; } }

  public Vector3 TopNorth { get { return Quaternion.AngleAxis(TopFaceNorthAngle * Mathf.Rad2Deg, Vector3.forward) * Vector2.right; } }

  public Vector3 TopEast { get { return Quaternion.AngleAxis(TopFaceNorthAngle * Mathf.Rad2Deg, Vector3.forward) * -Vector2.up; } }

  public Vector3 TopNorthEast { get { return (TopNorth + TopEast).normalized; } }

  public Vector3 TopSouthEast { get { return (-TopNorth + TopEast).normalized; } }

  public Vector3 TopSouthWest { get { return (-TopNorth - TopEast).normalized; } }

  public Vector3 TopNorthWest { get { return (TopNorth - TopEast).normalized; } }

  public float TopFaceNorthAngle { get; private set; }

  public Vector3 Size { get; private set; }

  public float TimeLastSeen { get; private set; }

  public float TimeCreated { get; private set; }

  protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  public bool isActive { get { return cubeType == CubeType.LIGHT_CUBE; } }

  public bool isFace { get { return cubeType == CubeType.FACE; } }

  public event Action<ObservedObject> OnDelete;

  public static Action SignificantChangeDetected = null;

  public float targetingScore {
    get {
      float score = -1f;

      if (robot.carryingObject != ID) {
        float distanceFromRobot = Vector2.Distance(robot.WorldPosition + robot.Forward * CozmoUtil.CARRIED_OBJECT_HORIZONTAL_OFFSET, WorldPosition) - (CozmoUtil.BLOCK_LENGTH_MM * 0.5f);
        float angleFromRobot = Vector2.Angle(robot.Forward, WorldPosition - robot.WorldPosition);
        float maxDistanceInMM = maxDistanceInCubeLengths * CozmoUtil.BLOCK_LENGTH_MM;

        if (maxDistanceInMM > distanceFromRobot && maxAngle > angleFromRobot) {
          float distanceScore = (100f - Mathf.Lerp(0f, 100f, distanceFromRobot / maxDistanceInMM)) * distanceScoreMultiplier;
          float angleScore = (100f - Mathf.Lerp(0f, 100f, angleFromRobot / maxAngle)) * angleScoreMultiplier;

          score = distanceScore + angleScore;
        }
      }

      return score;
    }
  }

  private static float angleScoreMultiplier = 1f;
  private static float distanceScoreMultiplier = 1f;
  private static float maxAngle = 45f;
  private static float maxDistanceInCubeLengths = 8f;

  public bool canBeStackedOn {
    get {
      if (robot.selectedObjects.Count > 1)
        return false; // if blocks stacked on each other

      //float distance = ( (Vector2)WorldPosition - (Vector2)robot.WorldPosition ).magnitude;
      float height = Mathf.Abs(WorldPosition.z - robot.WorldPosition.z);
    
      return /*distance <= CozmoUtil.BLOCK_LENGTH_MM * 4f &&*/ height < CozmoUtil.BLOCK_LENGTH_MM;
    }
  }

  public const float RemoveDelay = 0.15f;

  public string InfoString { get; private set; }

  public string SelectInfoString { get; private set; }

  public ObservedObject() {
  }

  public ObservedObject(int objectID, ObjectFamily objectFamily, ObjectType objectType) {
    Constructor(objectID, objectFamily, objectType);
  }

  protected void Constructor(int objectID, ObjectFamily objectFamily, ObjectType objectType) {
    TimeCreated = Time.time;
    Family = objectFamily;
    ObjectType = objectType;
    ID = objectID;
    
    InfoString = "ID: " + ID + " Family: " + Family + " Type: " + ObjectType;
    SelectInfoString = "Select ID: " + ID + " Family: " + Family + " Type: " + ObjectType;

    if (objectFamily == ObjectFamily.LightCube) {
      cubeType = CubeType.LIGHT_CUBE;
    }
    else if (objectFamily == ObjectFamily.HumanHead) {
      cubeType = CubeType.FACE;
      Debug.LogWarning("FACE " + ID + " !!!"); 
    }
    else if (objectType == ObjectType.Block_BULLSEYE2 || objectType == ObjectType.Block_BULLSEYE2_INVERTED) {
      cubeType = CubeType.BULLS_EYE;
    }
    else if (objectType == ObjectType.Block_FLAG || objectType == ObjectType.Block_FLAG2 || objectType == ObjectType.Block_FLAG_INVERTED) {
      cubeType = CubeType.FLAG;
    }
    else {
      cubeType = CubeType.UNKNOWN;
      Debug.LogWarning("Object " + ID + " with type " + objectType + " is unsupported"); 
    }

    DAS.Debug("ObservedObject", "ObservedObject cubeType(" + cubeType + ") from objectFamily(" + objectFamily + ") objectType(" + objectType + ")");

    SetTargetingScoreSettings();
  }

  public static implicit operator uint(ObservedObject observedObject) {
    if (observedObject == null) {
      Debug.LogWarning("converting null ObservedObject into uint: returning uint.MaxValue");
      return uint.MaxValue;
    }
    
    return (uint)observedObject.ID;
  }

  public static implicit operator int(ObservedObject observedObject) {
    if (observedObject == null)
      return -1;

    return observedObject.ID;
  }

  public static implicit operator string(ObservedObject observedObject) {
    return ((int)observedObject).ToString();
  }

  public void UpdateInfo(G2U.RobotObservedObject message) {
    RobotID = message.robotID;
    VizRect = new Rect(message.img_topLeft_x, message.img_topLeft_y, message.img_width, message.img_height);

    Vector3 newPos = new Vector3(message.world_x, message.world_y, message.world_z);

    //dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
    WorldPosition = newPos;
    Rotation = new Quaternion(message.quaternion1, message.quaternion2, message.quaternion3, message.quaternion0);
    Size = Vector3.one * CozmoUtil.BLOCK_LENGTH_MM;

    TopFaceNorthAngle = message.topFaceOrientation_rad + Mathf.PI * 0.5f;

    if (message.markersVisible > 0)
      TimeLastSeen = Time.time;
  }

  public virtual void Delete() {
    if (OnDelete != null)
      OnDelete(this);
    if (SignificantChangeDetected != null)
      SignificantChangeDetected();
  }

  public Vector2 GetBestFaceVector(Vector3 initialVector) {
    
    Vector2[] faces = { TopNorth, TopEast, -TopNorth, -TopEast };
    Vector2 face = faces[0];
    
    float bestDot = -float.MaxValue;
    
    for (int i = 0; i < 4; i++) {
      float dot = Vector2.Dot(initialVector, faces[i]);
      if (dot > bestDot) {
        bestDot = dot;
        face = faces[i];
      }
    }
    
    return face;
  }

  public static void RefreshAngleScoreMultiplier() {
    angleScoreMultiplier = OptionsScreen.GetAngleScoreMultiplier();
  }

  public static void RefreshDistanceScoreMultiplier() {
    distanceScoreMultiplier = OptionsScreen.GetDistanceScoreMultiplier();
  }

  public static void RefreshMaxAngle() {
    maxAngle = OptionsScreen.GetMaxAngle();
  }

  public static void RefreshMaxDistanceInCubeLengths() {
    maxDistanceInCubeLengths = OptionsScreen.GetMaxDistanceInCubeLengths();
  }

  private void SetTargetingScoreSettings() {
    RefreshAngleScoreMultiplier();
    RefreshDistanceScoreMultiplier();
    RefreshMaxAngle();
    RefreshMaxDistanceInCubeLengths();
  }
}
