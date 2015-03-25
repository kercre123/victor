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
	[SerializeField] protected AudioClip newObjectObservedSound;
	[SerializeField] protected AudioClip objectObservedLostSound;
	[SerializeField] protected float soundDelay = 2f;
	[SerializeField] public Sprite[] actionSprites = new Sprite[(int)ActionButtonMode.NUM_MODES];

	public UnityAction[] actions;

	protected RectTransform rTrans;
	protected RectTransform imageRectTrans;
	protected Canvas canvas;
	protected CanvasScaler canvasScaler;
	protected Rect rect;
	protected Robot robot;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );

	private float[] dingTimes = new float[2] { 0f, 0f };
	private static bool imageRequested = false;

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

		robot = null;
		imageRequested = false;
	}

	protected virtual void Awake()
	{
		rTrans = transform as RectTransform;
		imageRectTrans = image.gameObject.GetComponent<RectTransform>();
		canvas = GetComponentInParent<Canvas>();
		canvasScaler = canvas.gameObject.GetComponent<CanvasScaler>();

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
		if( !imageRequested && RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.current.RequestImage( RobotEngineManager.CameraResolution.CAMERA_RES_QVGA, RobotEngineManager.ImageSendMode_t.ISM_STREAM );
			RobotEngineManager.instance.current.SetHeadAngle();
			RobotEngineManager.instance.current.SetLiftHeight( 0f );
			imageRequested = true;
		}
	}

	protected virtual void OnEnable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage += RobotImage;
			RobotEngineManager.instance.DisconnectedFromClient += Reset;
		}

		RequestImage();
		ResizeToScreen();
	}

	protected virtual void Dings()
	{
		if( robot != null )
		{
			if( robot.isBusy || robot.selectedObjects.Count > 0 )
			{
				return;
			}

			if( robot.observedObjects.Count > 0/*lastObservedObjects.Count*/ )
			{
				Ding( true );
			}
			/*else if( robot.observedObjects.Count < lastObservedObjects.Count )
			{
				Ding( false );
			}*/
		}
	}

	protected void Ding( bool found )
	{
		if( found )
		{
			if( !audio.isPlaying && dingTimes[0] + soundDelay < Time.time )
			{
				audio.PlayOneShot( newObjectObservedSound );
				
				dingTimes[0] = Time.time;
			}
		}
		/*else
		{
			if( !audio.isPlaying && dingTimes[1] + soundDelay < Time.time )
			{
				audio.PlayOneShot( objectObservedLostSound );
				
				dingTimes[1] = Time.time;
			}
		}*/
	}

	protected virtual void LateUpdate()
	{
		if( robot != null )
		{
			robot.lastObservedObjects.Clear();
			robot.lastSelectedObjects.Clear();

			if( !robot.isBusy )
			{
				robot.lastObservedObjects.AddRange( robot.observedObjects );
				robot.lastSelectedObjects.AddRange( robot.selectedObjects );
			}
		}
	}

	private void OnDisable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage -= RobotImage;
			RobotEngineManager.instance.DisconnectedFromClient -= Reset;
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

}
