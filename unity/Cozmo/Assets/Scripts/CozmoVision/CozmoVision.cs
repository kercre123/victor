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
	[SerializeField] protected Slider headAngleSlider;
	
	protected RectTransform rTrans;
	protected Rect rect;
	protected Robot robot;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );
	protected float lastHeadAngle = 0f;
	protected List<int> lastObservedObjects = new List<int>();

	private float dingTime = 0f;
	private float dongTime = 0f;

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

	private void Awake()
	{
		rTrans = transform as RectTransform;
	}

	protected void SetActionButtons()
	{
		robot = RobotEngineManager.instance.current;

		for( int i = 0; i < actionButtons.Length; ++i )
		{
			actionButtons[i].button.gameObject.SetActive( ( i == 0 && robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK && robot.selectedObject == -1 ) || robot.selectedObject > -1 );
			
			if( i == 0 )
			{
				if( robot.status == Robot.StatusFlag.IS_CARRYING_BLOCK )
				{
					if( robot.selectedObject > -1 )
					{
						actionButtons[i].text.text = "Stack " + robot.carryingObjectID + " on " + robot.selectedObject;
					}
					else
					{
						actionButtons[i].text.text = "Drop " + robot.carryingObjectID;
					}
				}
				else
				{
					actionButtons[i].text.text = "Pick Up " + robot.selectedObject;
				}
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

		if( button.interactable )
		{
			button.interactable = false;
		}
	}

	public void ForceDropBox()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.SetRobotCarryingObject( Intro.CurrentRobotID );

			RobotEngineManager.instance.current.selectedObject = -1;
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
			RobotEngineManager.instance.SetHeadAngle( RobotEngineManager.instance.defaultHeadAngle );
		}
	}

	public void RequestImage()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RequestImage( Intro.CurrentRobotID );
			RobotEngineManager.instance.SetHeadAngle( RobotEngineManager.instance.defaultHeadAngle );
			RobotEngineManager.instance.SetLiftHeight( 0f );
		}
	}

	private void OnEnable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage += RobotImage;
		}

		RequestImage();
		ResizeToScreen();

		if(RobotEngineManager.instance != null && RobotEngineManager.instance.current.headAngle_rad != lastHeadAngle) {
			headAngleSlider.value = Mathf.Clamp(lastHeadAngle * Mathf.Rad2Deg, -90f, 90f);
			lastHeadAngle = RobotEngineManager.instance.current.headAngle_rad;
		}
	}

	protected void DetectObservedObjects()
	{
		if( RobotEngineManager.instance != null )
		{
			for( int i = 0; i < RobotEngineManager.instance.current.observedObjects.Count; ++i )
			{
				if( RobotEngineManager.instance.current.selectedObject == -1 && !lastObservedObjects.Contains( RobotEngineManager.instance.current.observedObjects[i].ID ) )
				{
					Ding( true );
					
					break;
				}
			}

			for( int i = 0; i < lastObservedObjects.Count; ++i )
			{
				ObservedObject lastObservedObject = RobotEngineManager.instance.current.observedObjects.Find( x => x.ID == lastObservedObjects[i] );
				
				if( RobotEngineManager.instance.current.selectedObject == -1 && lastObservedObject == null )
				{
					Ding( false );
					
					break;
				}
			}
		}
	}

	protected void Ding( bool found )
	{
		if( found )
		{
			if( dingTime + 2f < Time.time )
			{
				RobotEngineManager.instance.ObjectObserved( found );
			
				dingTime = Time.time;
			}
		}
		else
		{
			if( dongTime + 2f < Time.time )
			{
				RobotEngineManager.instance.ObjectObserved( found );
				
				dongTime = Time.time;
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
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.SetHeadAngle(lastHeadAngle);
		Debug.Log("OnHeadAngleSliderReleased lastHeadAngle("+lastHeadAngle+") headAngleSlider.value("+headAngleSlider.value+")");
	}

}
