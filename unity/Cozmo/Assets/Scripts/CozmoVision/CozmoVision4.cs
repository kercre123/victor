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

	public Image action1FrameSmall;
	public Image action2FrameSmall;
	public Image action1FrameBig;

	public Text text_action1Small;
	public Text text_action2Small;
	public Text text_action1Big;

	CozmoVision vision;
	bool pressed = false;

	public ActionButtonMode Mode { get; private set; }
	
	public void ClaimOwnership(CozmoVision vision) {
		this.vision = vision;
	}
	
	public void SetMode(ActionButtonMode mode, bool down, List<ActionButtonMode> modes=null) {
		//if(Mode == mode && down == pressed) return;

		//Debug.Log("ActionSlider.SetMode("+mode+", "+down+") modes("+(modes != null ? modes.Count.ToString() : "null")+")");
		
		Mode = mode;
		pressed = down;

		if(mode == ActionButtonMode.DISABLED) {
			slider.gameObject.SetActive(false);
			//slider.onClick.RemoveAllListeners();
			return;
		}
		
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
						text.text = "Searching";
					}
				}
				break;
		}

		if(modes == null || modes.Count <= 1) {
			action1FrameSmall.gameObject.SetActive(false);
			action2FrameSmall.gameObject.SetActive(false);
			action1FrameBig.gameObject.SetActive(false);
			text_action1Small.gameObject.SetActive(false);
			text_action2Small.gameObject.SetActive(false);
			text_action1Big.gameObject.SetActive(false);
		}
		else if(modes.Count == 2) {
			action1FrameSmall.gameObject.SetActive(false);
			action2FrameSmall.gameObject.SetActive(false);
			action1FrameBig.gameObject.SetActive(true);
			text_action1Small.gameObject.SetActive(false);
			text_action2Small.gameObject.SetActive(false);

			text_action1Big.text = ActionButton.GetModeName(modes[1]);
			text_action1Big.gameObject.SetActive(true);
		}
		else if(modes.Count >= 3) {
			action1FrameSmall.gameObject.SetActive(true);
			action2FrameSmall.gameObject.SetActive(true);

			text_action1Small.text = ActionButton.GetModeName(modes[1]);
			text_action2Small.text = ActionButton.GetModeName(modes[2]);

			text_action1Small.gameObject.SetActive(true);
			text_action2Small.gameObject.SetActive(true);

			action1FrameBig.gameObject.SetActive(false);
			text_action1Big.gameObject.SetActive(false);
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

	protected override void OnEnable()
	{
		base.OnEnable();

		Reset();
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
			return;
		}

		interactLastFrame = true;

		Dings();

		if((robot.selectedObjects.Count == 0 || targetLockTimer <= 0) && !robot.isBusy) AcquireTarget();

		RefreshSliderMode();

		//if we have a new target, let's see if our current slider mode wants to initiate an interaction
		if(robot.selectedObjects.Count > 0 && !robot.isBusy) {

			if(lastMode != actionSlider.Mode || robot.selectedObjects.Count != robot.lastSelectedObjects.Count ||
			   robot.selectedObjects[0] != robot.lastSelectedObjects[0] ) {

				InitiateAssistedInteraction();
			}
		}

		robot.lastSelectedObjects.Clear();
		robot.lastSelectedObjects.AddRange( robot.selectedObjects );
	}

	Vector2 centerViz = new Vector2(160f, 120f);

	private void AcquireTarget() {
		ObservedObject nearestToCoz = null;
		ObservedObject nearestToVizCenter = null;

		float bestDistFromCoz = float.MaxValue;
		float bestDistFromCenterViz = float.MaxValue;

		for(int i=0; i<robot.observedObjects.Count; i++) {
			float distFromCoz = (robot.observedObjects[i].WorldPosition - robot.WorldPosition).sqrMagnitude;
			if(distFromCoz < bestDistFromCoz) {
				bestDistFromCoz = distFromCoz;
				nearestToCoz = robot.observedObjects[i];
			}

			float distFromCenterViz = (robot.observedObjects[i].VizRect.center - centerViz).sqrMagnitude;
			if(distFromCenterViz < bestDistFromCenterViz) {
				bestDistFromCenterViz = distFromCenterViz;
				nearestToVizCenter = robot.observedObjects[i];
			}
		}

		ObservedObject best = nearestToVizCenter;
		if(nearestToCoz != null && nearestToCoz != best) {
			//Debug.Log("AcquireTarget found nearer object than the one closest to center view.");
		}

		robot.selectedObjects.Clear();

		if(best != null) {
			//Debug.Log("AcquireTarget " + best.ID);
			robot.selectedObjects.Add( best );
		}
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

		if(robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) {
			modes.Add(ActionButtonMode.DROP);
			if(robot.selectedObjects.Count > 0) modes.Add(ActionButtonMode.STACK);
		}
		else if(robot.selectedObjects.Count > 0) {
			modes.Add(ActionButtonMode.PICK_UP);
			modes.Add(ActionButtonMode.ROLL);
		}
	
		ActionButtonMode currentMode = modes[0];
	
		if(actionSlider.slider.value > 0.8f && modes.Count > 2) {
			currentMode = modes[2];
		}
		else if(actionSlider.slider.value > 0.2f && modes.Count > 1) {
			currentMode = modes[1];
		}
	
		actionSlider.SetMode(currentMode, interactPressed, modes);
	}

	private void InitiateAssistedInteraction() {
		switch(actionSlider.Mode) {
			case ActionButtonMode.TARGET:
				break;
			case ActionButtonMode.PICK_UP:
				RobotEngineManager.instance.PickAndPlaceObject( 0, false, true );
				targetLockTimer = 1f;
				break;
			case ActionButtonMode.DROP:
				break;
			case ActionButtonMode.STACK:
				RobotEngineManager.instance.PickAndPlaceObject( 0, false, true );
				break;
			case ActionButtonMode.ROLL:
				//RobotEngineManager.instance.PickAndPlaceObject( 0, false, true );
				break;
			case ActionButtonMode.ALIGN:
				//RobotEngineManager.instance.PickAndPlaceObject( 0, false, true );
				break;
			case ActionButtonMode.CHANGE:
				break;
		}
	}

	private void DoReleaseAction() {
		switch(actionSlider.Mode) {
			case ActionButtonMode.PICK_UP:
				RobotEngineManager.instance.PickAndPlaceObject();
				break;
			case ActionButtonMode.DROP:
				RobotEngineManager.instance.PlaceObjectOnGroundHere();
				break;
			case ActionButtonMode.STACK:
				RobotEngineManager.instance.PickAndPlaceObject();
				break;
			case ActionButtonMode.ROLL:
				//RobotEngineManager.instance.PickAndPlaceObject();
				break;
			case ActionButtonMode.ALIGN:
				RobotEngineManager.instance.PlaceObjectOnGroundHere();
				break;
			case ActionButtonMode.CHANGE:
				RobotEngineManager.instance.PickAndPlaceObject();
				break;
		}
	}
}
