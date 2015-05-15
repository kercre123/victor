using UnityEngine;
using System.Collections;

public class BuildActions : GameActions {


	[SerializeField] AudioClip actionDeniedSound;

	[SerializeField] AudioClip tooCloseVO;
	[SerializeField] AudioClip tooFarVO;
	[SerializeField] AudioClip tooHighVO;
	[SerializeField] AudioClip tooLowVO;
	[SerializeField] AudioClip wrongBlockVO;
	[SerializeField] AudioClip wrongStackVO;
	[SerializeField] AudioClip wrongColorVO;

	public override void Drop( bool onRelease, ObservedObject selectedObject )
	{
		if( robot == null || !onRelease ) return;
		if( robot.carryingObject == null ) return;

		//run validation prediction
		// if it will fail layout contraints, abort here and throw audio and text about why its a bad drop
		GameLayoutTracker tracker = GameLayoutTracker.instance;
		if(tracker != null) {

			Vector3 pos;
			float rad;
			if(tracker.AttemptAssistedDrop(robot.carryingObject, out pos, out rad)) {
				ActionButtonClick();
				robot.DropObjectAtPose(pos, rad);
				return;
			}
//			//drop location is one block forward and half a block up from robot's location
//			//  cozmo space up being Vector3.forward in unity
//			string error;
//			GameLayoutTracker.LayoutErrorType errorType;
//			if(!tracker.PredictDropValidation(robot.carryingObject, out error, out errorType)) {
//				Debug.Log("PredictDropValidation failed for robot.carryingObject("+robot.carryingObject+") error("+error+")");
//
//				//set error text message
//				tracker.SetMessage(error, Color.red);
//
//				PlaySoundsForErrorType(errorType);
//
//				return;
//			}
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
			GameLayoutTracker.LayoutErrorType errorType;
			if(!tracker.PredictStackValidation(robot.carryingObject, selectedObject, out error, out errorType)) {
				Debug.Log("PredictStackValidation failed for robot.carryingObject("+robot.carryingObject+") upon selectedObject("+selectedObject+") error("+error+")");
				//set error text message
				tracker.SetMessage(error, Color.red);

				PlaySoundsForErrorType(errorType);

				return;
			}
		}

		ActionButtonClick();
		base.Stack(onRelease, selectedObject);
	}

	void PlaySoundsForErrorType(GameLayoutTracker.LayoutErrorType errorType) {
	
		//play denied sound
		if(actionDeniedSound != null) audio.PlayOneShot(actionDeniedSound);

		AudioClip clip = null;
		switch(errorType) {
			case GameLayoutTracker.LayoutErrorType.NONE:
				break;
			case GameLayoutTracker.LayoutErrorType.TOO_CLOSE:
				clip = tooCloseVO;
				break;
			case GameLayoutTracker.LayoutErrorType.TOO_FAR:
				clip = tooFarVO;
				break;
			case GameLayoutTracker.LayoutErrorType.TOO_HIGH:
				clip = tooHighVO;
				break;
			case GameLayoutTracker.LayoutErrorType.TOO_LOW:
				clip = tooLowVO;
				break;
			case GameLayoutTracker.LayoutErrorType.WRONG_BLOCK:
				clip = wrongBlockVO;
				break;
			case GameLayoutTracker.LayoutErrorType.WRONG_STACK:
				clip = wrongStackVO;
				break;
			case GameLayoutTracker.LayoutErrorType.WRONG_COLOR:
				clip = wrongColorVO;
				break;
		}

		if(clip == null) return;

		//play VO for 'stack attempted on wrong block' or 'block is wrong color'
		StartCoroutine(PlaySoundAfterDelay(clip, actionDeniedSound != null ? actionDeniedSound.length : 0f));
	}


	IEnumerator PlaySoundAfterDelay (AudioClip clip, float delay) {
		yield return new WaitForSeconds(delay);
		if(clip != null) audio.PlayOneShot(clip);
	}

}
