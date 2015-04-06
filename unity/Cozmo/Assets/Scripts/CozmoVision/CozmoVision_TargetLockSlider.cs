using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;

[System.Serializable]
public class ActionSlider {
	public Slider slider;
	public Image image;
	public Text text;
	public GameObject highlight;
	public Image image_action1;
	public Image image_action2;
	public Text text_action1;
	public Text text_action2;

	CozmoVision vision;
	bool pressed = false;


	public ActionButtonMode Mode { get; private set; }
	
	public void ClaimOwnership(CozmoVision vision) {
		this.vision = vision;
	}

	public void SetMode(ActionButtonMode mode, bool down, ActionButtonMode topAction=ActionButtonMode.TARGET, ActionButtonMode bottomAction=ActionButtonMode.TARGET) {
		//Debug.Log("ActionSlider.SetMode("+mode+", "+down+") modes("+(modes != null ? modes.Count.ToString() : "null")+") index("+selectedIndex+")");
		
		Mode = mode;
		pressed = down;

		if(mode == ActionButtonMode.DISABLED) {
			//slider.gameObject.SetActive(false);
			//slider.onClick.RemoveAllListeners();
			return;
		}

		highlight.SetActive(pressed);

		text.gameObject.SetActive(pressed);

		image.sprite = vision.GetActionSprite(mode);
		text.text = ActionButton.GetModeName(mode);

		switch(mode) {
			case ActionButtonMode.TARGET:
				if(pressed) {
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
			image_action1.sprite = vision.GetActionSprite(bottomAction);
			text_action1.text = ActionButton.GetModeName(bottomAction);
			image_action1.gameObject.SetActive(true);
			text_action1.gameObject.SetActive(true);
		}
		else {
			image_action1.gameObject.SetActive(false);
			text_action1.gameObject.SetActive(false);
		}

		if(topAction != ActionButtonMode.TARGET) {
			image_action2.sprite = vision.GetActionSprite(topAction);
			text_action2.text = ActionButton.GetModeName(topAction);
			image_action2.gameObject.SetActive(true);
			text_action2.gameObject.SetActive(true);
		}
		else {
			image_action2.gameObject.SetActive(false);
			text_action2.gameObject.SetActive(false);
		}

		//slider.gameObject.SetActive(true);
	}

}

public class CozmoVision_TargetLockSlider : CozmoVision {
	[SerializeField] ActionSlider actionSlider = null;
	[SerializeField] RectTransform targetLockReticle = null;
	[SerializeField] float targetLockReticleScale = 1.1f;

	float targetLockTimer = 0f;
	bool interactLastFrame = false;
	bool interactPressed=false;
	ActionButtonMode lastMode = ActionButtonMode.DISABLED;
	
	//Vector2 centerViz = new Vector2(160f, 120f);

	protected override void Reset( DisconnectionReason reason = DisconnectionReason.None ) {
		base.Reset( reason );

		targetLockTimer = 0f;
		interactLastFrame = false;
		lastMode = ActionButtonMode.DISABLED;

	}

	protected override void Awake() {
		base.Awake();

		actionSlider.ClaimOwnership(this);
		actionSlider.slider.value = 0f;
		actionSlider.SetMode(ActionButtonMode.TARGET, false);
		if(targetLockReticle != null) targetLockReticle.gameObject.SetActive(false);
	}

	protected void Update() {
		lastMode = actionSlider.Mode;
		if(targetLockTimer > 0) targetLockTimer -= Time.deltaTime;

		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		ShowObservedObjects();
		RefreshFade();

		if(!interactPressed) {

			if(interactLastFrame) { // && robot.selectedObject > -1
				DoReleaseAction();
			}

			targetLockTimer = 0f;
			interactLastFrame = false;

			actionSlider.SetMode(ActionButtonMode.TARGET, false);
			robot.selectedObjects.Clear();

			if(targetLockReticle != null) targetLockReticle.gameObject.SetActive(false);

			FadeOut();

			return;
		}

		FadeIn();

		bool targetingPropInHand = robot.selectedObjects.Count > 0 && robot.selectedObjects.Find( x => x.ID == robot.carryingObjectID) != null;
		if((targetingPropInHand || robot.selectedObjects.Count == 0) && !robot.isBusy) AcquireTarget();

		RefreshSliderMode();

		//if we have a new target, let's see if our current slider mode wants to initiate an interaction
		if(robot.selectedObjects.Count > 0 && !robot.isBusy) {

			if(lastMode != actionSlider.Mode || robot.selectedObjects.Count != robot.lastSelectedObjects.Count ||
			   robot.selectedObjects[0] != robot.lastSelectedObjects[0] ) {

				InitiateAssistedInteraction();
			}
		}

		if(targetLockReticle != null) {
			targetLockReticle.gameObject.SetActive(robot.selectedObjects.Count > 0);

			if(robot.selectedObjects.Count > 0) {

				float w = imageRectTrans.sizeDelta.x;
				float h = imageRectTrans.sizeDelta.y;
				ObservedObject lockedObject = robot.selectedObjects[0];

				float lockX = (lockedObject.VizRect.center.x / 320f) * w;
				float lockY = (lockedObject.VizRect.center.y / 240f) * h;
				float lockW = (lockedObject.VizRect.width / 320f) * w;
				float lockH = (lockedObject.VizRect.height / 240f) * h;
				
				targetLockReticle.sizeDelta = new Vector2( lockW, lockH ) * targetLockReticleScale;
				targetLockReticle.anchoredPosition = new Vector2( lockX, -lockY );
			}
		}

		interactLastFrame = true;

		//Debug.Log("CozmoVision4.Update_2 selectedObjects("+robot.selectedObjects.Count+") isBusy("+robot.isBusy+")");
	}

