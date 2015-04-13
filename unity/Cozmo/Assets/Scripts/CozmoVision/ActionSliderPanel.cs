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

	/*public Image image_bottomAction { get { return ActionPanel.instance.actionButtons[0].image; } }
	public Text text_bottomAction { get { return ActionPanel.instance.actionButtons[0].text; } }
	public Image image_topAction { get { return ActionPanel.instance.actionButtons[1].image; } }
	public Text text_topAction { get { return ActionPanel.instance.actionButtons[1].text; } }*/
	
	//ActionSliderPanel vision;
	public bool Pressed { get; private set; }

	public ActionButtonMode actionButtonMode { get; private set; }
	public ActionButton actionButton { get; private set; }
	
	/*public void ClaimOwnership(ActionSliderPanel vision) {
		this.vision = vision;
	}*/

	public void SetMode(ActionButtonMode mode, bool down, ActionButton button = null) {
		//Debug.Log("ActionSlider.SetMode("+mode+", "+down+") modes("+(modes != null ? modes.Count.ToString() : "null")+") index("+selectedIndex+")");
		actionButton = button;
		actionButtonMode = mode;
		Pressed = down;
		
		if(mode == ActionButtonMode.DISABLED) {
			//slider.gameObject.SetActive(false);
			//slider.onClick.RemoveAllListeners();
			return;
		}

		highlight.SetActive(Pressed);
		
		text.gameObject.SetActive(Pressed);
		
		image.sprite = ActionButton.GetModeSprite(mode);
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
	}
	
}

public class ActionSliderPanel : ActionPanel
{
	[SerializeField] public ActionSlider actionSlider = null;
	[SerializeField] protected AudioClip slideInSound;
	[SerializeField] protected AudioClip slideOutSound;

	public bool Engaged { get { return actionSlider != null ? actionSlider.Pressed : false; } }

	public ActionButton bottomAction { get { return ActionPanel.instance.actionButtons[0]; } }
	public ActionButton topAction { get { return ActionPanel.instance.actionButtons[1]; } }

	bool interactLastFrame = false;
	ActionButtonMode lastMode = ActionButtonMode.DISABLED;

	protected override void Awake() {
		base.Awake();
		
		//actionSlider.ClaimOwnership(this);
		actionSlider.slider.value = 0f;
		actionSlider.SetMode(ActionButtonMode.TARGET, false);
	}
	
	protected void Update() {
		lastMode = actionSlider.actionButtonMode;

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
			
			if(lastMode != actionSlider.actionButtonMode || robot.selectedObjects.Count != robot.lastSelectedObjects.Count ||
			   robot.selectedObjects[0] != robot.lastSelectedObjects[0] ) {
				
				InitiateAssistedInteraction();
			}
		}
			
		interactLastFrame = true;
		
		//Debug.Log("CozmoVision4.Update_2 selectedObjects("+robot.selectedObjects.Count+") isBusy("+robot.isBusy+")");
	}

	public void ToggleInteract(bool val) {
		actionSlider.SetMode(actionSlider.actionButtonMode, val, actionSlider.actionButton);
		
		if(!val) {
			actionSlider.slider.value = 0f;
		}
		
		//Debug.Log("ToggleInteract("+val+")");
	}
	
	private void RefreshSliderMode() {
		gameActions.SetActionButtons();

		ActionButton currentButton = null;
		ActionButtonMode currentMode = ActionButtonMode.TARGET;
		
		ObservedObject targeted = robot.selectedObjects.Count > 0 ? robot.selectedObjects[0] : null;

		if(actionSlider.slider.value < -0.5f && bottomAction.mode != ActionButtonMode.DISABLED) {
			currentButton = bottomAction;
			currentMode = bottomAction.mode;
			
			float minZ = float.MaxValue;
			for(int i=0;i<robot.selectedObjects.Count && i<2;i++) {
				if(minZ < robot.selectedObjects[i].WorldPosition.z) continue;
				minZ = robot.selectedObjects[i].WorldPosition.z;
				targeted = robot.selectedObjects[i];
			}
		}
		else if(actionSlider.slider.value > 0.5f && topAction.mode != ActionButtonMode.DISABLED) {
			currentButton = topAction;
			currentMode = topAction.mode;
			
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
		//Debug.Log("RefreshSliderMode currentMode("+currentMode+","+currentButton+")");
		actionSlider.SetMode(currentMode, actionSlider.Pressed, currentButton);
	}
	
	private void InitiateAssistedInteraction() {
		switch(actionSlider.actionButtonMode) {
			case ActionButtonMode.TARGET:
				//do auto-targeting here!
				break;
		}
	}
	
	private void DoReleaseAction() {
		if(actionSlider.actionButton != null) actionSlider.actionButton.button.onClick.Invoke();
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
