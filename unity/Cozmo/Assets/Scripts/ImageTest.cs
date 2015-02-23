using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ImageTest : MonoBehaviour
{
	[SerializeField] protected Button button;
	[SerializeField] protected Button actionButton;
	[SerializeField] protected Image actionButtonImage;
	[SerializeField] protected Text actionButtonText;
	[SerializeField] protected Image image;
	[SerializeField] protected Text text;

	//protected uint lastBoxID = uint.MaxValue;
	protected Rect rect;
	protected readonly Vector2 pivot = new Vector2( 0.5f, 0.5f );

	protected void Update()
	{
		if( RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			//Debug.Log( RobotEngineManager.instance.current.box.ID + " vs " + lastBoxID );

			if( RobotEngineManager.instance.current.box.ID != uint.MaxValue /*&& RobotEngineManager.instance.current.box.ID != lastBoxID*/ )
			{
				actionButtonImage.rectTransform.sizeDelta = new Vector2( RobotEngineManager.instance.current.box.width, RobotEngineManager.instance.current.box.height );
				//Debug.Log( "x: " + RobotEngineManager.instance.current.box.topLeft_x );
				//Debug.Log( "y: " + RobotEngineManager.instance.current.box.topLeft_y );

				actionButtonImage.rectTransform.anchoredPosition = new Vector2( RobotEngineManager.instance.current.box.topLeft_x, -RobotEngineManager.instance.current.box.topLeft_y );

				actionButtonText.text = "Action on ID: " + RobotEngineManager.instance.current.box.ID;

				actionButton.gameObject.SetActive( true );
			}
			else
			{
				actionButton.gameObject.SetActive( false );

				/*if( RobotEngineManager.instance.current.box.ID == uint.MaxValue )
				{
					lastBoxID = uint.MaxValue;
				}*/
			}
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
		//lastBoxID = RobotEngineManager.instance.current.box.ID;

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
