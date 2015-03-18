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
	
	CozmoVision vision;
	bool pressed = false;

	public ActionButtonMode Mode { get; private set; }
	
	public void ClaimOwnership(CozmoVision vision) {
		this.vision = vision;
	}
	
	public void SetMode(ActionButtonMode mode, bool down) {
		if(Mode == mode && down == pressed) return;

		Debug.Log("ActionSlider.SetMode("+mode+", "+down+")");
		
		Mode = mode;
		pressed = down;

		if(mode == ActionButtonMode.DISABLED) {
			slider.gameObject.SetActive(false);
			//slider.onClick.RemoveAllListeners();
			return;
		}
		
		image.sprite = vision.actionSprites[(int)mode];
		
		switch(mode) {
			case ActionButtonMode.TARGET:
				text.text = pressed ? "Searching" : "Search";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.PICK_UP:
				text.text = pressed ? "Pick Up" : "Pick Up";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.DROP:
				text.text = pressed ? "Drop" : "Drop";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.STACK:
				text.text = pressed ? "Stack" : "Stack";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.ROLL:
				text.text = pressed ? "Roll" : "Roll";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.ALIGN:
				text.text = pressed ? "Align" : "Align";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.CHANGE:
				text.text = pressed ? "Change" : "Change";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.CANCEL:
				text.text = pressed ? "Cancel" : "Change";
				//slider.onClick.AddListener(vision.Cancel);
				break;
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

	protected void Update()
	{

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

		if(robot.selectedObjects.Count > 0 && !robot.isBusy && robot.selectedObjects != robot.lastSelectedObjects) {
			InitiateAssistedInteraction();
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
			robot.selectedObjects.Add( best.ID );
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
			if(robot.selectedObjects.Count > 0 && !robot.isBusy) modes.Add(ActionButtonMode.STACK);
		}
		else if(robot.selectedObjects.Count > 0 && !robot.isBusy) {
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
	
		actionSlider.SetMode(currentMode, interactPressed);
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
