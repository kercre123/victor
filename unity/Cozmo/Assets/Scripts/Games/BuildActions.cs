using UnityEngine;
using System.Collections;

/// <summary>
/// these are cozmo's action overrides for the building phases of gameplay
/// </summary>
public class BuildActions : GameActions
{

	#region INSPECTOR FIELDS

	[SerializeField] AudioClip actionDeniedSound;
	[SerializeField] AudioClip tooCloseVO;
	[SerializeField] AudioClip tooFarVO;
	[SerializeField] AudioClip tooHighVO;
	[SerializeField] AudioClip tooLowVO;
	[SerializeField] AudioClip wrongBlockVO;
	[SerializeField] AudioClip wrongStackVO;
	[SerializeField] AudioClip wrongColorVO;

	#endregion

	public override string STACK { get { return "Stack"; } }

	public override string ALIGN { get { return "Play!"; } }

	protected override void _SetActionButtons(bool isSlider) // 0 is bottom button, 1 is top button, 2 is center button
	{
		if(robot.isBusy)
		{
			buttons[2].SetMode(ActionButton.Mode.CANCEL, null, null, true);
			return;
		}

		if(GameLayoutTracker.instance != null && GameLayoutTracker.instance.Phase == LayoutTrackerPhase.COMPLETE)
		{
			buttons[2].SetMode(ActionButton.Mode.ALIGN, null, null, true);
			return;
		}

		if(robot.Status(RobotStatusFlag.IS_CARRYING_BLOCK))
		{
			//stack is overwritten to be our assisted place command
			if(robot.selectedObjects.Count > 0 && robot.selectedObjects[0].canBeStackedOn)
			{
				buttons[1].SetMode(ActionButton.Mode.STACK, robot.selectedObjects[0]);
			}

			//still allow normal dropping
			buttons[0].SetMode(ActionButton.Mode.DROP, null);
			
		} else
		{
			if(buttons.Length > 1 && robot.selectedObjects.Count == 1)
			{
				buttons[1].SetMode(ActionButton.Mode.PICK_UP, robot.selectedObjects[0]);
			} else
			{
				for(int i = 0; i < robot.selectedObjects.Count && i < 2 && i < buttons.Length; ++i)
				{
					buttons[i].SetMode(ActionButton.Mode.PICK_UP, robot.selectedObjects[i], i == 0 ? BOTTOM : TOP);
				}
			}
		}
		
		if(buttons.Length > 2)
		{
			if(isSlider)
			{
				buttons[2].SetMode(ActionButton.Mode.TARGET, null, null, true);
			} else if(robot.selectedObjects.Count > 0)
			{
				buttons[2].SetMode(ActionButton.Mode.CANCEL, null);
			}
		}
	}

	public override void PickUp(bool onRelease, ObservedObject selectedObject)
	{
		if(!onRelease || robot == null) return;
		
		GameLayoutTracker tracker = GameLayoutTracker.instance;
		if(tracker != null)
		{
			tracker.SetNextObjectToPlace(selectedObject);
		}

		base.PickUp(onRelease, selectedObject);
	}

	public override void Drop(bool onRelease, ObservedObject selectedObject)
	{
		if(robot == null || !onRelease) return;

		bool approveStandardDrop = true;
		//run validation prediction
		// if it will fail layout contraints, abort here and throw audio and text about why its a bad drop
		GameLayoutTracker tracker = GameLayoutTracker.instance;
		if(tracker != null)
		{

			//drop location is one block forward and half a block up from robot's location
			//  cozmo space up being Vector3.forward in unity
			string error;
			LayoutErrorType errorType;

			if(!tracker.PredictDropValidation(robot.carryingObject, out error, out errorType, out approveStandardDrop))
			{
				Debug.Log("PredictDropValidation failed for robot.carryingObject(" + robot.carryingObject + ") error(" + error + ")");

				if(!approveStandardDrop)
				{
					//set error text message
					tracker.SetMessage(error, Color.red);

					PlaySoundsForErrorType(errorType);

					return;
				}
			} else if(tracker.AttemptAssistedPlacement())
			{
				ActionButtonClick();
				return;
			}

		}

		//this isn't a block that belongs on the ground, but we'll let the user drop it in case they want to switch to another block

		base.Drop(onRelease, selectedObject);
	}

	public override void Stack(bool onRelease, ObservedObject selectedObject)
	{
		if(robot == null || !onRelease) return;
		if(robot.carryingObject == null) return;

		//run validation prediction
		// if it will fail layout contraints, abort here and throw audio and text about why its a bad drop
		GameLayoutTracker tracker = GameLayoutTracker.instance;
		if(tracker != null)
		{
			
			//drop location is one block forward and half a block up from robot's location
			//  cozmo space up being Vector3.forward in unity
			string error;
			LayoutErrorType errorType;
			if(!tracker.PredictStackValidation(robot.carryingObject, selectedObject, out error, out errorType, true))
			{
				Debug.Log("PredictStackValidation failed for robot.carryingObject(" + robot.carryingObject + ") error(" + error + ")");
				
				//set error text message
				tracker.SetMessage(error, Color.red);
				
				PlaySoundsForErrorType(errorType);
				
				return;
			}
		}

		ActionButtonClick();
		base.Stack(onRelease, selectedObject);
	}

	public override void Align(bool onRelease, ObservedObject selectedObject)
	{
		if(robot == null || !onRelease) return;

		GameLayoutTracker tracker = GameLayoutTracker.instance;
		if(tracker != null)
		{
			tracker.StartGame();
		}

	}

	public override void Cancel(bool onRelease, ObservedObject selectedObject)
	{

		if(GameLayoutTracker.instance.Phase == LayoutTrackerPhase.AUTO_BUILDING)
		{
			//cancel during auto build switches to manual build
			GameLayoutTracker.instance.StartBuild();
		}

		base.Cancel(onRelease, selectedObject);
	}

	protected void PlaySoundsForErrorType(LayoutErrorType errorType)
	{
	
		//play denied sound
		if(actionDeniedSound != null) AudioManager.PlayAudioClip(actionDeniedSound);

		AudioClip clip = null;
		switch(errorType)
		{
			case LayoutErrorType.NONE:
				break;
			case LayoutErrorType.TOO_CLOSE:
				clip = tooCloseVO;
				break;
			case LayoutErrorType.TOO_FAR:
				clip = tooFarVO;
				break;
			case LayoutErrorType.TOO_HIGH:
				clip = tooHighVO;
				break;
			case LayoutErrorType.TOO_LOW:
				clip = tooLowVO;
				break;
			case LayoutErrorType.WRONG_BLOCK:
				clip = wrongBlockVO;
				break;
			case LayoutErrorType.WRONG_STACK:
				clip = wrongStackVO;
				break;
			case LayoutErrorType.WRONG_COLOR:
				clip = wrongColorVO;
				break;
		}

		if(clip == null) return;

		//play VO for 'stack attempted on wrong block' or 'block is wrong color'
		AudioManager.PlayAudioClip(clip, actionDeniedSound != null ? actionDeniedSound.length : 0f);
	}
}
