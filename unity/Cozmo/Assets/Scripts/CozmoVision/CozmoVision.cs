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
				button.onClick.AddListener(vision.ActionButtonClick);
				break;
			case ActionButtonMode.DROP:
				text.text = "Drop";
				button.onClick.AddListener(RobotEngineManager.instance.current.PlaceObjectOnGroundHere);
				button.onClick.AddListener(vision.ActionButtonClick);
				break;
			case ActionButtonMode.STACK:
				text.text = "Stack";
				button.onClick.AddListener(PickAndPlaceObject);
				button.onClick.AddListener(vision.ActionButtonClick);
				break;
			case ActionButtonMode.ROLL:
				text.text = "Roll";
				//button.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.ALIGN:
				text.text = "Align";
				button.onClick.AddListener(RobotEngineManager.instance.current.PlaceObjectOnGroundHere);
				button.onClick.AddListener(vision.ActionButtonClick);
				break;
			case ActionButtonMode.CHANGE:
				text.text = "Change";
				//button.onClick.AddListener(vision.Action);
				break;
			case ActionButtonMode.CANCEL:
				text.text = "Cancel";
				button.onClick.AddListener(Cancel);
				button.onClick.AddListener(vision.CancelButtonClick);
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
	[SerializeField] protected AudioClip actionButtonSound;
	[SerializeField] protected AudioClip cancelButtonSound;
	[SerializeField] protected AudioClip swishSound;
	[SerializeField] protected AudioClip reverseSwishSound;
	[SerializeField] protected float soundDelay = 2f;
	[SerializeField] public Sprite[] actionSprites = new Sprite[(int)ActionButtonMode.NUM_MODES];
	[SerializeField] protected RectTransform anchorToSnapToSideBar;
	[SerializeField] protected float snapToSideBarScale = 1f;
	[SerializeField] protected RectTransform anchorToCenterOnSideBar;
	[SerializeField] protected RectTransform anchorToScaleOnSmallScreens;
	[SerializeField] protected float scaleOnSmallScreensFactor = 0.5f;
	[SerializeField] protected GameObject observedObjectCanvasPrefab;
	[SerializeField] protected Color selected;
	[SerializeField] protected Color select;

	public UnityAction[] actions;

	protected RectTransform rTrans;
	protected RectTransform imageRectTrans;
	protected Canvas canvas;
	protected CanvasScaler canvasScaler;
	protected float screenScaleFactor = 1f;
	protected Rect rect;
	protected Robot robot;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );
	protected ObservedObjectBox[] observedObjectBoxes;
	protected GameObject observedObjectCanvas;

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

		observedObjectCanvas = (GameObject)GameObject.Instantiate(observedObjectCanvasPrefab);
		observedObjectBoxes = observedObjectCanvas.GetComponentsInChildren<ObservedObjectBox>(true);

		foreach(ObservedObjectBox box in observedObjectBoxes) box.image.gameObject.SetActive(false);

		foreach(ActionButton button in actionButtons) button.ClaimOwnership(this);
	}

	protected virtual void ObservedObjectSeen( ObservedObjectBox box, ObservedObject observedObject )
	{
		float boxX = ( observedObject.VizRect.x / 320f ) * imageRectTrans.sizeDelta.x;
		float boxY = ( observedObject.VizRect.y / 240f ) * imageRectTrans.sizeDelta.y;
		float boxW = ( observedObject.VizRect.width / 320f ) * imageRectTrans.sizeDelta.x;
		float boxH = ( observedObject.VizRect.height / 240f ) * imageRectTrans.sizeDelta.y;
		
		box.image.rectTransform.sizeDelta = new Vector2( boxW, boxH );
		box.image.rectTransform.anchoredPosition = new Vector2( boxX, -boxY );

		//if( observedObject.MarkersVisible )
		{
			if( robot.selectedObjects.Find( x => x.ID == observedObject.ID ) != null )
			{
				box.image.color = selected;
				box.text.text = "ID: " + observedObject.ID + " Family: " + observedObject.Family;
			}
			else
			{
				box.image.color = select;
				box.text.text = "Select ID: " + observedObject.ID + " Family: " + observedObject.Family;
				box.observedObject = observedObject;
			}

			box.image.gameObject.SetActive( true );
		}
	}

	protected virtual void ShowObservedObjects()
	{
		for( int i = 0; i < observedObjectBoxes.Length; ++i )
		{
			if( robot.observedObjects.Count > i )
			{
				ObservedObjectSeen( observedObjectBoxes[i], robot.observedObjects[i] );
			}
			else
			{
				observedObjectBoxes[i].image.gameObject.SetActive( false );
			}
		}
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

	private void RequestImage()
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
		VisionEnabled();
	}

	protected virtual void ResizeToScreen() {
		float dpi = Screen.dpi;//
		
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
		if(screenHeightInches < CozmoUtil.SMALL_SCREEN_MAX_HEIGHT && anchorToScaleOnSmallScreens != null) {
			
			Vector2 size = anchorToScaleOnSmallScreens.sizeDelta;
			float newScale = (refH * scaleOnSmallScreensFactor) / size.y;
			
			anchorToScaleOnSmallScreens.localScale = Vector3.one * newScale;
			Vector3 anchor = anchorToScaleOnSmallScreens.anchoredPosition;
			anchor.y = 0f;
			anchorToScaleOnSmallScreens.anchoredPosition = anchor;
		}
		
	}

	private void VisionEnabled()
	{
		if( PlayerPrefs.GetInt( "VisionDisabled" ) > 0 )
		{
			image.color = new Color( 0f, 0f, 0f, 0f );
		}
		else
		{
			image.color = new Color( 1f, 1f, 1f, 1f );
		}
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

	protected void StopLoopingTargetSound()
	{

		audio.loop = false;
		audio.Stop();
		audio.volume = 1f;

		wasLooping = false;
	}

	float loopTimer = 0f;
	float fromVol = 0f;
	float maxVol = 0.5f;
	bool wasLooping = false;
	protected void RefreshLoopingTargetSound( bool on )
	{
		loopTimer += Time.deltaTime;

		if(wasLooping != on) {
			loopTimer = 0f;
		}

		if(on) {
			audio.loop = true;
			audio.clip = newObjectObservedSound;

			if(!wasLooping) {
				fromVol = 0f;//loopTimer < 1f ? audio.volume : 0f;
				audio.Play();
			}

			audio.volume = Mathf.Lerp(fromVol, maxVol, loopTimer);
		}
		else if(loopTimer > 1f) {
			if(wasLooping) {
				audio.loop = false;
				audio.Stop();
			}
		}
		else {
			if(wasLooping) fromVol = audio.volume;
			audio.volume = Mathf.Lerp(fromVol, 0f, loopTimer);
		}

		wasLooping = on;
	}

	public void ActionButtonClick()
	{
		if(audio.loop) StopLoopingTargetSound();

		audio.PlayOneShot( actionButtonSound );
	}

	public void CancelButtonClick()
	{
		if(audio.loop) StopLoopingTargetSound();
		audio.PlayOneShot( cancelButtonSound );
	}

	public void SwishSound()
	{
		audio.PlayOneShot( swishSound );
	}

	public void ReverseSwishSound()
	{
		audio.PlayOneShot( reverseSwishSound );
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

	protected virtual void OnDisable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage -= RobotImage;
			RobotEngineManager.instance.DisconnectedFromClient -= Reset;
		}

		foreach(ObservedObjectBox box in observedObjectBoxes) { if(box != null && box.image != null) { box.image.gameObject.SetActive(false); } }
	}

}
