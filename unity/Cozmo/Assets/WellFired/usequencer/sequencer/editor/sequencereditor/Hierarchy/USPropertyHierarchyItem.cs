using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Reflection;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public class USPropertyHierarchyItem : IUSHierarchyItem 
	{
		public override bool ShouldDisplayMoreButton
		{
			get { return false; }
			set { ; }
		}

		public override int SortOrder
		{
			get 
			{ 
				var order = 0;
				var local = SortName.Contains("local");


				if(SortName.Contains("position"))
					order = -3;
				
				if(SortName.Contains("rotation"))
					order = -1;
				
				if(SortName.Contains("euler"))
					order = -2;

				if(local)
					order -= 3;

				return order; 
			}
		}
		
		public override string SortName
		{
			get 
			{ 
				return PropertyFieldInfo.ReadableName; 
			}
		}

		[SerializeField]
		public PropertyFieldInfo PropertyFieldInfo
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
		
		[SerializeField]
		private bool isSelected;
		public bool IsSelected
		{
			get { return isSelected; }
			set 
			{
				if(isSelected != value)
					USUndoManager.PropertyChange(this, "Alter Displayed Curves"); 
				isSelected = value; 
			}
		}

		public string MappedType 
		{
			get;
			set;
		}
	
		public bool HasChildCurves
		{
			get
			{
				foreach(var child in Children)
				{
					if(((USPropertyMemberHierarchyItem)child).Curve)
						return true;
				}
				return false;
			}
			private set { ; }
		}
	
		public override void DoGUI(int depth)
		{
			GUILayout.Box("", FloatingBackground, GUILayout.Width(FloatingWidth), GUILayout.Height(17.0f));
			
			if(Event.current.type == EventType.Repaint)
			{	
				IsSelected = Children.Any(element => ((USPropertyMemberHierarchyItem)element).IsSelected);
				Rect lastRect = GUILayoutUtility.GetLastRect();
				lastRect.x += GetXOffsetForDepth(depth);
				lastRect.width -= GetXOffsetForDepth(depth);
				FloatingBackgroundRect = lastRect;
			}
	
			if(IsSelected)
				GUI.Box(new Rect(0, FloatingBackgroundRect.y, FloatingBackgroundRect.width + FloatingBackgroundRect.x, FloatingBackgroundRect.height), "");
			
			bool newExpandedState = IsExpanded;
			bool wasLabelPressed;

			newExpandedState = USEditor.FoldoutLabel(FloatingBackgroundRect, IsExpanded, PropertyFieldInfo.ReadableName, out wasLabelPressed);

			if(newExpandedState != IsExpanded)
			{
				USUndoManager.PropertyChange(this, "Foldout");
				IsExpanded = newExpandedState;
			}
			
			Rect addRect = FloatingBackgroundRect;
			float padding = 1.0f;
			float addWidth = 22.0f;
			addRect.x = addRect.x + addRect.width - addWidth - padding;
			addRect.width = addWidth;
			if(!HasChildCurves)
			{
				if(GUI.Button(addRect, "+"))
					AddProperty();
			}
			else
			{
				using(new WellFired.Shared.GUIChangeColor(Color.red))
				{
					if(GUI.Button(addRect, "-"))
						RemoveProperty();
				}
			}
		}

		public bool IsPropertyUsingCurrentValue()
		{
			string preFix = PropertyFieldInfo.Name;
			USPropertyInfo propertyInfo = PropertyTimeline.GetProperty(preFix, PropertyFieldInfo.Component);
			return propertyInfo.curves.Any(curve => curve.UseCurrentValue);
		}

		public void ToggleUseCurrentValue()
		{
			string preFix = PropertyFieldInfo.Name;
			USPropertyInfo propertyInfo = PropertyTimeline.GetProperty(preFix, PropertyFieldInfo.Component);
			foreach(var curve in propertyInfo.curves)
				curve.UseCurrentValue = !curve.UseCurrentValue;
		}

		private void AddProperty()
		{
			USPropertyInfo usPropertyInfo = ScriptableObject.CreateInstance<USPropertyInfo>();
	
			USUndoManager.RegisterCreatedObjectUndo(usPropertyInfo, "Add Curve");
			USUndoManager.PropertyChange(PropertyTimeline, "Add Curve");
			USUndoManager.PropertyChange(this, "Add Curve");
			foreach(var child in Children)
				USUndoManager.PropertyChange(child, "Add Curve");
			
			object propertyValue = null;
			usPropertyInfo.Component = PropertyFieldInfo.Component;
	
			if(PropertyFieldInfo.Property != null)
			{
				usPropertyInfo.propertyInfo = PropertyFieldInfo.Property;
				propertyValue = PropertyFieldInfo.Property.GetValue(PropertyFieldInfo.Component, null);
			}
			else if(PropertyFieldInfo.Field != null)
			{
				usPropertyInfo.fieldInfo = PropertyFieldInfo.Field;
				propertyValue = PropertyFieldInfo.Field.GetValue(PropertyFieldInfo.Component);
			}
	
			usPropertyInfo.InternalName = MappedType;
			usPropertyInfo.CreatePropertyInfo(USPropertyInfo.GetMappedType(propertyValue.GetType()));
			usPropertyInfo.AddKeyframe(propertyValue, 0.0f, CurveAutoTangentModes.None);
			usPropertyInfo.AddKeyframe(propertyValue, PropertyTimeline.Sequence.Duration, CurveAutoTangentModes.None);
			PropertyTimeline.AddProperty(usPropertyInfo);
			
			for(int curveIndex = 0; curveIndex < usPropertyInfo.curves.Count; curveIndex++)
				((USPropertyMemberHierarchyItem)Children[curveIndex]).Curve = usPropertyInfo.curves[curveIndex];
	
			usPropertyInfo.StoreBaseState();
	
			IsSelected = true;
			foreach(var child in Children)
				((USPropertyMemberHierarchyItem)child).IsSelected = true;
		}
	
		private void RemoveProperty()
		{
			USUndoManager.RegisterCompleteObjectUndo(PropertyTimeline, "Remove Curve");
			USUndoManager.RegisterCompleteObjectUndo(this, "Remove Curve");
			foreach(var child in Children)
				USUndoManager.RegisterCompleteObjectUndo(child, "Remove Curve");
			
			string preFix = PropertyFieldInfo.Name;
			USPropertyInfo propertyInfo = PropertyTimeline.GetProperty(preFix, PropertyFieldInfo.Component);
			
			foreach(var child in Children)
				((USPropertyMemberHierarchyItem)child).Curve = null;
	
			propertyInfo.RestoreBaseState();
			PropertyTimeline.RemoveProperty(propertyInfo);
			USUndoManager.DestroyImmediate(propertyInfo);
			
			IsSelected = false;
			foreach(var child in Children)
				((USPropertyMemberHierarchyItem)child).IsSelected = false;
		}
		
		public override void Initialize(USTimelineBase timeline)
		{
			int numberOfChildren = Children.Count;
			if(PropertyFieldInfo.Property != null)
				numberOfChildren = USPropertyMemberUtility.GetNumberOfMembers(PropertyFieldInfo.Property.PropertyType);
			else if(PropertyFieldInfo.Field != null)
				numberOfChildren = USPropertyMemberUtility.GetNumberOfMembers(PropertyFieldInfo.Field.FieldType);
	
			if(numberOfChildren != Children.Count)
			{
				Children.Clear();
				for(int index = 0; index < numberOfChildren; index++)
				{
					string postFix = "";
					if(PropertyFieldInfo.Property != null)
						postFix = USPropertyMemberUtility.GetAdditionalMemberName(PropertyFieldInfo.Property.PropertyType, index);
					else if(PropertyFieldInfo.Field != null)
						postFix = USPropertyMemberUtility.GetAdditionalMemberName(PropertyFieldInfo.Field.FieldType, index);
					
					string preFix = PropertyFieldInfo.Name;
	
					USPropertyMemberHierarchyItem propertyMemberItem = ScriptableObject.CreateInstance<USPropertyMemberHierarchyItem>();
					propertyMemberItem.PostFix = postFix;
					propertyMemberItem.Prefix = preFix;
					propertyMemberItem.CurveIndex = index;
					propertyMemberItem.Initialize(PropertyTimeline);
	
					USPropertyInfo propertyInfo = PropertyTimeline.GetProperty(preFix, PropertyFieldInfo.Component);
					if(propertyInfo != null)
					{
						propertyMemberItem.Curve = propertyInfo.curves[index];
						propertyInfo.InternalName = MappedType;
					}

					Children.Add(propertyMemberItem as IUSHierarchyItem);
				}
			}
		}
		
		public float PreviousKey(float currentTime)
		{
			var previousKey = 0.0f;

			foreach(var child in Children)
			{
				var curve = ((USPropertyMemberHierarchyItem)child).Curve;

				if(curve == null)
					continue;

				float curvePreviousKeyframeTime = curve.FindPrevKeyframeTime(currentTime);
				if(curvePreviousKeyframeTime > previousKey)
					previousKey = curvePreviousKeyframeTime;
			}

			return previousKey;
		}

		public float NextKey(float currentTime)
		{
			var nextKey = float.MaxValue;
			
			foreach(var child in Children)
			{
				var curve = ((USPropertyMemberHierarchyItem)child).Curve;
				
				if(curve == null)
					continue;
				
				float curveNextKeyframeTime = curve.FindNextKeyframeTime(currentTime);
				if(curveNextKeyframeTime < nextKey)
					nextKey = curveNextKeyframeTime;
			}
			
			return nextKey;
		}
	}
}