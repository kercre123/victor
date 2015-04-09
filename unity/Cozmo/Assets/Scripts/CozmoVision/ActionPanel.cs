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
	private ActionPanel panel;
	
	public void ClaimOwnership(ActionPanel panel) {
		this.panel = panel;
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
	
	public void SetMode( ActionButtonMode mode, int i = 0, string append = null  )
	{
		button.onClick.RemoveAllListeners();

		if(mode == ActionButtonMode.DISABLED) {
			button.gameObject.SetActive(false);
			//button.onClick.RemoveAllListeners();
			return;
		}
		
		image.sprite = panel.GetActionSprite(mode);
		index = i;

		switch(mode) {
			case ActionButtonMode.TARGET:
				text.text = TARGET;
				//button.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.PICK_UP:
				text.text = PICK_UP;
				button.onClick.AddListener(PickAndPlaceObject);
				button.onClick.AddListener(panel.ActionButtonClick);
				break;
			case ActionButtonMode.DROP:
				text.text = DROP;
				button.onClick.AddListener(RobotEngineManager.instance.current.PlaceObjectOnGroundHere);
				button.onClick.AddListener(panel.ActionButtonClick);
				break;
			case ActionButtonMode.STACK:
				text.text = STACK;
				button.onClick.AddListener(PickAndPlaceObject);
				button.onClick.AddListener(panel.ActionButtonClick);
				break;
			case ActionButtonMode.ROLL:
				text.text = ROLL;
				//button.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.ALIGN:
				text.text = ALIGN;
				button.onClick.AddListener(RobotEngineManager.instance.current.PlaceObjectOnGroundHere);
				button.onClick.AddListener(panel.ActionButtonClick);
				break;
			case ActionButtonMode.CHANGE:
				text.text = CHANGE;
				//button.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.CANCEL:
				text.text = CANCEL;
				button.onClick.AddListener(Cancel);
				button.onClick.AddListener(panel.CancelButtonClick);
				break;
		}

		if( append != null ) text.text += append;
		button.gameObject.SetActive(true);
	}


	private static string targetOverride = null;
	public static string TARGET
	{
		get { if( targetOverride == null ) { return "Search"; } return targetOverride; }
		
		set { targetOverride = value; }
	}

	private static string pickUpOverride = null;
	public static string PICK_UP
	{
		get { if( pickUpOverride == null ) { return "Pick Up"; } return pickUpOverride; }
		
		set { pickUpOverride = value; }
	}

	private static string dropOverride = null;
	public static string DROP
	{
		get { if( dropOverride == null ) { return "Drop"; } return dropOverride; }
		
		set { dropOverride = value; }
	}

	private static string stackOverride = null;
	public static string STACK
	{
		get { if( stackOverride == null ) { return "Stack"; } return stackOverride; }
		
		set { stackOverride = value; }
	}

	private static string rollOverride = null;
	public static string ROLL
	{
		get { if( rollOverride == null ) { return "Roll"; } return rollOverride; }
		
		set { rollOverride = value; }
	}

	private static string alignOverride = null;
	public static string ALIGN
	{
		get { if( alignOverride == null ) { return "Align"; } return alignOverride; }
		
		set { alignOverride = value; }
	}

	private static string changeOverride = null;
	public static string CHANGE
	{
		get { if( changeOverride == null ) { return "Change"; } return changeOverride; }
		
		set { changeOverride = value; }
	}

	private static string cancelOverride = null;
	public static string CANCEL
	{
		get { if( cancelOverride == null ) { return "Cancel"; } return cancelOverride; }
		
		set { cancelOverride = value; }
	}

	public static string GetModeName(ActionButtonMode mode) {
		
		switch(mode) {
			case ActionButtonMode.TARGET: return TARGET;
			case ActionButtonMode.PICK_UP: return PICK_UP;
			case ActionButtonMode.DROP: return DROP;
			case ActionButtonMode.STACK: return STACK;
			case ActionButtonMode.ROLL: return ROLL;
			case ActionButtonMode.ALIGN: return ALIGN;
			case ActionButtonMode.CHANGE: return CHANGE;
			case ActionButtonMode.CANCEL: return CANCEL;
		}
		
		return "None";
	}
}

public class ActionPanel : MonoBehaviour
{
	[SerializeField] protected ActionButton[] actionButtons;
	[SerializeField] protected AudioClip actionButtonSound;
	[SerializeField] protected AudioClip cancelButtonSound;
	[SerializeField] protected Sprite[] actionSprites = new Sprite[(int)ActionButtonMode.NUM_MODES];
	[SerializeField] protected RectTransform anchorToSnapToSideBar;
	[SerializeField] protected float snapToSideBarScale = 1f;
	[SerializeField] protected RectTransform anchorToCenterOnSideBar;
	[SerializeField] protected RectTransform anchorToScaleOnSmallScreens;
	[SerializeField] protected float scaleOnSmallScreensFactor = 0.5f;

