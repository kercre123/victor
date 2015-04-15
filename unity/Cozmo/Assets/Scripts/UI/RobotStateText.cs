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
	[SerializeField] Text text_headTrackingObjectID;
	[SerializeField] Text text_knownObjects;

	void Update() {
		if(RobotEngineManager.instance == null) return;

		Robot bot = RobotEngineManager.instance.current;
		if(bot == null) return;

		if(text_ID != null) 					text_ID.text = "ID: " + bot.ID.ToString();
		if(text_headAngle != null) 				text_headAngle.text = "HeadAngle: "+(bot.headAngle_rad * Mathf.Rad2Deg).ToString("N");
		if(text_poseAngle != null) 				text_poseAngle.text = "PoseAngle: "+(bot.poseAngle_rad * Mathf.Rad2Deg).ToString("N");
		if(text_leftWheelSpeed != null) 		text_leftWheelSpeed.text = "L: " + bot.leftWheelSpeed_mmps.ToString("N");
		if(text_rightWheelSpeed != null) 		text_rightWheelSpeed.text = "R: " + bot.rightWheelSpeed_mmps.ToString("N");
		if(text_liftHeight != null) 			text_liftHeight.text = "LiftHeight(" + bot.liftHeight_mm.ToString("N");
		if(text_WorldPosition != null) 			text_WorldPosition.text = "L(" + bot.WorldPosition.ToString("N");
		if(text_status != null) 				text_status.text = "Status: " + bot.status;
		if(text_batteryPercent != null) 		text_batteryPercent.text = "Battery: " + bot.batteryPercent.ToString("P");
		if(text_carryingObjectID != null) 		text_carryingObjectID.text = "Carrying: " + bot.carryingObjectID.ToString();
		if(text_headTrackingObjectID != null) 	text_headTrackingObjectID.text = "HeadTracking: " + bot.headTrackingObjectID.ToString();
		if(text_knownObjects != null) 			text_knownObjects.text = "KnownObjects: " + bot.knownObjects.Count.ToString();
	}
}
