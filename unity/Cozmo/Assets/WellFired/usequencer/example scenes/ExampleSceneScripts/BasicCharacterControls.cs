using UnityEngine;
using System.Collections;

namespace WellFired
{
	public class BasicCharacterControls : MonoBehaviour 
	{	
		public USSequencer cutscene = null;
		public float strength = 10.0f;
		
		// Update is called once per frame
		void Update () 
		{
			if(cutscene && cutscene.IsPlaying)
				return;
			
			float localStrength = strength * Time.deltaTime;
			
			if(Input.GetKey(KeyCode.W))
			{
				GetComponent<Rigidbody>().AddRelativeForce(-localStrength, 0.0f, 0.0f);
			}
			if(Input.GetKey(KeyCode.S))
			{
				GetComponent<Rigidbody>().AddRelativeForce(localStrength, 0.0f, 0.0f);
			}
			if(Input.GetKey(KeyCode.A))
			{
				GetComponent<Rigidbody>().AddRelativeForce(0.0f, 0.0f, -localStrength);
			}
			if(Input.GetKey(KeyCode.D))
			{
				GetComponent<Rigidbody>().AddRelativeForce(0.0f, 0.0f, localStrength);
			}
		}
	}
}