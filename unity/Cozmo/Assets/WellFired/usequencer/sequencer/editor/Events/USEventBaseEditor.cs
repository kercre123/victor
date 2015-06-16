using UnityEngine;
using UnityEditor;
using System;
using System.Reflection;
using System.Collections;

namespace WellFired
{
	/// <summary>
	/// Use this to mark up an <see cref="WellFired.USEventBaseEditor"/> so that the uSequencer editor knows which class to use when displaying an <see cref="WellFired.USEventBase"/> in the uSequencer editor.
	/// </summary>
	public class CustomUSEditorAttribute : Attribute
	{
		/// <summary>
		/// Initializes a new instance of the <see cref="WellFired.CustomUSEditorAttribute"/> class.
		/// </summary>
		/// <param name="myInspectedType">This should be a trype derived from <see cref="WellFired.USEventBase"/></param>
		public CustomUSEditorAttribute(Type myInspectedType)
		{
			inspectedType = myInspectedType;
		}
		
		/// <summary>
		/// Which derived type of <see cref="WellFired.USEventBase"/> are we rendering
		/// </summary>
		private Type inspectedType;
		public Type InspectedType
		{
			get { return inspectedType; }
		}
	}

	/// <summary>
	/// Our base class used when displaying something derived from <see cref="WellFired.USEventBase"/> in the uSequencer editor. Extend this if you need custom rendering support for your custom events.
	/// </summary>
	[Serializable]
	[CustomUSEditor(typeof(USEventBase))]
	public class USEventBaseEditor : Editor 
	{
		[NonSerialized]
		private GUIStyle defaultLabel = GUIStyle.none;
		public GUIStyle DefaultLabel
		{
			get
			{
				if (defaultLabel == GUIStyle.none)
					defaultLabel = USEditorUtility.USeqSkin.GetStyle("Label");
	
				return defaultLabel;
			}
			set { ; }
		}
		
		[SerializeField]
		private USEventBase usEventBase;
		public USEventBase TargetEvent
		{
			get { return usEventBase; }
			set { usEventBase = value; }
		}
	
		private float XScroll
		{
			get;
			set;
		}
	
		private float XScale
		{
			get;
			set;
		}
	
		private float ContainerWidth
		{
			get;
			set;
		}
		
		private float ContainerStart
		{
			get;
			set;
		}
	
		private float SequenceDuration
		{
			get;
			set;
		}
		
		[SerializeField]
		private USequencerFriendlyNameAttribute nameAttribute;
		private USequencerFriendlyNameAttribute NameAttribute
		{
			get
			{
				if(nameAttribute == null)
				{
					foreach (Attribute attr in TargetEvent.GetType().GetCustomAttributes(true))
					{
						if(attr is USequencerFriendlyNameAttribute)
						{
							nameAttribute = attr as USequencerFriendlyNameAttribute;
							break;
						}
					}
				}
	
				if(nameAttribute == null)
					throw new NullReferenceException("Custom Event must have a USequencerFriendlyNameAttribute");
	
				return nameAttribute;
			}
			set { ; }
		}

		/// <summary>
		/// Draws an expanded event in timeline. You probably won't need to alter this behaviour. Instead, look at <see cref="WellFired.USEventBaseEditor.RenderEvent"/>
		/// See <see cref="WellFired.USEventBaseEditor.DrawCollapsedEventInTimeline"/> for the collapsed variant.
		/// </summary>
		/// <returns>The grabbable area for this event.</returns>
		/// <param name="renderingArea">Rendering area.</param>
		/// <param name="containerStart">Our containers left most point.</param>
		/// <param name="containerWidth">Our containers width. (probably the width of the timeline)</param>
		/// <param name="sequenceDuration">The duration of the sequence that this event belongs</param>
		/// <param name="xScale">The current uSequencer window XScale, 1 means we have not zoomed in at all.</param>
		/// <param name="xScroll">The current uSequencer window XScroll.</param>
		public Rect DrawEventInTimeline(Rect renderingArea, float containerStart, float containerWidth, float sequenceDuration, float xScale, float xScroll)
		{
			ContainerStart = containerStart;
			ContainerWidth = containerWidth;
			SequenceDuration = sequenceDuration;
			XScale = xScale;
			XScroll = xScroll;
	
			renderingArea.x += 0.0f;
			renderingArea.y += 1.0f;
			
			float width = USPreferenceWindow.DefaultEventEditorWidth;
			if(TargetEvent.Duration > 0)
				width = ConvertTimeToXPos(TargetEvent.FireTime + TargetEvent.Duration);
				
			renderingArea.width = width;
			renderingArea.height -= 3.0f;
	
			return RenderEvent(renderingArea);
		}
		
