using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour
{
	[SerializeField]
	protected InputField id;
	[SerializeField]
	protected InputField ip;
	[SerializeField]
	protected Toggle simulated;
	[SerializeField]
	protected Button play;
	[SerializeField]
	protected Text error;

	protected void Awake()
	{
		play.onClick.AddListener( Play );
	}

	protected void Play()
	{
		Debug.Log( "ID: " + id.text );
		Debug.Log( "IP: " + ip.text );
		Debug.Log( "Simulated: " + simulated.isOn );

		//if(  )
		{
			Application.LoadLevel( 1 );
		}
		//else
		{
			//error.text = 
		}
	}
}
