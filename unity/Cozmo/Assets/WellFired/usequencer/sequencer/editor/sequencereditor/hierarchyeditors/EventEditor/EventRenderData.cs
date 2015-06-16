using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace WellFired
{
	[Serializable]
	public class EventRenderData : ScriptableObject
	{
		[SerializeField]
		private Rect renderRect;
		public Rect RenderRect
		{
			get { return renderRect; }
			set { renderRect = value; }
		}
		
		[SerializeField]
		private USEventBase evt;
		public USEventBase Event
		{
			get { return evt; }
			set { evt = value; }
		}
		
		[SerializeField]
		private Vector2 renderPosition;
		public Vector2 RenderPosition
		{
			get { return renderPosition; }
			set { renderPosition = value; }
		}
	
		[SerializeField]
		private USEventBaseEditor eventEditor;
		public USEventBaseEditor EventEditor
		{
			get
			{
				return eventEditor;
			}
			set 
			{
				eventEditor = value;
			}
		}

		public void Initialize(USEventBase evt)
		{
			Event = evt;

			// Use Reflection to find the Function we require and call it.
			Type baseType = typeof(USEventBaseEditor);
			Type foundType = baseType;
			Type[] types = USEditorUtility.GetAllSubTypes(baseType); 
			foreach (Type type in types)
			{
				if (type.IsSubclassOf(baseType))
				{
					foreach (Attribute attr in type.GetCustomAttributes(true))
					{
						CustomUSEditorAttribute eventAttr = attr as CustomUSEditorAttribute;
						
						if(eventAttr == null)
							continue;
						
						if (eventAttr.InspectedType == Event.GetType())
							foundType = type;
					}
				}
			}
			
			eventEditor = ScriptableObject.CreateInstance(foundType) as USEventBaseEditor;
			eventEditor.TargetEvent = Event;
		}
	} 
}