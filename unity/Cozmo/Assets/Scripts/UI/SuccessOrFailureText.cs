using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SuccessOrFailureText : ScreenMessage
{
	[SerializeField] private AudioClip success;
	[SerializeField] private AudioClip failure;
	[SerializeField] protected float timeOnScreen = 5f;

	private void OnEnable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.SuccessOrFailure += SuccessOrFailure;
		}

		text.text = string.Empty;
	}

	private void OnDisable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.SuccessOrFailure -= SuccessOrFailure;
		}
	}

	private void SuccessOrFailure( bool s )
	{
		if( s )
		{
			if(success != null) audio.PlayOneShot( success );
			ShowMessageForDuration( "SUCCESS", timeOnScreen, Color.green);
		}
		else
		{
			if(failure != null) audio.PlayOneShot( failure );
			ShowMessageForDuration( "FAILURE", timeOnScreen, Color.red);
		}
		
		StartCoroutine( TurnOffText(timeOnScreen) );
	}
}
