#if UNITY_3_4 || UNITY_3_5 || UNITY_4_0 || UNITY_4_1 || UNITY_4_2
	#define	 PRE_4_3_UNDO_SYSTEM
#else
	#define	 POST_4_3_UNDO_SYSTEM
#endif

using UnityEngine;
using UnityEditor;
using System.Collections;

namespace WellFired
{
	public static class USUndoManager 
	{
		private static bool hasPushed = false;
	
		public static void BeginChangeCheck()
		{
			if(hasPushed)
				throw new System.Exception("Trying to BeginChangeCheck without EndChangeCheck");
	
			hasPushed = true;
	
			EditorGUI.BeginChangeCheck();
		}
	
		public static bool EndChangeCheck()
		{
			if(!hasPushed)
				throw new System.Exception("Trying to EndChangeCheck without BeginChangeCheck");
			
			hasPushed = false;
	
			return EditorGUI.EndChangeCheck();
		}
	
		public static void PropertyChange(Object changedObject, string sendMessage)
		{
			Undo.RecordObject(changedObject, sendMessage);
		}
		
		public static void PropertiesChange(Object[] changedObjects, string sendMessage)
		{
			Undo.RecordObjects(changedObjects, sendMessage);
		}
	
		public static void DestroyImmediate(Object destroyedObject)
		{
			Undo.DestroyObjectImmediate(destroyedObject);
		}
	
		public static void RegisterCreatedObjectUndo(Object createdObject, string message)
		{
			Undo.RegisterCreatedObjectUndo(createdObject, message);
		}
	
		public static void RegisterCompleteObjectUndo(Object changedObject, string message)
		{
			Undo.RegisterCompleteObjectUndo(changedObject, message);
		}
	}
}