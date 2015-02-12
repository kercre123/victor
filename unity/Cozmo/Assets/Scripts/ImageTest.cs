using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ImageTest : MonoBehaviour
{
	[SerializeField] protected Image image;

	protected void RobotImage( Texture2D texture )
	{
		image.material.SetTexture( "_MainTex", texture );
	}

	public void RequestImage()
	{
		Debug.Log( "request image" );

		RobotEngineManager.instance.RequestImage(Intro.CurrentRobotID);
	}

	protected void Awake()
	{
		RobotEngineManager.instance.RobotImage += RobotImage;
	}
}