	private void AcquireTarget() {
		ObservedObject nearest = null;
		ObservedObject mostFacing = null;

		float bestDistFromCoz = float.MaxValue;
		float bestAngleFromCoz = float.MaxValue;
		Vector2 forward = robot.Forward;

		//List<ObservedObject> pertinentObjects = robot.knownObjects;
		List<ObservedObject> pertinentObjects = robot.observedObjects;


		for(int i=0; i<pertinentObjects.Count; i++) {
			if(robot.carryingObjectID == pertinentObjects[i].ID) continue;
			Vector2 atTarget = pertinentObjects[i].WorldPosition - robot.WorldPosition;

			float angleFromCoz = Vector2.Angle(forward, atTarget);
			if(angleFromCoz > 90f) continue;

			float distFromCoz = atTarget.sqrMagnitude;
			if(distFromCoz < bestDistFromCoz) {
				bestDistFromCoz = distFromCoz;
				nearest = pertinentObjects[i];
			}

			if(angleFromCoz < bestAngleFromCoz) {
				bestAngleFromCoz = angleFromCoz;
				mostFacing = pertinentObjects[i];
			}
		}

		ObservedObject best = mostFacing;
		if(nearest != null && nearest != best) {
			//Debug.Log("AcquireTarget found nearer object than the one closest to center view.");
			//float dist1 = (mostFacing.WorldPosition - robot.WorldPosition).sqrMagnitude;
			//if(bestDistFromCoz < dist1 * 0.5f) best = nearest;
		}

		robot.selectedObjects.Clear();

		if(best != null) {
			robot.selectedObjects.Add(best);

			//find any other objects in a 'stack' with our selected
			for(int i=0; i<pertinentObjects.Count; i++) {
				if(best == pertinentObjects[i])
					continue;
				if(robot.carryingObjectID == pertinentObjects[i].ID)
					continue;

				float dist = Vector2.Distance((Vector2)pertinentObjects[i].WorldPosition, (Vector2)best.WorldPosition);
				if(dist > best.Size.x * 0.5f) {
					//Debug.Log("AcquireTarget rejecting " + robot.knownObjects[i].ID +" because it is dist("+dist+") mm from best("+best.ID+") robot.carryingObjectID("+robot.carryingObjectID+")");
					continue;
				}

				robot.selectedObjects.Add(pertinentObjects[i]);
			}

			//sort selected from ground up
			robot.selectedObjects.Sort(( obj1, obj2 ) => {
				return obj1.WorldPosition.z.CompareTo(obj2.WorldPosition.z);   
			});
		}

		//Debug.Log("AcquireTarget targets(" + robot.selectedObjects.Count + ") from knownObjects("+robot.knownObjects.Count+")");
	}

	public void ToggleInteract(bool val) {
		interactPressed = val;
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

		actionSlider.SetMode(currentMode, interactPressed, topAction, bottomAction);
	}

	private void InitiateAssistedInteraction() {
		switch(actionSlider.Mode) {
			case ActionButtonMode.TARGET:
				//do auto-targeting here!
				break;
		}
	}

	private void DoReleaseAction() {

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

//	protected override void Dings()
//	{
//		if( robot != null )
//		{
//			if( !robot.isBusy && robot.selectedObjects.Count > 0/*robot.lastSelectedObjects.Count*/ )
//			{
//				Ding( true );
//			}
//			/*else if( robot.selectedObjects.Count < robot.lastSelectedObjects.Count )
//			{
//				Ding( false );
//			}*/
//		}
//	}
}
