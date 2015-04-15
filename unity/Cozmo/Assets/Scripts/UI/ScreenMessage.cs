using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ScreenMessage : MonoBehaviour {

	[SerializeField] protected Text text;

	IEnumerator coroutine = null;

	void Awake () {
		text.text = null;
		text.gameObject.SetActive(true);
	}

	public void ShowMessageForDuration(string message, float time_in_seconds, Color color)
	{
		if(coroutine != null) StopCoroutine(coroutine);

		//text.text = G2U_Message;
		ShowMessage (message, color);

		coroutine = TurnOffText(time_in_seconds);
		StartCoroutine(coroutine);
	}

	public void ShowMessage(string message, Color color)
	{
		text.text = message;
		text.color = color;
	}

	public void KillMessage()
	{
		text.text = string.Empty;
	}

	protected IEnumerator TurnOffText(float duration)
	{
		float time = Time.time + duration;
		
		while( time > Time.time )
		{
			yield return null;
		}
		
		KillMessage();
	}
}
