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
	
	public void SetMode(ActionButtonMode mode) {
		if(Mode == mode) return;
		
		Debug.Log("ActionSlider.SetMode("+mode+")");
		
		Mode = mode;
		
		if(mode == ActionButtonMode.DISABLED) {
			slider.gameObject.SetActive(false);
			//slider.onClick.RemoveAllListeners();
			return;
		}
		
		image.sprite = vision.actionSprites[(int)mode];
		
		switch(mode) {
			case ActionButtonMode.TARGET:
				text.text = "Searching";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.PICK_UP:
				text.text = "Pick Up";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.DROP:
				text.text = "Drop";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.STACK:
				text.text = "Stack";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.ROLL:
				text.text = "Roll";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.ALIGN:
				text.text = "Align";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.CHANGE:
				text.text = "Change";
				//slider.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.CANCEL:
				text.text = "Cancel";
				//slider.onClick.AddListener(vision.Cancel);
				break;
		}
		
		slider.gameObject.SetActive(true);
	}

}

public class CozmoVision4 : CozmoVision
{
	[SerializeField] ActionSlider actionSlider = null;

	int lastSelected = -1;
	float targetLockTimer = 0f;

	protected override void Awake()
	{
		base.Awake();

		actionSlider.ClaimOwnership(this);
		actionSlider.slider.value = 0f;
		actionSlider.SetMode(ActionButtonMode.TARGET);
	}

	protected void OnEnable() {
		lastSelected = -1;
	}

	protected void Update()
	{

		if(targetLockTimer > 0) targetLockTimer -= Time.deltaTime;

		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
			DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		if(!interact) {
			lastSelected = -1;
			return;
		}

		DetectObservedObjects();

		if(robot.selectedObject < 0 || targetLockTimer <= 0 && robot.status != Robot.StatusFlag.IS_PICKING_OR_PLACING) AcquireTarget();

		RefreshSliderMode();

		if(robot.selectedObject > -1 && lastSelected != robot.selectedObject) {
			InitiateAssistedInteraction();
		}

		lastSelected = robot.selectedObject;
	}

	Vector2 centerViz = new Vector2(160f, 120f);

	private void AcquireTarget() {

		if(robot.status == Robot.StatusFlag.IS_PICKING_OR_PLACING) {
			robot.selectedObject = -2;
			return;
		}

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
//		if(nearestToCoz != null && nearestToCoz != best) {
//
//
//
//		}
		robot.selectedObject = best != null ? best.ID : -1;
	}

	bool interact = false;
	public void ToggleInteract(bool val)
	{
		interact = val;

		if(!val) {
			actionSlider.slider.value = 0f;
			actionSlider.SetMode(ActionButtonMode.TARGET);
		}
	}

	private void RefreshSliderMode() {
	
		List<ActionButtonMode> modes = new List<ActionButtonMode>();
	
		if(robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK) {
			modes.Add(ActionButtonMode.DROP);
			if(robot.selectedObject > -1)
				modes.Add(ActionButtonMode.STACK);
		}
		else if(robot.selectedObject > -1) {
			modes.Add(ActionButtonMode.PICK_UP);
			modes.Add(ActionButtonMode.ROLL);
		}
		else {
			modes.Add(ActionButtonMode.TARGET);
		}
	
		ActionButtonMode currentMode = modes[0];
	
		if(actionSlider.slider.value > 0.25f && modes.Count > 1) {
			currentMode = modes[1];
		}
	
		actionSlider.SetMode(currentMode);
	}

	private void InitiateAssistedInteraction() {
		switch(actionSlider.Mode) {
			case ActionButtonMode.TARGET:
				break;
			case ActionButtonMode.PICK_UP:
				RobotEngineManager.instance.ManualPickAndPlaceObject();
				targetLockTimer = 1f;
				break;
			case ActionButtonMode.DROP:
				RobotEngineManager.instance.ManualPickAndPlaceObject();
				break;
			case ActionButtonMode.STACK:
				RobotEngineManager.instance.ManualPickAndPlaceObject();
				break;
			case ActionButtonMode.ROLL:
				RobotEngineManager.instance.ManualPickAndPlaceObject();
				break;
			case ActionButtonMode.ALIGN:
				RobotEngineManager.instance.ManualPickAndPlaceObject();
				break;
			case ActionButtonMode.CHANGE:
				RobotEngineManager.instance.ManualPickAndPlaceObject();
				break;
		}
	}
}
