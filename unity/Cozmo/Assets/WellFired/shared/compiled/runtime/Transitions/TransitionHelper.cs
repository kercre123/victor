using UnityEngine;
using System.Collections;
using System;
using System.Reflection;

namespace WellFired.Shared
{
	public static class TransitionHelper 
	{
		private static Type gameViewType;
		private static MethodInfo getMainGameView;
		private static MethodInfo repaintAllMethod;
		private static PropertyInfo gameViewRenderRect;
		private static FieldInfo shownResolution;
		private static object mainGameView;

		private static Type GameViewType
		{
			get
			{
				if(gameViewType == null)
					gameViewType = System.Type.GetType("UnityEditor.GameView,UnityEditor");

				return gameViewType;
			}
		}
		
		private static MethodInfo GetMainGameView
		{
			get
			{
				if(getMainGameView == null)
					getMainGameView = WellFired.Shared.PlatformSpecificFactory.ReflectionHelper.GetNonPublicStaticMethod(GameViewType, "GetMainGameView");

				return getMainGameView;
			}
		}

		private static MethodInfo RepaintAllMethod
		{
			get
			{
				if(repaintAllMethod == null)
					repaintAllMethod = WellFired.Shared.PlatformSpecificFactory.ReflectionHelper.GetMethod(GameViewType, "Repaint");

				return repaintAllMethod;
			}
		}

		private static PropertyInfo GameViewRenderRect
		{
			get
			{
				if(gameViewRenderRect == null)
					gameViewRenderRect = WellFired.Shared.PlatformSpecificFactory.ReflectionHelper.GetNonPublicInstanceProperty(GameViewType, "gameViewRenderRect");

				return gameViewRenderRect;
			}
		}
		
		private static FieldInfo ShownResolution
		{
			get
			{
				if(shownResolution == null)
					shownResolution = WellFired.Shared.PlatformSpecificFactory.ReflectionHelper.GetNonPublicInstanceField(GameViewType, "m_ShownResolution");
				
				return shownResolution;
			}
		}

		private static object MainGameView
		{
			get
			{
				if(mainGameView == null)
					mainGameView = GetMainGameView.Invoke(null, null);

				return mainGameView;
			}
		}

		public static Vector2 MainGameViewSize
		{
			get
			{
				if(Application.isEditor)
				{
					var Res = (Vector2)ShownResolution.GetValue(MainGameView);

					if(Res == Vector2.zero)
					{
						var actualRes = GameViewRenderRect.GetValue(MainGameView, null);
						var rect = (Rect)actualRes;
						Res = rect.size;
					}

					return Res;
				}
				else
				{
					return new Vector2(Screen.width, Screen.height);
				}
			}
			set { ; }
		}

		public static float DefaultTransitionTimeFor(WellFired.Shared.TypeOfTransition transitionType)
		{
			switch(transitionType)
			{
			case TypeOfTransition.Cut:
				return 0.0f;
			default:
				return 2.0f;
			}
		}

		public static void ForceGameViewRepaint()
		{
			if(Application.isEditor)
			{
				RepaintAllMethod.Invoke(MainGameView, null);
			}
		}
	}
}