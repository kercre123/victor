using UnityEngine;
using UnityEngine.UI;
using System.Collections;

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

	const string prefix_ID = "ID: ";
	const string prefix_headAngle = "HeadAngle: ";
	const string prefix_poseAngle = "PoseAngle: ";
	const string prefix_leftWheelSpeed = "L: ";
	const string prefix_rightWheelSpeed = "R: ";
	const string prefix_liftHeight = "LiftHeight: ";
	const string prefix_WorldPosition = "WorldPosition: ";
	const string prefix_status = "Status: ";
	const string prefix_batteryPercent = "Battery: ";
	const string prefix_carryingObjectID = "Carrying: ";
	const string prefix_headTracking = "HeadTracking: ";
	const string prefix_text_knownObjects = "KnownObjects: ";

	void Update() {
		if(RobotEngineManager.instance == null) return;

		Robot bot = RobotEngineManager.instance.current;
		if(bot == null) return;

		if(text_ID != null) 				text_ID.text 				= prefix_ID + bot.ID.ToString();
		if(text_headAngle != null) 			text_headAngle.text 		= prefix_headAngle + (bot.headAngle_rad * Mathf.Rad2Deg).ToString("N");
		if(text_poseAngle != null) 			text_poseAngle.text 		= prefix_poseAngle +(bot.poseAngle_rad * Mathf.Rad2Deg).ToString("N");
		if(text_leftWheelSpeed != null) 	text_leftWheelSpeed.text 	= prefix_leftWheelSpeed + bot.leftWheelSpeed_mmps.ToString("N");
		if(text_rightWheelSpeed != null) 	text_rightWheelSpeed.text	= prefix_rightWheelSpeed + bot.rightWheelSpeed_mmps.ToString("N");
		if(text_liftHeight != null) 		text_liftHeight.text 		= prefix_liftHeight + bot.liftHeight_mm.ToString("N");
		if(text_WorldPosition != null) 		text_WorldPosition.text 	= prefix_WorldPosition + bot.WorldPosition.ToString("N");
		if(text_status != null) 			text_status.text 			= prefix_status + bot.status;
		if(text_batteryPercent != null) 	text_batteryPercent.text 	= prefix_batteryPercent + bot.batteryPercent.ToString("P");
		if(text_carryingObjectID != null) 	text_carryingObjectID.text 	= prefix_carryingObjectID + bot.carryingObjectID.ToString();
		if(text_headTracking != null) 		text_headTracking.text 		= prefix_headTracking + bot.headTrackingObjectID.ToString();
		if(text_knownObjects != null) 		text_knownObjects.text 		= prefix_text_knownObjects + bot.knownObjects.Count.ToString();
	}
}
