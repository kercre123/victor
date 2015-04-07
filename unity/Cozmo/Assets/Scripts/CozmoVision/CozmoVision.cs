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
		
		image.sprite = vision.GetActionSprite(mode);
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
	[SerializeField] protected AudioClip newObjectObservedSound;
	[SerializeField] protected AudioClip objectObservedLostSound;
	[SerializeField] protected AudioClip actionButtonSound;
	[SerializeField] protected AudioClip cancelButtonSound;
	[SerializeField] protected AudioSource loopAudioSource;
	[SerializeField] protected AudioClip visionActiveLoop;
	[SerializeField] protected float maxLoopingVol = 0.5f;
	[SerializeField] protected AudioClip visionActivateSound;
	[SerializeField] protected float maxVisionStartVol = 0.1f;
	[SerializeField] protected AudioClip visionDeactivateSound;
	[SerializeField] protected float maxVisionStopVol = 0.2f;
	[SerializeField] protected AudioClip slideInSound;
	[SerializeField] protected AudioClip slideOutSound;
	[SerializeField] protected AudioClip selectSound;
	[SerializeField] protected float soundDelay = 2f;
	[SerializeField] protected Sprite[] actionSprites = new Sprite[(int)ActionButtonMode.NUM_MODES];
	[SerializeField] protected RectTransform anchorToSnapToSideBar;
	[SerializeField] protected float snapToSideBarScale = 1f;
	[SerializeField] protected RectTransform anchorToCenterOnSideBar;
	[SerializeField] protected RectTransform anchorToScaleOnSmallScreens;
	[SerializeField] protected float scaleOnSmallScreensFactor = 0.5f;
	[SerializeField] protected GameObject observedObjectCanvasPrefab;
	[SerializeField] protected Color selected;
	[SerializeField] protected Color select;


	public enum ObservedObjectListType {
		OBSERVED_RECENTLY,
		MARKERS_SEEN,
		KNOWN
	}

	[SerializeField] protected ObservedObjectListType observedObjectListType = ObservedObjectListType.MARKERS_SEEN;

	private List<ObservedObject> _pertinentObjects = new List<ObservedObject>(); 
	private int pertinenceStamp = -1; 
	public List<ObservedObject> pertinentObjects {
		get {

			if(pertinenceStamp == Time.frameCount) return _pertinentObjects;
			pertinenceStamp = Time.frameCount;

			float tooHigh = 2f * CozmoUtil.BLOCK_LENGTH_MM;

			_pertinentObjects.Clear();

			switch(observedObjectListType) {
				case ObservedObjectListType.MARKERS_SEEN: 
					_pertinentObjects.AddRange(robot.markersVisibleObjects);
					break;
				case ObservedObjectListType.OBSERVED_RECENTLY: 
					_pertinentObjects.AddRange(robot.observedObjects);
					break;
				case ObservedObjectListType.KNOWN: 
					_pertinentObjects.AddRange(robot.knownObjects);
					break;
			}

			_pertinentObjects = _pertinentObjects.FindAll( x => Mathf.Abs( (x.WorldPosition - robot.WorldPosition).z ) < tooHigh );

			_pertinentObjects.Sort( ( obj1 ,obj2 ) => {
				float d1 = ( (Vector2)obj1.WorldPosition - (Vector2)robot.WorldPosition).sqrMagnitude;
				float d2 = ( (Vector2)obj2.WorldPosition - (Vector2)robot.WorldPosition).sqrMagnitude;
				return d1.CompareTo(d2);   
			} );

			return _pertinentObjects;
		}
	}


	public UnityAction[] actions;
	
	protected RectTransform rTrans;
	protected RectTransform imageRectTrans;
	protected Canvas canvas;
	protected CanvasScaler canvasScaler;
	protected float screenScaleFactor = 1f;
	protected Rect rect;
	protected Robot robot;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );
	protected List<ObservedObjectBox> observedObjectBoxes = new List<ObservedObjectBox>();
	protected GameObject observedObjectCanvas;
	
	private float[] dingTimes = new float[2] { 0f, 0f };
	private static bool imageRequested = false;
	private float fromVol = 0f;
	private bool fadingOut = false;
	private bool fadingIn = false;
	private float fadeTimer = 0f;
	private float fadeDuration = 1f;
	private float fromAlpha = 0f;


	public static CozmoVision instance = null;


	public Sprite GetActionSprite(ActionButtonMode mode) {
		return actionSprites[(int)mode];
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

		Canvas canv = observedObjectCanvas.GetComponent<Canvas>();
		canv.worldCamera = canvas.worldCamera;

		observedObjectBoxes.Clear();
		observedObjectBoxes.AddRange(observedObjectCanvas.GetComponentsInChildren<ObservedObjectBox>(true));
		
		foreach(ObservedObjectBox box in observedObjectBoxes) box.gameObject.SetActive(false);
		
		foreach(ActionButton button in actionButtons) button.ClaimOwnership(this);
	}
	
	protected virtual void ObservedObjectSeen( ObservedObjectBox box, ObservedObject observedObject )
	{
		float boxX = ( observedObject.VizRect.x / 320f ) * imageRectTrans.sizeDelta.x;
		float boxY = ( observedObject.VizRect.y / 240f ) * imageRectTrans.sizeDelta.y;
		float boxW = ( observedObject.VizRect.width / 320f ) * imageRectTrans.sizeDelta.x;
		float boxH = ( observedObject.VizRect.height / 240f ) * imageRectTrans.sizeDelta.y;

		/*if( boxX < 200f )
		{
			boxW -= 200f - boxX;
			boxX = 200f;
		}
		else if( boxX + boxW > 1000f )
		{
			boxW -= boxX + boxW - 1000f;
		}
		
		if( boxW <= 0f )
		{
			box.image.gameObject.SetActive( true );
			return;
		}*/
		
		box.rectTransform.sizeDelta = new Vector2( boxW, boxH );
		box.rectTransform.anchoredPosition = new Vector2( boxX, -boxY );
		
		if( robot.selectedObjects.Find( x => x.ID == observedObject.ID ) != null )
		{
			box.SetColor( selected );
			box.text.text = "ID: " + observedObject.ID + " Family: " + observedObject.Family;
		}
		else
		{
			box.SetColor( select );
			box.text.text = "Select ID: " + observedObject.ID + " Family: " + observedObject.Family;
			box.observedObject = observedObject;
		}
		
		box.gameObject.SetActive( true );
	}

	protected void UnselectNonObservedObjects()
	{
		if( robot == null ) return;
		if( pertinentObjects == null ) return;

		for( int i = 0; i < pertinentObjects.Count; ++i )
		{
			if( pertinentObjects.Find( x => x.ID == robot.selectedObjects[i].ID ) == null )
			{
				robot.selectedObjects.RemoveAt( i-- );
			}
		}

		if(robot.selectedObjects.Count == 0) {
			robot.SetHeadAngle();
		}
	}
	
	protected virtual void ShowObservedObjects()
	{
		if( robot == null ) return;
		if( pertinentObjects == null ) return;
		
		for( int i = 0; i < observedObjectBoxes.Count; ++i )
		{
			if(pertinentObjects.Count > i) {
				ObservedObjectSeen( observedObjectBoxes[i], pertinentObjects[i] );
			}
			else {
				observedObjectBoxes[i].gameObject.SetActive( false );
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
		
		ShowObservedObjects();
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
		instance = this;

		int objectPertinenceOverride = PlayerPrefs.GetInt("ObjectPertinence", -1);
		if(objectPertinenceOverride >= 0) {
			observedObjectListType = (ObservedObjectListType)objectPertinenceOverride;
			Debug.Log("CozmoVision.OnEnable observedObjectListType("+observedObjectListType+")");
		}

		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage += RobotImage;
			RobotEngineManager.instance.DisconnectedFromClient += Reset;

			if(RobotEngineManager.instance.current != null) RobotEngineManager.instance.current.selectedObjects.Clear();
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
	
	protected virtual void VisionEnabled()
	{
		fadingIn = false;
		fadingOut = false;
		image.enabled = PlayerPrefs.GetInt("VisionDisabled") == 0;

		//start at no alpha
		float alpha = 0f;

		Color color = image.color;
		color.a = alpha;
		image.color = color;
		
		color = select;
		color.a = alpha;
		select = color;
		
		color = selected;
		color.a = alpha;
		selected = color;
	}

	protected void RefreshFade()
	{
		//if(!image.enabled) return;
		if(!fadingIn && !fadingOut) return;

		fadeTimer += Time.deltaTime;

		float factor = fadeTimer / fadeDuration;
		float alpha = 0f;
		if(fadingIn) {
			alpha = Mathf.Lerp(fromAlpha, 1f, factor);
		}
		else {
			alpha = Mathf.Lerp(fromAlpha, 0f, factor);
		}

		Color color = image.color;
		color.a = alpha;
		image.color = color;
		
		color = select;
		color.a = alpha;
		select = color;
		
		color = selected;
		color.a = alpha;
		selected = color;

		RefreshLoopingTargetSound(factor);

		if(fadeTimer >= 1f) {
			fadingIn = false;
			fadingOut = false;
		}

	}

	protected void FadeIn()
	{
		if(fadingIn) return;
		if(image.color.a >= 1f) return;

		fadeTimer = 0f;
		fadingIn = true;
		fadingOut = false;

		PlayVisionActivateSound();

		//if(!image.enabled) return;

		Color color = image.color;
		fromAlpha = color.a;
		
		color = select;
		color.a = fromAlpha;
		select = color;
		
		color = selected;
		color.a = fromAlpha;
		selected = color;
	}

	protected void FadeOut()
	{

		if(fadingOut) return;
		if(image.color.a <= 0f) return;
		
		fadeTimer = 0f;
		fadingOut = true;
		fadingIn = false;
		
		PlayVisionDeactivateSound();

		//if(!image.enabled) return;

		Color color = image.color;
		fromAlpha = color.a;
		
		color = select;
		color.a = fromAlpha;
		select = color;
		
		color = selected;
		color.a = fromAlpha;
		selected = color;
	}

	protected void StopLoopingTargetSound()
	{
		if(loopAudioSource != null) loopAudioSource.Stop();
	}
	
	protected void PlayVisionActivateSound()
	{
		if(visionActivateSound != null) {
			audio.volume = maxVisionStartVol;
			audio.PlayOneShot(visionActivateSound, maxVisionStartVol);
		}

		if(loopAudioSource != null) {
			fromVol = loopAudioSource.volume;
			loopAudioSource.loop = true;
			loopAudioSource.clip = visionActiveLoop;
			loopAudioSource.Play();
		}
		// Debug.Log("TargetSearchStartSound audio.volume("+audio.volume+")");
	}
	
	protected void PlayVisionDeactivateSound()
	{
		if(visionDeactivateSound != null) {
			audio.volume = maxVisionStopVol;
			audio.PlayOneShot(visionDeactivateSound, maxVisionStopVol);
		}

		if(loopAudioSource != null) fromVol = loopAudioSource.volume;

		// Debug.Log("TargetSearchStopSound audio.volume("+audio.volume+")");
	}
	
	protected void RefreshLoopingTargetSound(float factor)
	{
		if(loopAudioSource == null) return;

		if(fadingIn) {
			loopAudioSource.volume = Mathf.Lerp(fromVol, maxLoopingVol, factor);
		}
		else if(fadingOut) {
			loopAudioSource.volume = Mathf.Lerp(fromVol, 0f, factor);
		}

		if(factor >= 1f && fadingOut) {
			loopAudioSource.Stop();
		}

	}

	protected virtual void Dings()
	{
		if( robot == null || robot.isBusy || robot.selectedObjects.Count > 0 )
		{
			return;
		}
			
		if( pertinentObjects.Count > 0/*lastObservedObjects.Count*/ )
		{
			Ding( true );
		}
		/*else if( robot.observedObjects.Count < lastObservedObjects.Count )
		{
			Ding( false );
		}*/
	}
	
	protected void Ding( bool found )
	{
		if(newObjectObservedSound == null) return;

		if( found )
		{
			if( !audio.isPlaying && dingTimes[0] + soundDelay < Time.time )
			{
				audio.volume = 1f;
				audio.PlayOneShot( newObjectObservedSound, 1f );
				
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
	
	public void SlideInSound()
	{
		audio.volume = 1f;
		audio.PlayOneShot( slideInSound, 1f );
	}
	
	public void SlideOutSound()
	{
		audio.volume = 1f;
		audio.PlayOneShot( slideOutSound, 1f );
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
		
		foreach(ObservedObjectBox box in observedObjectBoxes) { if(box != null) { box.gameObject.SetActive(false); } }

		StopLoopingTargetSound();

		if(instance == this) instance = null;
	}

	public void Selection(ObservedObject obj)
	{
		if( robot != null )
		{
			robot.selectedObjects.Clear();
			RobotEngineManager.instance.current.selectedObjects.Add(obj);
			RobotEngineManager.instance.current.TrackHeadToObject(obj);
		}
		
		if( audio != null ) audio.PlayOneShot( selectSound );
	}

}
