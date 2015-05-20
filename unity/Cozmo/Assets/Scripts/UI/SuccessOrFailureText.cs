using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

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

	private void SuccessOrFailure( bool s, RobotActionType action_type )
	{
		if( s )
		{
			if(success != null) AudioManager.PlayOneShot( success );
			ShowMessageForDuration( action_type + " SUCCEEDED", timeOnScreen, Color.green);
		}
		else
		{
			if(failure != null) AudioManager.PlayOneShot( failure );
			ShowMessageForDuration( action_type + " FAILED", timeOnScreen, Color.red);
		}
	}
}
