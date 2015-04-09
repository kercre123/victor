using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision : MonoBehaviour
{
	[SerializeField] protected Image image;
	[SerializeField] protected Text text;
	[SerializeField] protected GameObject actionPanelPrefab;
	[SerializeField] protected AudioClip newObjectObservedSound;
	[SerializeField] protected AudioClip objectObservedLostSound;
	[SerializeField] protected AudioSource loopAudioSource;
	[SerializeField] protected AudioClip visionActiveLoop;
	[SerializeField] protected float maxLoopingVol = 0.5f;
	[SerializeField] protected AudioClip visionActivateSound;
	[SerializeField] protected float maxVisionStartVol = 0.1f;
	[SerializeField] protected AudioClip visionDeactivateSound;
	[SerializeField] protected float maxVisionStopVol = 0.2f;
	[SerializeField] protected AudioClip selectSound;
	[SerializeField] protected float soundDelay = 2f;
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

	protected ActionPanel actionPanel;

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
	private static bool dingEnabled = true;
	
	protected bool isSmallScreen = false;
	protected static readonly Vector2 NativeResolution = new Vector2( 320f, 240f );

	public static CozmoVision instance = null;

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

		if(actionPanelPrefab != null) {
			GameObject actionPanelObject = (GameObject)GameObject.Instantiate(actionPanelPrefab);
			actionPanel = (ActionPanel)actionPanelObject.GetComponentInChildren(typeof(ActionPanel));
			actionPanel.gameObject.SetActive(false);
		}
	}
	
	protected virtual void ObservedObjectSeen( ObservedObjectBox box, ObservedObject observedObject )
	{
		float boxX = ( observedObject.VizRect.x / NativeResolution.x ) * imageRectTrans.sizeDelta.x;
		float boxY = ( observedObject.VizRect.y / NativeResolution.y ) * imageRectTrans.sizeDelta.y;
		float boxW = ( observedObject.VizRect.width / NativeResolution.x ) * imageRectTrans.sizeDelta.x;
		float boxH = ( observedObject.VizRect.height / NativeResolution.y ) * imageRectTrans.sizeDelta.y;
		
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
		if( robot == null || pertinentObjects == null ) return;

		for( int i = 0; i < robot.selectedObjects.Count; ++i )
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
		if(!image.enabled) return;

		if( robot == null || pertinentObjects == null ) return;
		
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

		if(actionPanel != null) actionPanel.gameObject.SetActive(true);
	}

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
	
	protected virtual void VisionEnabled()
	{
		fadingIn = false;
		fadingOut = false;
		image.enabled = !OptionsScreen.GetToggleDisableVision();

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

		if(!fadingIn && !fadingOut) return;

		fadeTimer += Time.deltaTime;

		float factor = fadeTimer / fadeDuration;

		RefreshLoopingTargetSound(factor);
		
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

		if(factor >= 1f) {
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

		if (dingEnabled) {
			if (found) {
				if (!audio.isPlaying && dingTimes [0] + soundDelay < Time.time) {
					audio.volume = 1f;
					audio.PlayOneShot (newObjectObservedSound, 1f);
				
					dingTimes [0] = Time.time;
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
	}

	protected virtual void LateUpdate()
	{
		if( robot != null )
		{
			robot.lastObservedObjects.Clear();
			robot.lastSelectedObjects.Clear();
			robot.lastMarkersVisibleObjects.Clear();
			
			if( !robot.isBusy )
			{
				robot.lastObservedObjects.AddRange( robot.observedObjects );
				robot.lastSelectedObjects.AddRange( robot.selectedObjects );
				robot.lastMarkersVisibleObjects.AddRange( robot.markersVisibleObjects );
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

		if(actionPanel != null) actionPanel.gameObject.SetActive(false);
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

	public static void EnableDing(bool on = true)
	{
		dingEnabled = on;
	}

}
