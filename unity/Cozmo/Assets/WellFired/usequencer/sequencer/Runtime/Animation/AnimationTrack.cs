using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public class AnimationTrack : ScriptableObject
	{
		[SerializeField]
		private List<AnimationClipData> trackClipList = new List<AnimationClipData>();

		[SerializeField]
		private int layer;

		public int Layer
		{
			get { return layer; }
			set { layer = value; }
		}

		public List<AnimationClipData> TrackClips
		{
			get { return trackClipList; }
			private set { trackClipList = value; }
		}

		public void AddClip(AnimationClipData clipData)
		{
			if(trackClipList.Contains(clipData))
				throw new Exception("Track already contains Clip");

			trackClipList.Add(clipData);
		}
		
		public void RemoveClip(AnimationClipData clipData)
		{
			if(!trackClipList.Contains(clipData))
				throw new Exception("Track doesn't contains Clip");
			
			trackClipList.Remove(clipData);
		}

		private void SortClips()
		{
			trackClipList = trackClipList.OrderBy(trackClip => trackClip.StartTime).ToList();
		}

		public void SetClipData (List<AnimationClipData> animationClipData)
		{
			trackClipList = animationClipData;
		}
	}
}