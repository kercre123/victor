using UnityEngine;
using UnityEditor;
using System.Collections;
using System.IO;
using System.Text;

namespace WellFired
{
	public class USLicenseAgreementWindow : EditorWindow 
	{
		#region Member Variables
	#if (USEQ_FREE || USEQ_LICENSE)
		private static string fileContents = "";
		private static Vector2 scrollPosition = Vector2.zero;
		private bool hasAgreed = false;
		public static string editorPrefsAgreement = "uSequencer - Lisence Agreement V" + USUpgradePaths.CurrentVersionNumber;
	#endif
	#if !USEQ_NO_LICENSE
		protected static bool canOpen = false;
	#endif
		#endregion
		
		public static void OpenWindow()
		{
			// According to Unity, products on the asset store must abide by their EULA, I've had to remove this for asset store
			// purchases.
	#if !USEQ_LICENSE
			OpenUSequencer();
	#else
			if(EditorPrefs.HasKey(editorPrefsAgreement))
			{
				canOpen = EditorPrefs.GetBool(editorPrefsAgreement);
				if(canOpen)
				{
					OpenUSequencer();
					return;
				}
			}
			
			USLicenseAgreementWindow window = EditorWindow.GetWindow(typeof(USLicenseAgreementWindow), false, "License", true) as USLicenseAgreementWindow;
		    window.position = new Rect(100, 100, 520, 600);
	#endif
		}
	
		private static void OpenUSequencer()
		{	
	#if USEQ_LICENSE
			// Security Check
			if(!EditorPrefs.HasKey(editorPrefsAgreement) || !EditorPrefs.GetBool(editorPrefsAgreement))
				return;
	#endif
			
			USWindow window = EditorWindow.GetWindow (typeof(USWindow), false, "uSequencer") as USWindow;
			
			window.autoRepaintOnSceneChange = true;
			window.minSize = USWindow.minWindowSize; 
			
			USWelcomeScreenWindow.OpenWindow(false);
		}
		
	#if (USEQ_FREE || USEQ_LICENSE)
		private void OnGUI()
		{
			if(fileContents.Length == 0)
			{
				byte[] bytes = USRuntimeUtility.GetByteResource("uSequencer - License.txt");
				
				if(bytes.Length != 0)
					fileContents = ASCIIEncoding.ASCII.GetString(bytes);
				else
				{
					var textAsset = Resources.Load("uSequencer - License") as TextAsset;
					fileContents = textAsset.text;
				}
			}
			
			if(!EditorPrefs.HasKey(editorPrefsAgreement))
				EditorPrefs.SetBool(editorPrefsAgreement, false);
				
			using(new WellFired.Shared.GUIBeginVertical())
			{
				using(var ScrollView = new WellFired.Shared.GUIBeginScrollView(scrollPosition))
				{
					scrollPosition = ScrollView.Scroll;
					GUI.enabled = false;
					EditorGUILayout.TextArea(fileContents);
					GUI.enabled = true;
				}
	
				using(new WellFired.Shared.GUIBeginHorizontal())
				{
					hasAgreed = GUILayout.Toggle(hasAgreed, "I agree to the terms of this license.");
					if(hasAgreed && GUILayout.Button("Open uSequencer", GUILayout.MaxWidth(120.0f)))
					{
						EditorPrefs.SetBool(editorPrefsAgreement, true);
						OpenUSequencer();
						CloseAgreement();
					}
					if(GUILayout.Button("Close uSequencer", GUILayout.MaxWidth(120.0f)))
						CloseAgreement();
				}
			}
		}
	#endif
		
		private void CloseAgreement()
		{
			Close();
		}
	}
}