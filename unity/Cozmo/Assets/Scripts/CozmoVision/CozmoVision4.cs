using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

[System.Serializable]
public class ActionSlider
{
	public Slider slider;
	public Image image;
	public Text text;

	public GameObject highlight;

	public Image image_action1;
	public Image image_action2;

	public Text text_action1;
	public Text text_action2;

	public int selectedIndex = 0;

	CozmoVision vision;
	bool pressed = false;

	public ActionButtonMode Mode { get; private set; }
	
	public void ClaimOwnership(CozmoVision vision) {
		this.vision = vision;
	}

	public void SetMode(ActionButtonMode mode, bool down) {
		SetMode(mode, down, null, selectedIndex);
	}

	public void SetMode(ActionButtonMode mode, bool down, List<ActionButtonMode> modes, int index=0) {


		//if(Mode == mode && down == pressed) return;

		//Debug.Log("ActionSlider.SetMode("+mode+", "+down+") modes("+(modes != null ? modes.Count.ToString() : "null")+") index("+selectedIndex+")");
		
		Mode = mode;
		pressed = down;
		selectedIndex = index;

		if(mode == ActionButtonMode.DISABLED) {
			slider.gameObject.SetActive(false);
			//slider.onClick.RemoveAllListeners();
			return;
		}


		highlight.SetActive(pressed);

		text.gameObject.SetActive(pressed);

		image.sprite = vision.actionSprites[(int)mode];
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

		if(modes == null || modes.Count <= 1) {
			image_action1.gameObject.SetActive(false);
			image_action2.gameObject.SetActive(false);
			text_action1.gameObject.SetActive(false);
			text_action2.gameObject.SetActive(false);

		}
		else if(modes.Count == 2) {
			image_action1.gameObject.SetActive(true);
			image_action2.gameObject.SetActive(false);
			text_action1.gameObject.SetActive(true);
			text_action2.gameObject.SetActive(false);

			image_action1.sprite = vision.actionSprites[(int)modes[1]];
			text_action1.text = ActionButton.GetModeName(modes[1]);
		}
		else if(modes.Count >= 3) {
			image_action1.gameObject.SetActive(true);
			image_action2.gameObject.SetActive(true);
			text_action1.gameObject.SetActive(true);
			text_action2.gameObject.SetActive(true);
			
			image_action1.sprite = vision.actionSprites[(int)modes[1]];
			text_action1.text = ActionButton.GetModeName(modes[1]);

			image_action2.sprite = vision.actionSprites[(int)modes[2]];
			text_action2.text = ActionButton.GetModeName(modes[2]);
		}

		slider.gameObject.SetActive(true);
	}

}

public class CozmoVision4 : CozmoVision
{
	[SerializeField] ActionSlider actionSlider = null;
	
	float targetLockTimer = 0f;
	bool interactLastFrame = false;

	protected override void Reset( DisconnectionReason reason = DisconnectionReason.None )
	{
		base.Reset( reason );

		targetLockTimer = 0f;
		interactLastFrame = false;
		lastMode = ActionButtonMode.DISABLED;

	}

	protected override void Awake()
	{
		base.Awake();

		actionSlider.ClaimOwnership(this);
		actionSlider.slider.value = 0f;
		actionSlider.SetMode(ActionButtonMode.TARGET, false);
	}

	ActionButtonMode lastMode = ActionButtonMode.DISABLED;
	protected void Update()
	{
		lastMode = actionSlider.Mode;
		if(targetLockTimer > 0) targetLockTimer -= Time.deltaTime;

		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		if(!interactPressed) {

			if(interactLastFrame) { // && robot.selectedObject > -1
				DoReleaseAction();
			}

			targetLockTimer = 0f;
			interactLastFrame = false;

			actionSlider.SetMode(ActionButtonMode.TARGET, false);
			robot.selectedObjects.Clear();
			return;
		}

		Dings();

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

		interactLastFrame = true;
	}

	//Vector2 centerViz = new Vector2(160f, 120f);

