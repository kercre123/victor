using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;

public enum ActionButtonMode {
	DISABLED,
	TARGET,
	PICK_UP,
	DROP,
	STACK,
	ROLL,
	ALIGN,
	CHANGE,
	CANCEL,
	NUM_MODES
}


[System.Serializable]
public class ActionButton
{
	public Button button;
	public Image image;
	public Text text;

	private int index;
	private CozmoVision vision;
	
	public void ClaimOwnership(CozmoVision vision) {
		this.vision = vision;
	}

	protected void PickAndPlaceObject()
	{
		RobotEngineManager.instance.current.PickAndPlaceObject( index );
	}
	
	public void Cancel()
	{
		Debug.Log( "Cancel" );
		
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.current.selectedObjects.Clear();
			RobotEngineManager.instance.current.SetHeadAngle();
		}
	}

	public void SetMode(ActionButtonMode mode, int i = 0) {
		
		if(mode == ActionButtonMode.DISABLED) {
			button.gameObject.SetActive(false);
			button.onClick.RemoveAllListeners();
			return;
		}
		
		image.sprite = vision.actionSprites[(int)mode];
		index = i;
		
		switch(mode) {
			case ActionButtonMode.TARGET:
				text.text = "Target";
				//button.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.PICK_UP:
				text.text = "Pick Up";
				button.onClick.AddListener(PickAndPlaceObject);
				break;
			case ActionButtonMode.DROP:
				text.text = "Drop";
				button.onClick.AddListener(RobotEngineManager.instance.current.PlaceObjectOnGroundHere);
				break;
			case ActionButtonMode.STACK:
				text.text = "Stack";
				button.onClick.AddListener(PickAndPlaceObject);
				break;
			case ActionButtonMode.ROLL:
				text.text = "Roll";
				//button.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.ALIGN:
				text.text = "Align";
				button.onClick.AddListener(RobotEngineManager.instance.current.PlaceObjectOnGroundHere);
				break;
			case ActionButtonMode.CHANGE:
				text.text = "Change";
				//button.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.CANCEL:
				text.text = "Cancel";
				button.onClick.AddListener(Cancel);
				break;
		}
		
		button.gameObject.SetActive(true);
	}

	public static string GetModeName(ActionButtonMode mode) {

		switch(mode) {
			case ActionButtonMode.TARGET: return "Search";
			case ActionButtonMode.PICK_UP: return "Pick Up";
			case ActionButtonMode.DROP: return "Drop";
			case ActionButtonMode.STACK: return "Stack";
			case ActionButtonMode.ROLL: return "Roll";
			case ActionButtonMode.ALIGN: return "Align";
			case ActionButtonMode.CHANGE: return "Change";
			case ActionButtonMode.CANCEL: return "Cancel";
		}

		return "None";
	}
}

public class CozmoVision : MonoBehaviour
{
	[SerializeField] protected Image image;
	[SerializeField] protected Text text;
	[SerializeField] protected ActionButton[] actionButtons;
	[SerializeField] protected int maxObservedObjects;
	[SerializeField] protected Slider headAngleSlider;
	[SerializeField] protected AudioClip newObjectObservedSound;
	[SerializeField] protected AudioClip objectObservedLostSound;
	[SerializeField] protected float soundDelay = 2f;
	[SerializeField] public Sprite[] actionSprites = new Sprite[(int)ActionButtonMode.NUM_MODES];

	public UnityAction[] actions;

