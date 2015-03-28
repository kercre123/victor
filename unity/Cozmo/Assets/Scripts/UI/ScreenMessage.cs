using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ScreenMessage : MonoBehaviour {

	[SerializeField] protected Text text;

	// Use this for initialization
	void Start () {
	
	}
	
	// Update is called once per frame
	void Update () {
	}

	public void ShowMessageForDuration(string message, float time_in_seconds, Color color)
	{
		//text.text = G2U_Message;
		ShowMessage (message, color);
		StartCoroutine(TurnOffText(time_in_seconds));
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
