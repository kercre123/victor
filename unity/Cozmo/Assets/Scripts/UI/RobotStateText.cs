using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

/// <summary>
/// used to display all the latest robot state information to screen
/// </summary>
public class RobotStateText : MonoBehaviour {

  [SerializeField] Text text_ID;
  [SerializeField] Text text_headAngle;
  [SerializeField] Text text_poseAngle;
  [SerializeField] Text text_leftWheelSpeed;
  [SerializeField] Text text_rightWheelSpeed;
  [SerializeField] Text text_liftHeight;
  [SerializeField] Text text_WorldPosition;
  [SerializeField] Text text_status;
  [SerializeField] Text text_batteryPercent;
  [SerializeField] Text text_carryingObjectID;
  [SerializeField] Text text_headTracking;
  [SerializeField] Text text_knownObjects;
  [SerializeField] Text text_searching;

  const string prefix_ID = "ID: ";
  const string prefix_headAngle = "Head Angle: ";
  const string prefix_poseAngle = "Pose Angle: ";
  const string prefix_leftWheelSpeed = "Left Speed: ";
  const string prefix_rightWheelSpeed = "Right Speed: ";
  const string prefix_liftHeight = "Lift Height: ";
  const string prefix_WorldPosition = "World Position: ";
  const string prefix_status = "Status: ";
  const string prefix_batteryPercent = "Battery: ";
  const string prefix_carryingObjectID = "Carrying: ";
  const string prefix_headTracking = "Head Tracking: ";
  const string prefix_text_knownObjects = "Known Objects: ";
  const string prefix_text_searching = "Searching: ";
  const string eol = "\n";
  const string moving = "MOVING";
  const string carrying = "CARRYING";
  const string manipulating = "MANIPULATING";
  const string pickedUP = "HELD";
  const string animating = "ANIMATING";
  const string acting = "ACTING";
  const string none = "none";
  const string mm = " mm";
  const string degree = "°";
  const string empty = "";

  List<RobotStatusFlag> statuses = new List<RobotStatusFlag>();

  byte lastID;
  float lastHeadAngle;
  float lastPoseAngle;
  float lastLeftWheelSpeed;
  float lastRightWheelSpeed;
  float lastLiftHeight;
  Vector3 lastWorldPosition;
  RobotStatusFlag lastStatus;
  float lastBatteryPercent;
  ObservedObject lastCarryingObjectID;
  ObservedObject lastHeadTracking;
  int lastKnownObjects;
  bool lastSearching;

  Robot bot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  void OnEnable() {
    if (bot == null)
      return;
    
    RefreshID();
    RefreshHeadAngle();
    RefreshPoseAngle();
    RefreshLeftWheelSpeed();
    RefreshRightWheelSpeed();
    RefreshLiftHeight();
    RefreshWorldPosition();
    RefreshStatus();
    RefreshBatteryPercent();
    RefreshCarryingObject();
    RefreshHeadTracking();
    RefreshKnownObjects();
    RefreshSearching();
  }

  void Update() {
    if (bot == null)
      return;

    if (DirtyID())
      RefreshID();
    if (DirtyHeadAngle())
      RefreshHeadAngle();
    if (DirtyPoseAngle())
      RefreshPoseAngle();
    if (DirtyLeftWheelSpeed())
      RefreshLeftWheelSpeed();
    if (DirtyRightWheelSpeed())
      RefreshRightWheelSpeed();
    if (DirtyLiftHeight())
      RefreshLiftHeight();
    if (DirtyWorldPosition())
      RefreshWorldPosition();
    if (DirtyStatus())
      RefreshStatus();
    if (DirtyBatteryPercent())
      RefreshBatteryPercent();
    if (DirtyCarryingObject())
      RefreshCarryingObject();
    if (DirtyHeadTracking())
      RefreshHeadTracking();
    if (DirtyKnownObjects())
      RefreshKnownObjects();
    if (DirtySearching())
      RefreshSearching();
  }

  bool DirtyID() {
    return lastID != bot.ID;
  }

  void RefreshID() {
    if (text_ID == null)
      return;
    text_ID.text = prefix_ID + bot.ID.ToString();
    lastID = bot.ID;
  }

  bool DirtyHeadAngle() {
    return lastHeadAngle != bot.headAngle_rad;
  }

  void RefreshHeadAngle() {
    if (text_headAngle == null)
      return;
    text_headAngle.text = prefix_headAngle + Mathf.RoundToInt(bot.headAngle_rad * Mathf.Rad2Deg).ToString() + degree;
    lastHeadAngle = bot.headAngle_rad;
  }

  bool DirtyPoseAngle() {
    return lastPoseAngle != bot.poseAngle_rad;
  }

  void RefreshPoseAngle() {
    if (text_poseAngle == null)
      return;
    text_poseAngle.text = prefix_poseAngle + Mathf.RoundToInt(bot.poseAngle_rad * Mathf.Rad2Deg).ToString() + degree;
    lastPoseAngle = bot.poseAngle_rad;
  }