	private void AcquireTarget() {
		ObservedObject nearest = null;
		ObservedObject mostFacing = null;

		float bestDistFromCoz = float.MaxValue;
		float bestAngleFromCoz = float.MaxValue;
		Vector2 forward = robot.Forward;

		for(int i=0; i<robot.knownObjects.Count; i++) {
			if(robot.carryingObjectID == robot.knownObjects[i].ID) continue;
			Vector2 atTarget = robot.knownObjects[i].WorldPosition - robot.WorldPosition;

			float angleFromCoz = Vector2.Angle(forward, atTarget);
			//if(angleFromCoz > 90f) return;

			float distFromCoz = atTarget.sqrMagnitude;
			if(distFromCoz < bestDistFromCoz) {
				bestDistFromCoz = distFromCoz;
				nearest = robot.knownObjects[i];
			}

			if(angleFromCoz < bestAngleFromCoz) {
				bestAngleFromCoz = angleFromCoz;
				mostFacing = robot.knownObjects[i];
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
		}

		//find any other objects in a 'stack' with our selected
		for(int i=0; i<robot.knownObjects.Count; i++) {
			if(best == robot.knownObjects[i]) continue;
			if(robot.carryingObjectID == robot.knownObjects[i].ID) continue;

			float dist = Vector2.Distance((Vector2)robot.knownObjects[i].WorldPosition, (Vector2)best.WorldPosition);
			if(dist > best.Size.x * 0.5f) {
				//Debug.Log("AcquireTarget rejecting " + robot.knownObjects[i].ID +" because it is dist("+dist+") mm from best("+best.ID+") robot.carryingObjectID("+robot.carryingObjectID+")");
				continue;
			}

			robot.selectedObjects.Add(robot.knownObjects[i]);
		}

		//sort selected from ground up
		robot.selectedObjects.Sort( ( obj1, obj2 ) => {
			return obj1.WorldPosition.z.CompareTo( obj2.WorldPosition.z );   
		} );

		//Debug.Log("AcquireTarget targets(" + robot.selectedObjects.Count + ") from knownObjects("+robot.knownObjects.Count+")");
	}

	bool interactPressed = false;
	public void ToggleInteract(bool val)
	{
		interactPressed = val;
		actionSlider.SetMode(actionSlider.Mode, val);

		if(!val) {
			actionSlider.slider.value = 0f;
		}
	}

	List<ActionButtonMode> modes = new List<ActionButtonMode>();
	private void RefreshSliderMode() {
	
		modes.Clear();
		modes.Add(ActionButtonMode.TARGET);

		int index1 = 0;
		int index2 = 0;

		if(robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) {
			modes.Add(ActionButtonMode.DROP);
			if(robot.selectedObjects.Count == 1) modes.Add(ActionButtonMode.STACK);
		}
		else if(robot.selectedObjects.Count > 1) {
			modes.Add(ActionButtonMode.PICK_UP);
			modes.Add(ActionButtonMode.PICK_UP);
			index2 = 1;
			//Debug.Log("RefreshSliderMode double pick up, set index2("+index2+")");
		}
		else if(robot.selectedObjects.Count == 1) {
			modes.Add(ActionButtonMode.ROLL);
			modes.Add(ActionButtonMode.PICK_UP);
		}

		ActionButtonMode currentMode = modes[0];
		int index = index1;

		if(actionSlider.slider.value < -0.2f && modes.Count > 1) {
			currentMode = modes[1];
		}
		else if(actionSlider.slider.value > 0.2f && modes.Count > 2) {
			currentMode = modes[2];
			index = index2;
			//Debug.Log("RefreshSliderMode index = index2("+index2+")");
		}
	
		actionSlider.SetMode(currentMode, interactPressed, modes, index);
	}

	private void InitiateAssistedInteraction() {
		switch(actionSlider.Mode) {
			case ActionButtonMode.TARGET:
				//do auto-targeting here!
				break;
		}
	}

	private void DoReleaseAction() {
		switch(actionSlider.Mode) {
			case ActionButtonMode.PICK_UP:
				RobotEngineManager.instance.current.PickAndPlaceObject(actionSlider.selectedIndex);
				break;
			case ActionButtonMode.DROP:
				RobotEngineManager.instance.current.PlaceObjectOnGroundHere();
				break;
			case ActionButtonMode.STACK:
				RobotEngineManager.instance.current.PickAndPlaceObject(actionSlider.selectedIndex);
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

	protected override void Dings()
	{
		if( robot != null )
		{
			if( !robot.isBusy && robot.selectedObjects.Count > 0/*robot.lastSelectedObjects.Count*/ )
			{
				Ding( true );
			}
			/*else if( robot.selectedObjects.Count < robot.lastSelectedObjects.Count )
			{
				Ding( false );
			}*/
		}
	}
}
