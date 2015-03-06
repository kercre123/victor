using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision : MonoBehaviour
{
	[System.Serializable]
	public struct ActionButton
	{
		public Button button;
		public Text text;
	}

	[SerializeField] protected Button button;
	[SerializeField] protected Image image;
	[SerializeField] protected Text text;
	[SerializeField] protected ActionButton[] actionButtons;
	[SerializeField] protected int maxObservedObjects;
	
	protected RectTransform rTrans;
	protected Rect rect;
	protected Robot robot;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );

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

	private void Awake() {
		rTrans = transform as RectTransform;
	}

	protected void RobotImage( Texture2D texture )
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

		if( button.interactable )
		{
			button.interactable = false;
		}
	}
	
	public void Action()
	{
		Debug.Log( "Action" );

		if( RobotEngineManager.instance != null )
		{
			if( RobotEngineManager.instance.current.status == Robot.StatusFlag.IS_CARRYING_BLOCK && RobotEngineManager.instance.current.selectedObject == -1 )
			{
				RobotEngineManager.instance.PlaceObjectOnGroundHere();
			}
			else
			{
				RobotEngineManager.instance.PickAndPlaceObject();
			}

			RobotEngineManager.instance.current.selectedObject = -2;
		}
	}

	public void Cancel()
	{
		Debug.Log( "Cancel" );

		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.current.selectedObject = -1;
		}
	}

	public void RequestImage()
	{
		Debug.Log( "request image" );

		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RequestImage( Intro.CurrentRobotID );
			RobotEngineManager.instance.SetHeadAngle( RobotEngineManager.instance.defaultHeadAngle );
		}
	}

	protected void OnEnable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage += RobotImage;
		}

		ResizeToScreen();
	}

	protected void OnDisable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage -= RobotImage;
		}
	}

	private void ResizeToScreen() {
		if(rTrans == null) return;

		float scale = (Screen.width * 0.5f) / 320f;
		rTrans.localScale = Vector3.one * scale;
	}
}
