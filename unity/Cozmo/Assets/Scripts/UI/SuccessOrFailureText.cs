using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

/// <summary>
/// on screen debug text and sfx for displaying the results of a completed or failed AI assisted action
/// </summary>
public class SuccessOrFailureText : ScreenMessage
{
	[SerializeField] private AudioClip success;
	[SerializeField] private AudioClip failure;
	[SerializeField] protected float timeOnScreen = 5f;

	bool showText = true;

	private void OnEnable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.SuccessOrFailure += SuccessOrFailure;
		}

		text.text = string.Empty;

		RefreshSettings();
		OptionsScreen.RefreshSettings += RefreshSettings;
	}

	private void OnDisable()
	{
		if( RobotEngineManager.instance != null )
		{
			RobotEngineManager.instance.SuccessOrFailure -= SuccessOrFailure;
		}
		OptionsScreen.RefreshSettings -= RefreshSettings;
	}

	private void SuccessOrFailure( bool s, RobotActionType action_type )
	{
		if( s )
		{
			if(success != null) AudioManager.PlayOneShot( success );
			if(showText) ShowMessageForDuration( action_type + " SUCCEEDED", timeOnScreen, Color.green);
		}
		else
		{
			if(failure != null) AudioManager.PlayOneShot( failure );
			if(showText) ShowMessageForDuration( action_type + " FAILED", timeOnScreen, Color.red);
		}
	}

	void RefreshSettings() {
		showText = PlayerPrefs.GetInt("ShowDebugInfo", 0) == 1;
		if(!showText) text.text = string.Empty;
	}
}