	protected RectTransform rTrans;
	protected Rect rect;
	protected Robot robot;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );
	protected float lastHeadAngle = 0f;
	protected List<int> lastObservedObjects = new List<int>();

	private float[] dingTimes = new float[2] { 0f, 0f };

	protected int observedObjectsCount
	{
		get
		{
			if( RobotEngineManager.instance.current.observedObjects.Count < maxObservedObjects )
			{
				return RobotEngineManager.instance.current.observedObjects.Count;
			}
			else
			{
				return maxObservedObjects;
			}
		}
	}

	protected virtual void Reset( DisconnectionReason reason = DisconnectionReason.None )
	{
		for( int i = 0; i < dingTimes.Length; ++i )
		{
			dingTimes[i] = 0f;
		}

		lastObservedObjects.Clear();
		lastHeadAngle = 0f;
		robot = null;
	}

	protected virtual void Awake()
	{
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.DisconnectedFromClient += Reset;

		rTrans = transform as RectTransform;

		foreach(ActionButton button in actionButtons) button.ClaimOwnership(this);
	}

	protected void DisableButtons() {
		for(int i=0; i<actionButtons.Length; i++) actionButtons[i].SetMode(ActionButtonMode.DISABLED);
	}

	protected virtual void SetActionButtons()
	{
		DisableButtons();
		robot = RobotEngineManager.instance.current;
		if(robot == null || robot.isBusy) return;

		if(robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) {
			if(robot.selectedObjects.Count > 0) actionButtons[0].SetMode(ActionButtonMode.STACK);
			actionButtons[1].SetMode(ActionButtonMode.DROP);
		}
		else {
			if(robot.selectedObjects.Count > 0) actionButtons[0].SetMode(ActionButtonMode.PICK_UP);
		}

		if(robot.selectedObjects.Count > 0) actionButtons[2].SetMode(ActionButtonMode.CANCEL);

//		for( int i = 0; i < actionButtons.Length; ++i )
//		{
//			actionButtons[i].button.gameObject.SetActive( ( i == 0 && robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK && robot.selectedObject == -1 ) || robot.selectedObject > -1 );
//			
//			if( i == 0 )
//			{
//				if( robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK )
//				{
//					if( robot.selectedObject > -1 )
//					{
//						actionButtons[i].image.sprite = sprite_stack;
//						actionButtons[i].text.text = "Stack " + robot.carryingObjectID + " on " + robot.selectedObject;
//					}
//					else
//					{
//						actionButtons[i].image.sprite = sprite_drop;
//						actionButtons[i].text.text = "Drop " + robot.carryingObjectID;
//					}
//				}
//				else
//				{
//					actionButtons[i].image.sprite = sprite_pickUp;
//					actionButtons[i].text.text = "Pick Up " + robot.selectedObject;
//				}
//			}
//		}
	}

	private void RobotImage( Texture2D texture )
	{
		if( rect.height != texture.height || rect.width != texture.width )
		{
			rect = new Rect( 0, 0, texture.width, texture.height );
		}

		image.sprite = Sprite.Create( texture, rect, pivot );

		if( text.gameObject.activeSelf )
		{
			text.gameObject.SetActive( false );
		}
	}

	public void ForceDropBox()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.current.SetRobotCarryingObject();
		}
	}

	public void ForceClearAll()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.current.ClearAllBlocks();
		}
	}

	public void RequestImage()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.current.RequestImage();
			RobotEngineManager.instance.current.SetHeadAngle();
			RobotEngineManager.instance.current.SetLiftHeight( 0f );
		}
	}

	protected virtual void OnEnable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage += RobotImage;
		}

		Reset();
		RequestImage();
		ResizeToScreen();

		if(RobotEngineManager.instance != null && RobotEngineManager.instance.current.headAngle_rad != lastHeadAngle) {
			headAngleSlider.value = Mathf.Clamp(lastHeadAngle * Mathf.Rad2Deg, -90f, 90f);
			lastHeadAngle = RobotEngineManager.instance.current.headAngle_rad;
		}
	}

	protected virtual void Dings()
	{
		if( RobotEngineManager.instance != null )
		{
			robot = RobotEngineManager.instance.current;

			if( robot == null || robot.isBusy || robot.selectedObjects.Count > 0 )
			{
				return;
			}

			if( robot.observedObjects.Count > lastObservedObjects.Count )
			{
				Ding( true );
			}
			else if( robot.observedObjects.Count < lastObservedObjects.Count )
			{
				Ding( false );
			}
		}
	}

	protected void Ding( bool found )
	{
		if( found )
		{
			if( dingTimes[0] + soundDelay < Time.time )
			{
				audio.PlayOneShot( newObjectObservedSound );
				
				dingTimes[0] = Time.time;
			}
		}
		else
		{
			if( dingTimes[1] + soundDelay < Time.time )
			{
				audio.PlayOneShot( objectObservedLostSound );
				
				dingTimes[1] = Time.time;
			}
		}
	}

	protected virtual void LateUpdate()
	{
		if( RobotEngineManager.instance != null )
		{
			lastObservedObjects.Clear();

			for( int i = 0; i < RobotEngineManager.instance.current.observedObjects.Count; ++i )
			{
				lastObservedObjects.Add( RobotEngineManager.instance.current.observedObjects[i].ID );
			}
		}
	}

	private void OnDisable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage -= RobotImage;
		}
	}

	private void ResizeToScreen()
	{
		if( rTrans == null )
		{
			return;
		}

//		RectTransform parentT = rTrans.parent as RectTransform;
//		Vector3[] corners = new Vector3[4];
//		parentT.GetWorldCorners(corners);
//
//		float w = corners[0] - corners[2];
//
//		float scale = ( w * 0.5f ) / 320f;
//		rTrans.localScale = Vector3.one * scale;
	}

	public void OnHeadAngleSliderReleased() {
		lastHeadAngle = headAngleSlider.value * Mathf.Deg2Rad;
		if(RobotEngineManager.instance != null && RobotEngineManager.instance.current != null) {
			RobotEngineManager.instance.current.SetHeadAngle(lastHeadAngle);
		}
		Debug.Log("OnHeadAngleSliderReleased lastHeadAngle("+lastHeadAngle+") headAngleSlider.value("+headAngleSlider.value+")");
	}

}
