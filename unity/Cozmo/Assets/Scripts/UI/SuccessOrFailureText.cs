using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SuccessOrFailureText : MonoBehaviour
{
	[SerializeField] private AudioClip success;
	[SerializeField] private AudioClip failure;
	[SerializeField] private Text text;
	[SerializeField] private float timeOnScreen = 5f;

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
			audio.PlayOneShot( success );
			text.text = "SUCCESS";
			text.color = Color.green;
		}
		else
		{
			audio.PlayOneShot( failure );
			text.text = "FAILURE";
			text.color = Color.red;
		}
		
		StartCoroutine( TurnOffText() );
	}

	protected IEnumerator TurnOffText()
	{
		float time = Time.time + timeOnScreen;
		
		while( time > Time.time )
		{
			yield return null;
		}
		
		text.text = string.Empty;
	}
}
