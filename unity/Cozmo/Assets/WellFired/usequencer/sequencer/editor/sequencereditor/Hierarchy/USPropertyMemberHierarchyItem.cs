using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Reflection;

namespace WellFired
{
	[Serializable]
	public class USPropertyMemberHierarchyItem : IUSHierarchyItem 
	{
		public override bool ShouldDisplayMoreButton
		{
			get { return true; }
			set { ; }
		}

		[SerializeField]
		public string Prefix
		{
			get;
			set;
		}
	
		[SerializeField]
		public string PostFix
		{
			get;
			set;
		}
	
		[SerializeField]
		public int CurveIndex
		{
			get;
			set;
		}
	
		[SerializeField]
		private Texture colorIndicatorTexture;
		private Texture ColorIndicatorTexture
		{
			get
			{
				if(colorIndicatorTexture)
					return colorIndicatorTexture;
	
				colorIndicatorTexture = Resources.Load("CurveColourIndicator", typeof(Texture)) as Texture;
				return colorIndicatorTexture;
			}
			set { ; }
		}
		
		[SerializeField]
		private USInternalCurve curve;
		public USInternalCurve Curve
		{
			get { return curve; }
			set { curve = value; }
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
	
			if(IsSelected)
				GUI.Box(new Rect(0, FloatingBackgroundRect.y, FloatingBackgroundRect.width + FloatingBackgroundRect.x, FloatingBackgroundRect.height), "");
	
			using(IsSelected == true ? new WellFired.Shared.GUIChangeColor(AnimationCurveEditorUtility.GetCurveColor(CurveIndex)) : null)
			{
				GUIContent displayContent = new GUIContent(String.Format("{0}{1}", Prefix, PostFix), ColorIndicatorTexture);
				GUI.Label(FloatingBackgroundRect, displayContent);
			}
		}
	}
}