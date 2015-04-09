using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;


[System.Serializable]
public class ActionSlider {
	public Slider slider;
	public Image image;
	public Text text;
	public GameObject highlight;
	public Image image_bottomAction;
	public Text text_bottomAction;
	public Image image_topAction;
	public Text text_topAction;
	
	ActionSliderPanel vision;
	public bool Pressed { get; private set; }

	public ActionButtonMode Mode { get; private set; }
	
	public void ClaimOwnership(ActionSliderPanel vision) {
		this.vision = vision;
	}
	
	public void SetMode(ActionButtonMode mode, bool down, ActionButtonMode topAction=ActionButtonMode.TARGET, ActionButtonMode bottomAction=ActionButtonMode.TARGET) {
		//Debug.Log("ActionSlider.SetMode("+mode+", "+down+") modes("+(modes != null ? modes.Count.ToString() : "null")+") index("+selectedIndex+")");
		
		Mode = mode;
		Pressed = down;
		
		if(mode == ActionButtonMode.DISABLED) {
			//slider.gameObject.SetActive(false);
			//slider.onClick.RemoveAllListeners();
			return;
		}
		
		highlight.SetActive(Pressed);
		
		text.gameObject.SetActive(Pressed);
		
		image.sprite = vision.GetActionSprite(mode);
		text.text = ActionButton.GetModeName(mode);
		
		switch(mode) {
			case ActionButtonMode.TARGET:
				if(Pressed) {
					if(		RobotEngineManager.instance != null
					   && 	RobotEngineManager.instance.current != null
					   &&	RobotEngineManager.instance.current.selectedObjects.Count > 0) {
						
						//dmd2do get prop family name from observedobject
						text.text = "Cube " + RobotEngineManager.instance.current.selectedObjects[0].ID;
					}
					else {
						text.text = "Target";
					}
				}
				break;
		}
		
		if(bottomAction != ActionButtonMode.TARGET) {
			image_bottomAction.sprite = vision.GetActionSprite(bottomAction);
			
			if(bottomAction == ActionButtonMode.PICK_UP && topAction == ActionButtonMode.PICK_UP) {
				text_bottomAction.text = ActionButton.GetModeName(bottomAction) + " Bottom";
			}
			else {
				text_bottomAction.text = ActionButton.GetModeName(bottomAction);
			}
			
			image_bottomAction.gameObject.SetActive(true);
			text_bottomAction.gameObject.SetActive(true);
		}
		else {
			image_bottomAction.gameObject.SetActive(false);
			text_bottomAction.gameObject.SetActive(false);
		}
		
		if(topAction != ActionButtonMode.TARGET) {
			image_topAction.sprite = vision.GetActionSprite(topAction);
			
			if(bottomAction == ActionButtonMode.PICK_UP && bottomAction == ActionButtonMode.PICK_UP) {
				text_topAction.text = ActionButton.GetModeName(topAction) + " Top";
			}
			else {
				text_topAction.text = ActionButton.GetModeName(topAction);
			}
			
			image_topAction.gameObject.SetActive(true);
			text_topAction.gameObject.SetActive(true);
		}
		else {
			image_topAction.gameObject.SetActive(false);
			text_topAction.gameObject.SetActive(false);
		}
		
		//slider.gameObject.SetActive(true);
	}
	
}

public class ActionSliderPanel : ActionPanel
{

	[SerializeField] public ActionSlider actionSlider = null;
	[SerializeField] protected AudioClip slideInSound;
	[SerializeField] protected AudioClip slideOutSound;

	public bool Engaged { get { return actionSlider != null ? actionSlider.Pressed : false; } }


	bool interactLastFrame = false;
	ActionButtonMode lastMode = ActionButtonMode.DISABLED;

	protected override void Awake() {
		base.Awake();
		
		actionSlider.ClaimOwnership(this);
		actionSlider.slider.value = 0f;
		actionSlider.SetMode(ActionButtonMode.TARGET, false);
	}
	
	protected void Update() {
		lastMode = actionSlider.Mode;

		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			return;
		}
		
		robot = RobotEngineManager.instance.current;

		
		if(!actionSlider.Pressed) {
			
			if(interactLastFrame) {
				DoReleaseAction();
			}

			robot.selectedObjects.Clear();
			interactLastFrame = false;
			
			actionSlider.SetMode(ActionButtonMode.TARGET, false);

			return;
		}

		RefreshSliderMode();
		
		//if we have a new target, let's see if our current slider mode wants to initiate an interaction
		if(robot.selectedObjects.Count > 0 && !robot.isBusy) {
			
			if(lastMode != actionSlider.Mode || robot.selectedObjects.Count != robot.lastSelectedObjects.Count ||
			   robot.selectedObjects[0] != robot.lastSelectedObjects[0] ) {
				
				InitiateAssistedInteraction();
			}
		}
			
		interactLastFrame = true;
		
