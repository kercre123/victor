using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;

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

	public bool IsSmallScreen { get; protected set; }

	public static ActionPanel instance = null;

	protected virtual void Awake()
	{
		rTrans = transform as RectTransform;
		canvas = GetComponentInParent<Canvas>();
		canvasScaler = canvas.gameObject.GetComponent<CanvasScaler>();
	}

	public void DisableButtons() {
		for(int i=0; i<actionButtons.Length; i++) actionButtons[i].SetMode(ActionButton.Mode.DISABLED);
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
		IsSmallScreen = false;
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
		IsSmallScreen = screenHeightInches < CozmoUtil.SMALL_SCREEN_MAX_HEIGHT;
		if(IsSmallScreen && anchorToScaleOnSmallScreens != null) {
			
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