		/// <summary>
		/// Draws an collapsed event in timeline. You probably won't need to alter this behaviour. Instead, look at <see cref="WellFired.USEventBaseEditor.RenderCollapsedEvent"/>
		/// See <see cref="WellFired.USEventBaseEditor.DrawEventInTimeline"/> for the expanded variant.
		/// </summary>
		/// <returns>The grabbable area for this event.</returns>
		/// <param name="renderingArea">Rendering area.</param>
		/// <param name="containerStart">Our containers left most point.</param>
		/// <param name="containerWidth">Our containers width. (probably the width of the timeline)</param>
		/// <param name="sequenceDuration">The duration of the sequence that this event belongs</param>
		/// <param name="xScale">The current uSequencer window XScale, 1 means we have not zoomed in at all.</param>
		/// <param name="xScroll">The current uSequencer window XScroll.</param>
		public void DrawCollapsedEventInTimeline(Rect renderingArea, float containerStart, float containerWidth, float sequenceDuration, float xScale, float xScroll)
		{
			ContainerStart = containerStart;
			ContainerWidth = containerWidth;
			SequenceDuration = sequenceDuration;
			XScale = xScale;
			XScroll = xScroll;
	
			renderingArea.x += 0.0f;
			renderingArea.y += 1.0f;
	
			renderingArea.width = 4.0f;
			renderingArea.height -= 3.0f;
	
			RenderCollapsedEvent(renderingArea);
		}

		/// <summary>
		/// Renders the actual event.
		/// </summary>
		/// <returns>The events grabbable area.</returns>
		/// <param name="area">The area that this event will occupy on the uSequencer window.</param>
		public virtual Rect RenderEvent(Rect area)
		{
			area = DrawDurationDefinedBox(area);
			
			using(new WellFired.Shared.GUIBeginArea(area))
				GUILayout.Label(GetReadableEventName(), DefaultLabel);
	
			return area;
		}

		/// <summary>
		/// Renders the actual collapsed event.
		/// </summary>
		/// <returns>The events grabbable area.</returns>
		/// <param name="area">The area that this event will occupy on the uSequencer window.</param>
		public virtual void RenderCollapsedEvent(Rect area)
		{
			DrawDefaultBox(area);
			Rect labelArea = area;
			labelArea.x += 2.0f;
			labelArea.width = 1000.0f;
			 
			using(new WellFired.Shared.GUIBeginArea(labelArea))
				GUILayout.Label(GetReadableEventName(), DefaultLabel);
		}

		/// <summary>
		/// This method draws the default box that most events displayed in the uSequencer window use.
		/// </summary>
		/// <param name="area">The area that this box will occupy.</param>
		public void DrawDefaultBox(Rect area)
		{
			GUI.Box(area, "");
		}
	
		/// <summary>
		/// Draws an event, taking into account the duration.
		/// </summary>
		/// <returns>The events grabbable area.</returns>
		/// <param name="area">The area that this box will occupy.</param>
		public Rect DrawDurationDefinedBox(Rect area)
		{
			if(TargetEvent.Duration > 0.0f) 
			{
				float endPosition = ConvertTimeToXPos(TargetEvent.FireTime + TargetEvent.Duration);
				area.width = endPosition - area.x;
			}
			
			GUI.Box(area, "");
	
			return area;
		}

		/// <summary>
		/// Returns the events readable name, by default, this is specified by default with the <see cref="WellFired.USequencerFriendlyName"/> attribute on a <see cref="WellFired.USEventBase"/>.
		/// </summary>
		/// <returns>The readable event name.</returns>
		public string GetReadableEventName()
		{
			return NameAttribute.FriendlyName;
		}

		/// <summary>
		/// A utility method that will convert a given time in seconds to an X Position on the timeline.
		/// </summary>
		/// <returns>The time in seconds</returns>
		/// <param name="time">The XPosition on the timeline</param>
		protected float ConvertTimeToXPos(float time)
		{
			float xPos = ContainerWidth * (time / SequenceDuration);
			return ContainerStart + ((xPos * XScale) - XScroll);
		}
	}
}