  bool DirtyLeftWheelSpeed() {
    return lastLeftWheelSpeed != bot.leftWheelSpeed_mmps;
  }

  void RefreshLeftWheelSpeed() {
    if (text_leftWheelSpeed == null)
      return;
    text_leftWheelSpeed.text = prefix_leftWheelSpeed + Mathf.RoundToInt(bot.leftWheelSpeed_mmps).ToString() + mm;
    lastLeftWheelSpeed = bot.leftWheelSpeed_mmps;
  }

  bool DirtyRightWheelSpeed() {
    return lastRightWheelSpeed != bot.rightWheelSpeed_mmps;
  }

  void RefreshRightWheelSpeed() {
    if (text_rightWheelSpeed == null)
      return;
    text_rightWheelSpeed.text = prefix_rightWheelSpeed + Mathf.RoundToInt(bot.rightWheelSpeed_mmps).ToString() + mm;
    lastRightWheelSpeed = bot.rightWheelSpeed_mmps;
  }

  bool DirtyLiftHeight() {
    return lastLiftHeight != bot.liftHeight_mm;
  }

  void RefreshLiftHeight() {
    if (text_liftHeight == null)
      return;
    text_liftHeight.text = prefix_liftHeight + Mathf.RoundToInt(bot.liftHeight_mm).ToString() + mm;
    lastLiftHeight = bot.liftHeight_mm;
  }

  bool DirtyWorldPosition() {
    return lastWorldPosition != bot.WorldPosition;
  }

  void RefreshWorldPosition() {
    if (text_WorldPosition == null)
      return;
    text_WorldPosition.text = prefix_WorldPosition + bot.WorldPosition.ToString("N");
    lastWorldPosition = bot.WorldPosition;
  }

  void CheckSpecificStatus(RobotStatusFlag status) {
    
    bool statusActive = bot.Status(status);
    
    if (statuses.Contains(status) != statusActive) {
      if (statusActive) {
        statuses.Add(status);
      }
      else {
        statuses.Remove(status);
      }
    }
  }

  bool DirtyStatus() {
    return lastStatus != bot.status;
  }

  void RefreshStatus() {
    if (text_status == null)
      return;

    CheckSpecificStatus(RobotStatusFlag.IS_MOVING);
    CheckSpecificStatus(RobotStatusFlag.IS_CARRYING_BLOCK);
    CheckSpecificStatus(RobotStatusFlag.IS_PICKING_OR_PLACING);
    CheckSpecificStatus(RobotStatusFlag.IS_PICKED_UP);
    CheckSpecificStatus(RobotStatusFlag.IS_ANIMATING);
    CheckSpecificStatus(RobotStatusFlag.IS_PERFORMING_ACTION);

    string statusString = prefix_status + eol;

    for (int i = 0; i < statuses.Count; i++) {
      statusString += statuses[i].ToString() + eol;
    }

    text_status.text = statusString;

    lastStatus = bot.status;
  }

  bool DirtyBatteryPercent() {
    return lastBatteryPercent != bot.batteryPercent;
  }

  void RefreshBatteryPercent() {
    if (text_batteryPercent == null)
      return;
    text_batteryPercent.text = prefix_batteryPercent + bot.batteryPercent.ToString("P0");
    lastBatteryPercent = bot.batteryPercent;
  }

  bool DirtyCarryingObject() {
    return lastCarryingObjectID != bot.carryingObject;
  }

  void RefreshCarryingObject() {
    if (text_carryingObjectID == null)
      return;
    text_carryingObjectID.text = prefix_carryingObjectID + bot.carryingObject;
    lastCarryingObjectID = bot.carryingObject;
  }

  bool DirtyHeadTracking() {
    return lastHeadTracking != bot.headTrackingObject;
  }

  void RefreshHeadTracking() {
    if (text_headTracking == null)
      return;
    if (bot.headTrackingObject == -1) {
      text_headTracking.text = prefix_headTracking + none;
    }
    else {
      text_headTracking.text = prefix_headTracking + bot.headTrackingObject;
    }
    lastHeadTracking = bot.headTrackingObject;
  }

  bool DirtyKnownObjects() {
    return lastKnownObjects != bot.knownObjects.Count;
  }

  void RefreshKnownObjects() {
    if (text_knownObjects == null)
      return;
    text_knownObjects.text = prefix_text_knownObjects + bot.knownObjects.Count.ToString();
    lastKnownObjects = bot.knownObjects.Count;
  }

  bool DirtySearching() {
    return lastSearching != bot.searching;
  }

  void RefreshSearching() {
    if (text_searching == null)
      return;
    text_searching.text = prefix_text_searching + bot.searching.ToString();
    lastSearching = bot.searching;
  }
}
