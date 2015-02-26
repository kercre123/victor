using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision : MonoBehaviour
{
	[SerializeField] protected Button button;
	[SerializeField] protected Image image;
	[SerializeField] protected Text text;
	[SerializeField] protected Button[] actionButtons;
	[SerializeField] protected int maxBoxes;
	
	protected Rect rect;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );

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
			RobotEngineManager.instance.PickAndPlaceObject();

			RobotEngineManager.instance.current.selectedObject = uint.MaxValue;
		}
	}

	public void Cancel()
	{
		Debug.Log( "Cancel" );

		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.current.selectedObject = uint.MaxValue;
		}
	}

	public void RequestImage()
	{
		Debug.Log( "request image" );

		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RequestImage( Intro.CurrentRobotID );
		}
	}

	protected void OnEnable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage += RobotImage;
		}
	}

	protected void OnDisable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.RobotImage -= RobotImage;
		}
	}
}