	protected RectTransform rTrans;
	protected Canvas canvas;
	protected CanvasScaler canvasScaler;
	protected float screenScaleFactor = 1f;
	protected Rect rect;
	protected Robot robot;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );

	protected bool isSmallScreen = false;

	public static ActionPanel instance = null;

	public Sprite GetActionSprite(ActionButtonMode mode) {
		return actionSprites[(int)mode];
	}

	protected virtual void Awake()
	{
		rTrans = transform as RectTransform;
		canvas = GetComponentInParent<Canvas>();
		canvasScaler = canvas.gameObject.GetComponent<CanvasScaler>();
		foreach(ActionButton button in actionButtons) button.ClaimOwnership(this);
	}

	public void DisableButtons() {
		for(int i=0; i<actionButtons.Length; i++) actionButtons[i].SetMode(ActionButtonMode.DISABLED);
	}
	
	public void SetActionButtons()
	{
		DisableButtons();
		
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null ) return;
		
		robot = RobotEngineManager.instance.current;

		if( robot.isBusy ) return;
		
		if( robot.Status( Robot.StatusFlag.IS_CARRYING_BLOCK ) )
		{
			if( robot.selectedObjects.Count > 0 ) actionButtons[1].SetMode( ActionButtonMode.STACK );
			
			actionButtons[0].SetMode( ActionButtonMode.DROP );
		}
		else
		{
			if( robot.selectedObjects.Count == 1 )
			{
				actionButtons[1].SetMode( ActionButtonMode.PICK_UP );
			}
			else
			{
				for( int i = 0; i < robot.selectedObjects.Count; ++i )
				{
					if( actionButtons.Length > i ) actionButtons[i].SetMode( ActionButtonMode.PICK_UP, i );
				}
			}
		}
		
		if( robot.selectedObjects.Count > 0 && actionButtons.Length > 2 ) actionButtons[2].SetMode( ActionButtonMode.CANCEL );
	}

	protected virtual void OnEnable()
	{
		instance = this;

		ResizeToScreen();
	}

//	protected virtual void Update() {
//		if(RobotEngineManager.instance == null || RobotEngineManager.instance.current == null) {
//			DisableButtons();
//			return;
//		}
//	}

	protected virtual void ResizeToScreen() {
		float dpi = Screen.dpi;//
		isSmallScreen = false;
		if(dpi == 0f) return;
		
		float refW = Screen.width;
		float refH = Screen.height;
		
		screenScaleFactor = 1f;
		
		if(canvasScaler != null) {
			screenScaleFactor = canvasScaler.referenceResolution.y / Screen.height;
			refW = canvasScaler.referenceResolution.x;
			refH = canvasScaler.referenceResolution.y;
		}
		
		float refAspect = refW / refH;
		float actualAspect = (float)Screen.width / (float)Screen.height;
		
		float totalRefWidth = (refW / refAspect) * actualAspect;
		float sideBarWidth = (totalRefWidth - refW) * 0.5f;
		
		if( sideBarWidth > 50f && anchorToSnapToSideBar != null) {
			Vector2 size = anchorToSnapToSideBar.sizeDelta;
			size.x = sideBarWidth * snapToSideBarScale;
			anchorToSnapToSideBar.sizeDelta = size;
			
			if(anchorToCenterOnSideBar != null) {
				Vector3 anchor = anchorToCenterOnSideBar.anchoredPosition;
				anchor.x = size.x * 0.5f - sideBarWidth * 0.5f;
				anchorToCenterOnSideBar.anchoredPosition = anchor;
			}
		}
		
		float screenHeightInches = (float)Screen.height / (float)dpi;
		isSmallScreen = screenHeightInches < CozmoUtil.SMALL_SCREEN_MAX_HEIGHT;
		if(isSmallScreen && anchorToScaleOnSmallScreens != null) {
			
			Vector2 size = anchorToScaleOnSmallScreens.sizeDelta;
			float newScale = (refH * scaleOnSmallScreensFactor) / size.y;
			
			anchorToScaleOnSmallScreens.localScale = Vector3.one * newScale;
			Vector3 anchor = anchorToScaleOnSmallScreens.anchoredPosition;
			anchor.y = 0f;
			anchorToScaleOnSmallScreens.anchoredPosition = anchor;
		}
		
	}

	public void ActionButtonClick()
	{
		audio.volume = 1f;
		audio.PlayOneShot( actionButtonSound, 1f );
	}
	
	public void CancelButtonClick()
	{
		audio.volume = 1f;
		audio.PlayOneShot( cancelButtonSound, 1f );
	}

	protected virtual void OnDisable()
	{
		if(instance == this) instance = null;
	}

}