		//Debug.Log("CozmoVision4.Update_2 selectedObjects("+robot.selectedObjects.Count+") isBusy("+robot.isBusy+")");
	}

	public void ToggleInteract(bool val) {
		actionSlider.SetMode(actionSlider.Mode, val);
		
		if(!val) {
			actionSlider.slider.value = 0f;
		}
		
		//Debug.Log("ToggleInteract("+val+")");
	}
	
	private void RefreshSliderMode() {
		
		ActionButtonMode currentMode = ActionButtonMode.TARGET;
		ActionButtonMode topAction = ActionButtonMode.TARGET;
		ActionButtonMode bottomAction = ActionButtonMode.TARGET;
		
		if(robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) {
			bottomAction = ActionButtonMode.DROP;
			if(robot.selectedObjects.Count == 1) topAction = ActionButtonMode.STACK;
		}
		else if(robot.selectedObjects.Count > 1) {
			bottomAction = ActionButtonMode.PICK_UP;
			topAction = ActionButtonMode.PICK_UP;
			//Debug.Log("RefreshSliderMode double pick up, set index2("+index2+")");
		}
		else if(robot.selectedObjects.Count == 1) {
			//bottomAction = ActionButtonMode.ROLL;
			topAction = ActionButtonMode.PICK_UP;
		}
		
		ObservedObject targeted = robot.selectedObjects.Count > 0 ? robot.selectedObjects[0] : null;
		
		if(actionSlider.slider.value < -0.5f && bottomAction != ActionButtonMode.TARGET) {
			currentMode = bottomAction;
			
			float minZ = float.MaxValue;
			for(int i=0;i<robot.selectedObjects.Count && i<2;i++) {
				if(minZ < robot.selectedObjects[i].WorldPosition.z) continue;
				minZ = robot.selectedObjects[i].WorldPosition.z;
				targeted = robot.selectedObjects[i];
			}
		}
		else if(actionSlider.slider.value > 0.5f && topAction != ActionButtonMode.TARGET) {
			currentMode = topAction;
			
			float maxZ = float.MinValue;
			for(int i=0;i<robot.selectedObjects.Count && i<2;i++) {
				if(maxZ > robot.selectedObjects[i].WorldPosition.z) continue;
				maxZ = robot.selectedObjects[i].WorldPosition.z;
				targeted = robot.selectedObjects[i];
			}
			
			//Debug.Log("RefreshSliderMode index = index2("+index2+")");
		}
		
		//if necessary switch our primary targetLock to the target of this action...
		if(robot.selectedObjects.Count > 0 && targeted != null && targeted != robot.selectedObjects[0]) {
			robot.selectedObjects.Remove(targeted);
			robot.selectedObjects.Insert(0, targeted);
		}
		
		if(currentMode != lastMode && currentMode != ActionButtonMode.TARGET) {
			SlideInSound();
		}
		else if(currentMode != lastMode) {
			SlideOutSound();
		}
		//Debug.Log("RefreshSliderMode currentMode("+currentMode+")");
		actionSlider.SetMode(currentMode, actionSlider.Pressed, topAction, bottomAction);
	}
	
	private void InitiateAssistedInteraction() {
		switch(actionSlider.Mode) {
			case ActionButtonMode.TARGET:
				//do auto-targeting here!
				break;
		}
	}
	
	private void DoReleaseAction() {
		if(robot == null) return;
		//Debug.Log("frame("+Time.frameCount+") DoReleaseAction targets(" + robot.selectedObjects.Count + ") from knownObjects("+robot.knownObjects.Count+")");

		//if(robot.selectedObjects.Count == 0) return;
		//		bool usePreDockPose = true;
		//
		//		//if marker is visable and we are roughly facing a side, let's not use the pre dock pose
		//		if(robot.selectedObjects.Count > actionSlider.selectedIndex && robot.selectedObjects[actionSlider.selectedIndex].MarkersVisible) {
		//			float angleDiff = Vector2.Angle(robot.Forward, robot.selectedObjects[actionSlider.selectedIndex].Forward);
		//			float modulo90 = Mathf.Abs(angleDiff % 90f);
		//			if(modulo90 > 45f) modulo90 = 90f - angleDiff;
		//			if(modulo90 < 20f) usePreDockPose = false;
		//
		//			//Debug.Log("usePreDockPose("+usePreDockPose+") angleDiff("+angleDiff+") modulo90("+modulo90+")" );
		//		}
		
		switch(actionSlider.Mode) {
			case ActionButtonMode.PICK_UP:
				RobotEngineManager.instance.current.PickAndPlaceObject();//, usePreDockPose, false);
				ActionButtonClick();
				break;
			case ActionButtonMode.DROP:
				RobotEngineManager.instance.current.PlaceObjectOnGroundHere();
				ActionButtonClick();
				break;
			case ActionButtonMode.STACK:
				RobotEngineManager.instance.current.PickAndPlaceObject();//, usePreDockPose, false);
				ActionButtonClick();
				break;
			case ActionButtonMode.ROLL:
				//RobotEngineManager.instance.current.PickAndPlaceObject(actionSlider.selectedIndex);
				break;
			case ActionButtonMode.ALIGN:
				RobotEngineManager.instance.current.PlaceObjectOnGroundHere();
				break;
			case ActionButtonMode.CHANGE:
				RobotEngineManager.instance.current.PickAndPlaceObject();
				break;
		}
	}

	public void SlideInSound()
	{
		audio.volume = 1f;
		audio.PlayOneShot( slideInSound, 1f );
	}
	
	public void SlideOutSound()
	{
		audio.volume = 1f;
		audio.PlayOneShot( slideOutSound, 1f );
	}

}
