using UnityEngine;
using System.Collections;

namespace WellFired
{
	public class AlterPlaybackForASequence : MonoBehaviour 
	{
		public USSequencer sequence = null;
		
		private float runningTime = 0.0f;
		
		// Update is called once per frame
		void Update () 
		{
			runningTime += Time.deltaTime;
				
			if(!sequence || runningTime > 5.0f)
				return;
			
			sequence.PlaybackRate -= (Time.deltaTime * 1.0f);
		}
	}
}