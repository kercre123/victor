using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public class USComponentHierarchyItem : IUSHierarchyItem 
	{
		public override bool ShouldDisplayMoreButton
		{
			get { return true; }
			set { ; }
		}

		[SerializeField]
		public Component Component
		{
			get;
			set;
		}
		
		[SerializeField]
		public USTimelineProperty PropertyTimeline
		{
			get;
			set;
		}
	
		public override void DoGUI(int depth)
		{
			FloatingOnGUI(depth);
		}
		
		protected override void FloatingOnGUI(int depth)
		{
			GUILayout.Box("", FloatingBackground, GUILayout.Width(FloatingWidth), GUILayout.Height(17.0f));
			
			if(Event.current.type == EventType.Repaint)
			{
				Rect lastRect = GUILayoutUtility.GetLastRect();
				lastRect.x += GetXOffsetForDepth(depth);
				lastRect.width -= GetXOffsetForDepth(depth);
				FloatingBackgroundRect = lastRect;
			}

			if(Component == null)
			{
				var missingRect = FloatingBackgroundRect;
				missingRect.x += GetXOffsetForDepth(2);
				GUI.Label(missingRect, "*Missing*");
				IsExpanded = false;
				return;
			}
	
			bool wasLabelPressed = false;
			GUIContent displayableContent = new GUIContent(Component.GetType().Name, EditorGUIUtility.ObjectContent(Component, Component.GetType()).image);
			bool newExpandedState = USEditor.FoldoutLabel(FloatingBackgroundRect, IsExpanded, displayableContent, out wasLabelPressed);
			if(newExpandedState != IsExpanded)
			{
				USUndoManager.PropertyChange(this, "Foldout");
				IsExpanded = newExpandedState;
			}
		}
		 
		public override void Initialize(USTimelineBase timeline)
		{
			var properties = Component.GetType().GetProperties().Where(property => !PropertyFieldInfoUtility.ShouldIgnoreProperty(property, Component));
			var fields = Component.GetType().GetFields().Where(field => !PropertyFieldInfoUtility.shouldIgnoreField(field, Component));
	
			int totalCount = properties.Count() + fields.Count();
			
			// Removal
			if(totalCount < Children.Count)
			{
				var propertiesNotInBoth = Children.Where((hierarchyItem) => ((hierarchyItem as USPropertyHierarchyItem).PropertyFieldInfo != null && !properties.Contains((hierarchyItem as USPropertyHierarchyItem).PropertyFieldInfo.Property)));
				var fieldsNotInBoth = Children.Where((hierarchyItem) => ((hierarchyItem as USPropertyHierarchyItem).PropertyFieldInfo != null && !fields.Contains((hierarchyItem as USPropertyHierarchyItem).PropertyFieldInfo.Field)));

				foreach(var missingProperty in propertiesNotInBoth)
					RemoveChild(missingProperty as IUSHierarchyItem);			
				foreach(var missingField in fieldsNotInBoth)
					RemoveChild(missingField as IUSHierarchyItem);
			}
	
			// Addition
			if(totalCount > Children.Count)
			{
				var extraProperties = properties.Where((property) => !Children.Any((item) => ((item is USPropertyHierarchyItem) && (item as USPropertyHierarchyItem).PropertyFieldInfo && (item as USPropertyHierarchyItem).PropertyFieldInfo.Property == property)));
				var extraFields = fields.Where((field) => !Children.Any((item) => ((item is USPropertyHierarchyItem) && (item as USPropertyHierarchyItem).PropertyFieldInfo && (item as USPropertyHierarchyItem).PropertyFieldInfo.Field == field)));

				foreach(var extraProperty in extraProperties)
				{
					PropertyFieldInfo propertyFieldInfo = ScriptableObject.CreateInstance(typeof(PropertyFieldInfo)) as PropertyFieldInfo;
					propertyFieldInfo.Component = Component;
					propertyFieldInfo.Property = extraProperty;
					
					var mappedType = USPropertyMemberUtility.GetUnityPropertyNameFromUSProperty(propertyFieldInfo.Property.Name, Component);
					if(mappedType == string.Empty)
						continue;
	
					USPropertyHierarchyItem hierarchyItem = ScriptableObject.CreateInstance(typeof(USPropertyHierarchyItem)) as USPropertyHierarchyItem;
					hierarchyItem.PropertyFieldInfo = propertyFieldInfo;
					hierarchyItem.PropertyTimeline = PropertyTimeline;
					hierarchyItem.MappedType = mappedType;
					hierarchyItem.Initialize(PropertyTimeline);

					AddChild(hierarchyItem as IUSHierarchyItem);
				}
				foreach(var extraField in extraFields)
				{
					PropertyFieldInfo propertyFieldInfo = ScriptableObject.CreateInstance(typeof(PropertyFieldInfo)) as PropertyFieldInfo;
					propertyFieldInfo.Component = Component;
					propertyFieldInfo.Field = extraField;
					
					var mappedType = USPropertyMemberUtility.GetUnityPropertyNameFromUSProperty(propertyFieldInfo.Field.Name, Component);
					if(mappedType == string.Empty)
						continue;
					
					USPropertyHierarchyItem hierarchyItem = ScriptableObject.CreateInstance(typeof(USPropertyHierarchyItem)) as USPropertyHierarchyItem;
					hierarchyItem.PropertyFieldInfo = propertyFieldInfo;
					hierarchyItem.PropertyTimeline = PropertyTimeline;
					hierarchyItem.MappedType = mappedType;
					hierarchyItem.Initialize(PropertyTimeline);

					AddChild(hierarchyItem as IUSHierarchyItem);
				}
			}
		}

		public bool HasAnyShownChildren()
		{
			var hasShownChildren = false;
			foreach(var child in Children)
			{
				var propertyHierarchyItem = child as USPropertyHierarchyItem;
				if(propertyHierarchyItem == null)
					continue;

				if(propertyHierarchyItem.IsSelected)
					hasShownChildren = true;
			}

			return hasShownChildren;
		}
		
		public float GetPreviousShownKeyframeTime(float currentTime)
		{
			var previousKeyframeTime = 0.0f;

			foreach(var child in Children)
			{
				var propertyHierarchyItem = child as USPropertyHierarchyItem;
				if(propertyHierarchyItem == null)
					continue;
				
				if(propertyHierarchyItem.IsSelected)
				{
					var propertyPreviousKeyframeTime = propertyHierarchyItem.PreviousKey(currentTime);
					if(previousKeyframeTime < propertyPreviousKeyframeTime)
						previousKeyframeTime = propertyPreviousKeyframeTime;
				}
			}
			
			return previousKeyframeTime;
		}

		public float GetNextShownKeyframeTime(float currentTime)
		{
			var nextKeyframeTime = float.MaxValue;
			
			foreach(var child in Children)
			{
				var propertyHierarchyItem = child as USPropertyHierarchyItem;
				if(propertyHierarchyItem == null)
					continue;
				
				if(propertyHierarchyItem.IsSelected)
				{
					var propertyNextKeyframeTime = propertyHierarchyItem.NextKey(currentTime);
					if(propertyNextKeyframeTime < nextKeyframeTime)
						nextKeyframeTime = propertyNextKeyframeTime;
				}
			}
			
			return nextKeyframeTime;
		}
	}
}