using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ImageTest : MonoBehaviour
{
	[SerializeField] protected Button button;
	[SerializeField] protected Button actionButton;
	[SerializeField] protected Image image;
	[SerializeField] protected Text text;

	protected Rect rect;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );

	protected void Update()
	{
		if( RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			actionButton.gameObject.SetActive( RobotEngineManager.instance.current.box.ID != 0 );
		}
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
		RobotEngineManager.instance.PickUpBox();
	}

	public void RequestImage()
	{
		Debug.Log( "request image" );

		RobotEngineManager.instance.RequestImage( Intro.CurrentRobotID );
	}

	protected void OnEnable()
	{
		RobotEngineManager.instance.RobotImage += RobotImage;
	}

	protected void OnDisable()
	{
		RobotEngineManager.instance.RobotImage -= RobotImage;
	}
}
