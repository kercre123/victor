using UnityEngine;
using UnityEngine.UI;

namespace WellFired
{
	/// <summary>
	/// You can add this component to a Unity UI slider to hookup slider support
	/// </summary>
	[RequireComponent(typeof(Slider))]
	public class SequencePreviewer : MonoBehaviour 
	{
		[SerializeField]
		private USSequencer sequenceToPreview;
		
		private void Start() 
		{
			var slider = GetComponent<Slider>();
			
			if(!slider)
			{
				Debug.LogError("The component SequenceSlider button must be added to a Unity Slider");
				return;
			}
			
			if(!sequenceToPreview)
			{
				Debug.LogError("The Sequence to Preview field must be hooked up in the Inspector");
				return;
			}

			slider.onValueChanged.AddListener((value) => SetRunningTime(value));
		}
		
		private void SetRunningTime(float runningTime)
		{
			sequenceToPreview.RunningTime = runningTime * sequenceToPreview.Duration;
		}

		private void Update()
		{
			var slider = GetComponent<Slider>();
			slider.value = sequenceToPreview.RunningTime / sequenceToPreview.Duration;
		}
	}
}