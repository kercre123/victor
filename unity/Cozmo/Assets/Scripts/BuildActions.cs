using UnityEngine;
using System.Collections;

public class BuildActions : GameActions {

	public override void Drop( bool onRelease, ObservedObject selectedObject )
	{
		if( robot == null || !onRelease ) return;

		//run validation prediction
		// if it will fail layout contraints, abort here and throw audio and text about why its a bad drop
		GameLayoutTracker tracker = GameLayoutTracker.instance;
		if(tracker != null) {
			Vector3 pos = robot.WorldPosition + robot.Forward * CozmoUtil.BLOCK_LENGTH_MM;
			string error;
			if(!tracker.PredictDropValidation(selectedObject, pos, out error)) {
				Debug.Log("PredictDropValidation failed for selectedObject("+selectedObject+") error("+error+")");

				//play error sound

				//set error text message

				//play VO for 'too close' or 'too far'


				return;
			}
		}

		ActionButtonClick();
		base.Drop(onRelease, selectedObject);
	}

	public override void Stack( bool onRelease, ObservedObject selectedObject )
	{
		if( robot == null || !onRelease ) return;

		//run validation prediction
		// if it will fail layout contraints, abort here and throw audio and text about why its a bad drop
		GameLayoutTracker tracker = GameLayoutTracker.instance;
		if(tracker != null) {
			string error;
			if(!tracker.PredictStackValidation(robot.carryingObject, selectedObject, out error)) {
				Debug.Log("PredictStackValidation failed for robot.carryingObject("+robot.carryingObject+") upon selectedObject("+selectedObject+") error("+error+")");
				
				//play error sound
				
				//set error text message
				
				//play VO for 'stack attempted on wrong block' or 'block is wrong color'

				return;
			}
		}

		ActionButtonClick();
		base.Stack(onRelease, selectedObject);
	}

}
