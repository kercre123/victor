#if WELLFIRED_INTERNAL
#define DEBUG_PROPERTYTIMELINE
#endif

using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace WellFired
{
	[Serializable]
	[USCustomTimelineHierarchyItem(typeof(USTimelineProperty), "Property Timeline", 1)]
	public class USPropertyTimelineHierarchyItem : IUSTimelineHierarchyItem 
	{
		public override bool ShouldDisplayMoreButton
		{
			get { return true; }
			set { ; }
		}

		public override Transform PingableObject
		{
			get { return PropertyTimeline.AffectedObject; }
			set { ; }
		}

		[SerializeField]
		public USTimelineProperty PropertyTimeline
		{
			get { return BaseTimeline as USTimelineProperty; }
			set { BaseTimeline = value; }
		}
		
		[SerializeField]
		private AnimationCurveEditor _curveEditor;
		public AnimationCurveEditor CurveEditor
		{
			get { return _curveEditor; }
			set { _curveEditor = value; }
		}
	
		[SerializeField]
		public List<ISelectableContainer> ISelectableContainers
		{
			get;
			set;
		}
	
		[SerializeField]
		public float MaxHeight
		{
			get;
			private set;
		}
	
		[SerializeField]
		private USHierarchy hierarchy;
		private USHierarchy USHierarchy
		{
			get { return hierarchy; }
			set { hierarchy = value; }
		}
	
		public override void OnEnable()
		{
			base.OnEnable();
	
			if(USHierarchy == null)
				USHierarchy = ScriptableObject.CreateInstance<USHierarchy>();
	
			if(CurveEditor == null)
				CurveEditor = ScriptableObject.CreateInstance<AnimationCurveEditor>();

			USHierarchy.OnItemContextClicked -= OnItemContextClicked;
			USHierarchy.OnItemContextClicked += OnItemContextClicked;

			Undo.postprocessModifications -= PostProcess;
			Undo.postprocessModifications += PostProcess;
	
			if(ISelectableContainers == null)
			{
				ISelectableContainers = new List<ISelectableContainer>();
				ISelectableContainers.Add(CurveEditor);
			}
		}
		
		public override void DoGUI(int depth)
		{
			BaseTimeline.ShouldRenderGizmos = IsExpanded && USPreferenceWindow.RenderHierarchyGizmos;

			USHierarchy.OnItemClicked -= OnItemClicked;
			USHierarchy.OnItemClicked += OnItemClicked;
	
			using(new WellFired.Shared.GUIBeginHorizontal())
			{
				using(new WellFired.Shared.GUIBeginVertical(GUILayout.MaxWidth(FloatingWidth)))
				{
					FloatingOnGUI(depth);
	
					if(IsExpanded)
					{
						USHierarchy.EditorWindow = EditorWindow;
						USHierarchy.FloatingWidth = FloatingWidth;
						USHierarchy.DoGUI(depth + 1);
					}
				}
	
				if(Event.current.type == EventType.Repaint)
				{
					float newMaxHeight = GUILayoutUtility.GetLastRect().height;
					
					if(MaxHeight != newMaxHeight)
					{
						EditorWindow.Repaint();
						MaxHeight = newMaxHeight;
					}
				}
	
				ContentOnGUI();
			}
			
			if(Event.current.commandName == "UndoRedoPerformed")
				return;
	
			if(CurveEditor.AreCurvesDirty)
			{
				PropertyTimeline.Process(PropertyTimeline.Sequence.RunningTime, PropertyTimeline.Sequence.PlaybackRate);
				CurveEditor.AreCurvesDirty = false;
			}
		}
		
		private void OnItemContextClicked(IUSHierarchyItem item)
		{ 
			if(!(item is USPropertyHierarchyItem))
				return;

			var propertyHierarchyItem = item as USPropertyHierarchyItem;

			GenericMenu contextMenu = new GenericMenu();
			contextMenu.AddItem(new GUIContent("Use Current Value"), propertyHierarchyItem.IsPropertyUsingCurrentValue(), () => propertyHierarchyItem.ToggleUseCurrentValue());
			contextMenu.ShowAsContext();
		}
		
		protected override void FloatingOnGUI(int depth)
		{
			GUILayout.Box("", FloatingBackground, GUILayout.MaxWidth(FloatingWidth), GUILayout.Height(17.0f));
			
			if(Event.current.type == EventType.Repaint)
			{
				Rect lastRect = GUILayoutUtility.GetLastRect();
				lastRect.x += GetXOffsetForDepth(depth);
				lastRect.width -= GetXOffsetForDepth(depth);
				if(FloatingBackgroundRect != lastRect)
				{
					EditorWindow.Repaint();
					FloatingBackgroundRect = lastRect;
				}
			}
	
			bool wasLabelPressed = false;
			bool newExpandedState = USEditor.FoldoutLabel(FloatingBackgroundRect, IsExpanded, PropertyTimeline.name, out wasLabelPressed);
			if(newExpandedState != IsExpanded)
			{
				USUndoManager.PropertyChange(this, "Foldout");
				IsExpanded = newExpandedState;
				EditorWindow.Repaint();
			}

			base.FloatingOnGUI(depth);
		}
		
		private void ContentOnGUI()
		{
			AlterDisplayCurves();
	
			CurveEditor.MaxHeight = MaxHeight;
			CurveEditor.Duration = PropertyTimeline.Sequence.Duration;
			if(IsExpanded)
				CurveEditor.OnGUI();
			else
				CurveEditor.OnCollapsedGUI();
	
			if(Event.current.type == EventType.Repaint)
				ContentBackgroundRect = GUILayoutUtility.GetLastRect();
		}
		
		private void OnItemClicked(IUSHierarchyItem item)
		{
			USPropertyHierarchyItem propertyTimelineHierarchyItem = item as USPropertyHierarchyItem;
			USPropertyMemberHierarchyItem propertyMemberHierarchyItem = item as USPropertyMemberHierarchyItem;
	
			USUndoManager.PropertyChange(this, "Selected Curve");
			foreach(var child in Children)
				USUndoManager.PropertyChange(child, "Selected Curve");

			if(propertyTimelineHierarchyItem)
			{
				propertyTimelineHierarchyItem.IsSelected = !propertyTimelineHierarchyItem.IsSelected;
				bool anyNotSelected = propertyTimelineHierarchyItem.Children.Cast<USPropertyMemberHierarchyItem>().Any(element => !element.IsSelected);
				foreach(var child in propertyTimelineHierarchyItem.Children)
				{
					if(((USPropertyMemberHierarchyItem)child).Curve == null)
						continue;
	
					if(anyNotSelected)
						((USPropertyMemberHierarchyItem)child).IsSelected = true;
					else
						((USPropertyMemberHierarchyItem)child).IsSelected = false;
				}
			}
			if(propertyMemberHierarchyItem)
				if(propertyMemberHierarchyItem.Curve != null)
					propertyMemberHierarchyItem.IsSelected = !propertyMemberHierarchyItem.IsSelected;
		}
		
		public override List<ISelectableContainer> GetISelectableContainers()
		{
			return ISelectableContainers;
		}
		
		public override void Initialize(USTimelineBase timeline)
		{
			base.Initialize(timeline);

			if(PropertyTimeline.AffectedObject == null)
				return;

			var components = PropertyTimeline.AffectedObject.GetComponents<Component>().ToList();
			if(ShowOnlyAnimated)
				components = components.Where(component => PropertyTimeline.HasPropertiesForComponent(component)).ToList();
	
			// Removal
			if(components.Count() < USHierarchy.RootItems.Count)
			{
				var notInBoth = USHierarchy.RootItems.Where((hierarchyItem) => !components.Contains((hierarchyItem as USComponentHierarchyItem).Component));
				foreach(var missingComponent in notInBoth)
					USHierarchy.RootItems.Remove(missingComponent as IUSHierarchyItem);
			}
	
			// Addition
			if(components.Count() > USHierarchy.RootItems.Count)
			{
				var notInBoth = components.Where((component) => !USHierarchy.RootItems.Any((item) => ((item is USComponentHierarchyItem) && (item as USComponentHierarchyItem).Component == component)));

				foreach(var extraComponent in notInBoth)
				{
					if(!PropertyFieldInfoUtility.IsValidComponent(extraComponent.GetType()))
						continue;

					USComponentHierarchyItem hierarchyItem = ScriptableObject.CreateInstance(typeof(USComponentHierarchyItem)) as USComponentHierarchyItem;
					hierarchyItem.Component = extraComponent;
					hierarchyItem.PropertyTimeline = PropertyTimeline;
					hierarchyItem.Initialize(PropertyTimeline);
					USHierarchy.RootItems.Add(hierarchyItem as IUSHierarchyItem);
				}
			}
		}
	
		public override void CheckConsistency()
		{
			CurveEditor.EditorWindow = EditorWindow;
			CurveEditor.CurrentXMarkerDist = CurrentXMarkerDist;
			CurveEditor.XScale = XScale;
			CurveEditor.XScroll = XScroll;
			CurveEditor.YScroll = YScroll;
			base.CheckConsistency();
		}
	
		public override void StoreBaseState()
		{
			foreach(var property in PropertyTimeline.Properties)
				property.StoreBaseState();
	
			base.StoreBaseState();
		}
		
		public override void RestoreBaseState()
		{
			if(PropertyTimeline == null)
			{
#if DEBUG_PROPERTYTIMELINE
				Debug.LogWarning("This should never be null, unless doing an undo");
#endif
				return;
			}

			foreach(var property in PropertyTimeline.Properties)
			{
				if(property == null)
				{
					Debug.LogWarning("Something is null on : " + PropertyTimeline, PropertyTimeline.gameObject);
					continue;
				}
				property.RestoreBaseState();
			}

			base.RestoreBaseState();
		}
	
		private void AlterDisplayCurves()
		{
			var childPropertyMembers = USHierarchy.RootItems.SelectMany(root => root.Children)
			.SelectMany(child => child.Children)
			.Where(element => element is USPropertyMemberHierarchyItem && ((USPropertyMemberHierarchyItem)element).IsSelected).ToList();

			var childCurves = childPropertyMembers.Select(element => ((USPropertyMemberHierarchyItem)element).Curve);
			List<USInternalCurve> curves = childCurves.Cast<USInternalCurve>().ToList();
			
			for(int index = 0; index < childPropertyMembers.Count(); index++)
				((USPropertyMemberHierarchyItem)childPropertyMembers[index]).CurveIndex = index;

			bool curveDifference = false;
			// Curves different?
			if(CurveEditor.Curves == null)
				curveDifference = true;
			else if(CurveEditor.Curves.Count != curves.Count || curves.Intersect(CurveEditor.Curves).Count() != curves.Count)
				curveDifference = true;

			if(curveDifference)
			{
				CurveEditor.Curves = curves;
				if(Selection.activeTransform != PingableObject)
				{
					EditorGUIUtility.PingObject(PingableObject);
					Selection.activeTransform = PingableObject;
				}
			}
		}
		
		public override void Expand()
		{
			base.Expand();
			USHierarchy.ExpandAll();
		}
		
		public override void HideAll()
		{
			base.HideAll();
			USHierarchy.HideAll();
		}
		
		public override void ExternalModification()
		{
			base.ExternalModification();
			CurveEditor.RebuildCurvesOnNextGUI = true;
			CurveEditor.AreCurvesDirty = true;
			EditorWindow.Repaint();
		}

		public float GetPreviousShownKeyframeTime(float runningTime)
		{					
			var previousPropertyTime = 0.0f;
			foreach(var rootItem in USHierarchy.RootItems)
			{
				var componentContainer = rootItem as USComponentHierarchyItem;
				
				if(componentContainer == null || !componentContainer.HasAnyShownChildren())
					continue;
				
				var componentPreviousKeyframeTime = componentContainer.GetPreviousShownKeyframeTime(runningTime);
				
				if(componentPreviousKeyframeTime > previousPropertyTime)
					previousPropertyTime = componentPreviousKeyframeTime;
			}

			return previousPropertyTime;
		}

		public float GetNextShownKeyframeTime(float runningTime)
		{
			var nextPropertyTime = float.MaxValue;
			foreach(var rootItem in USHierarchy.RootItems)
			{
				var componentContainer = rootItem as USComponentHierarchyItem;
				
				if(componentContainer == null || !componentContainer.HasAnyShownChildren())
					continue;
				
				var componentNextKeyframeTime = componentContainer.GetNextShownKeyframeTime(runningTime);
				if(componentNextKeyframeTime < nextPropertyTime)
					nextPropertyTime = componentNextKeyframeTime;
			}
			
			return nextPropertyTime;
		}

		private UndoPropertyModification[] PostProcess(UndoPropertyModification[] modifications)
		{
			if(PropertyTimeline && PropertyTimeline.Sequence && PropertyTimeline.Sequence.IsPlaying)
				return modifications;

			if(!WellFired.Animation.IsInAnimationMode)
				return modifications;

			if(!USPreferenceWindow.AutoKeyframing)
				return modifications;

			if(USWindow.IsScrubbing)
				return modifications;

			try
			{
				var propertyModifications = ExtractRecordableModifications(modifications);
				foreach(var modifiedProperty in propertyModifications)
				{
					var modifiedCurves = modifiedProperty.GetModifiedCurvesAtTime(PropertyTimeline.Sequence.RunningTime);
					foreach(var modifiedCurve in modifiedCurves)
						CurveEditor.AddKeyframeAtTime(modifiedCurve);
				}
			}
			catch(Exception e)
			{
				Debug.Log(e);
			}

			return modifications;
		}

		private IEnumerable<USPropertyInfo> ExtractRecordableModifications(UndoPropertyModification[] modifications)
		{
			var extractedModifications = new List<USPropertyInfo>();
			foreach(var modification in modifications)
			{
				var binding = default(EditorCurveBinding);
				AnimationUtility.PropertyModificationToEditorCurveBinding(modification.propertyModification, PropertyTimeline.AffectedObject.gameObject, out binding);

				foreach(var propertyInfo in PropertyTimeline.Properties)
				{
					if(!binding.propertyName.Contains(propertyInfo.InternalName))
						continue;

					extractedModifications.Add(propertyInfo);
					break;
				}
			}

			return extractedModifications;
		}
	}
}