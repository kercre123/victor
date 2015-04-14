using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;

public enum ActionButtonMode
{
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

	public ActionButtonMode mode { get; private set; }

	public void SetMode( ActionButtonMode m, int selectedObjectIndex = 0, string append = null )
	{
		button.onClick.RemoveAllListeners();
		mode = m;
		
		if( mode == ActionButtonMode.DISABLED || ActionPanel.instance == null || ActionPanel.instance.gameActions == null )
		{
			button.gameObject.SetActive( false );
			return;
		}

		GameActions gameActions = ActionPanel.instance.gameActions;

		image.sprite = ActionButton.GetModeSprite( mode );
		gameActions.selectedObjectIndex = selectedObjectIndex;
		
		switch( mode )
		{
			case ActionButtonMode.PICK_UP:
				text.text = gameActions.PICK_UP;
				button.onClick.AddListener( gameActions.PickUp );
				button.onClick.AddListener( gameActions.ActionButtonClick );
				break;
			case ActionButtonMode.DROP:
				text.text = gameActions.DROP;
				button.onClick.AddListener( gameActions.Drop );
				button.onClick.AddListener( gameActions.ActionButtonClick );
				break;
			case ActionButtonMode.STACK:
				text.text = gameActions.STACK;
				button.onClick.AddListener( gameActions.Stack );
				button.onClick.AddListener( gameActions.ActionButtonClick );
				break;
			case ActionButtonMode.ROLL:
				text.text = gameActions.ROLL;
				button.onClick.AddListener( gameActions.Roll );
				button.onClick.AddListener( gameActions.ActionButtonClick );
				break;
			case ActionButtonMode.ALIGN:
				text.text = gameActions.ALIGN;
				button.onClick.AddListener( gameActions.Align );
				button.onClick.AddListener( gameActions.ActionButtonClick );
				break;
			case ActionButtonMode.CHANGE:
				text.text = gameActions.CHANGE;
				button.onClick.AddListener( gameActions.Align );
				button.onClick.AddListener( gameActions.ActionButtonClick );
				break;
			case ActionButtonMode.CANCEL:
				text.text = gameActions.CANCEL;
				button.onClick.AddListener( gameActions.Cancel );
				button.onClick.AddListener( gameActions.CancelButtonClick );
				break;
		}
		
		if( append != null ) text.text += append;
		
		button.gameObject.SetActive( true );
	}

	public static Sprite GetModeSprite( ActionButtonMode mode )
	{
		if( ActionPanel.instance != null && ActionPanel.instance.gameActions != null )
		{
			switch( mode )
			{
				case ActionButtonMode.TARGET: return ActionPanel.instance.gameActions.GetActionSprite( mode );
				case ActionButtonMode.PICK_UP: return ActionPanel.instance.gameActions.GetActionSprite( mode );
				case ActionButtonMode.DROP: return ActionPanel.instance.gameActions.GetActionSprite( mode );
				case ActionButtonMode.STACK: return ActionPanel.instance.gameActions.GetActionSprite( mode );
				case ActionButtonMode.ROLL: return ActionPanel.instance.gameActions.GetActionSprite( mode );
				case ActionButtonMode.ALIGN: return ActionPanel.instance.gameActions.GetActionSprite( mode );
				case ActionButtonMode.CHANGE: return ActionPanel.instance.gameActions.GetActionSprite( mode );
				case ActionButtonMode.CANCEL: return ActionPanel.instance.gameActions.GetActionSprite( mode );
			}
		}
		
		return null;
	}

	public static string GetModeName( ActionButtonMode mode )
	{
		if( ActionPanel.instance != null && ActionPanel.instance.gameActions != null )
		{
			switch( mode )
			{
				case ActionButtonMode.TARGET: return ActionPanel.instance.gameActions.TARGET;
				case ActionButtonMode.PICK_UP: return ActionPanel.instance.gameActions.PICK_UP;
				case ActionButtonMode.DROP: return ActionPanel.instance.gameActions.DROP;
				case ActionButtonMode.STACK: return ActionPanel.instance.gameActions.STACK;
				case ActionButtonMode.ROLL: return ActionPanel.instance.gameActions.ROLL;
				case ActionButtonMode.ALIGN: return ActionPanel.instance.gameActions.ALIGN;
				case ActionButtonMode.CHANGE: return ActionPanel.instance.gameActions.CHANGE;
				case ActionButtonMode.CANCEL: return ActionPanel.instance.gameActions.CANCEL;
			}
		}
		
		return "None";
	}
}

public class ActionPanel : MonoBehaviour
{
	public ActionButton[] actionButtons;
	
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
	protected int selectedObjectIndex;

	protected bool isSmallScreen = false;

	public static ActionPanel instance = null;

	[System.NonSerialized] public GameActions gameActions;

	protected virtual void Awake()
	{
		rTrans = transform as RectTransform;
		canvas = GetComponentInParent<Canvas>();
		canvasScaler = canvas.gameObject.GetComponent<CanvasScaler>();
	}

	public void DisableButtons() {
		for(int i=0; i<actionButtons.Length; i++) actionButtons[i].SetMode(ActionButtonMode.DISABLED);
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

	public virtual void OnDisable()
	{
		if(instance == this) instance = null;
	}
}
