using UnityEngine;
using UnityEditor;
using System.Collections;
using System.IO;
using System.Text;
using WellFired.Shared;

namespace WellFired
{
	public class USWelcomeScreenWindow : EditorWindow
	{
		public static void OpenWindow(bool force)
		{
			if(!force && EditorPrefs.HasKey(editorPrefsShowWelcome))
			{
				if(!EditorPrefs.GetBool(editorPrefsShowWelcome))
					return;
			}
			
			USWelcomeScreenWindow window = EditorWindow.GetWindow(typeof(USWelcomeScreenWindow), true, "Welcome", true) as USWelcomeScreenWindow;
		    window.position = new Rect(350, 150, 400, 620);
		}
		
		#region Member Variables
		private static string editorPrefsShowWelcome = "uSequencer - Welcome Screen V" + USUpgradePaths.CurrentVersionNumber;
		
		private Texture USeqLogo = null;
		private Texture DocumentationLogo = null;
		private Texture VideosLogo = null;
		private Texture CommunityLogo = null;
		private Texture FeedbackLogo = null;
		private Texture WebsiteLogo = null;
		private Texture BlogLogo = null;
		
		private Texture TwitterLogo = null;
		private Texture YoutubeLogo = null;
		private Texture FacebookLogo = null;
		private Texture GoogleLogo = null;
		
		private GUISkin USeqSkin = null;
		private GUIStyle Heading1Style = null;
		private GUIStyle SubHeading1Style = null;
		private GUIStyle SubHeading2Style = null;
		private GUIStyle NormalStyle = null;
		#endregion
		
		private void OnGUI()
		{
			if(!EditorPrefs.HasKey(editorPrefsShowWelcome))
				EditorPrefs.SetBool(editorPrefsShowWelcome, true);
			
			if(!LoadAssets())
			{
				Debug.LogWarning("There was a problem loading assets, the welcome screen will not display correctly.");
				return;
			}
				
			using(new WellFired.Shared.GUIBeginVertical())	
			{
				int distanceFromEdge = 100;
			
				Rect area = new Rect(0, 0, position.width, 107);
				using(new WellFired.Shared.GUIBeginArea(area, ""))
				{
					using(new WellFired.Shared.GUIBeginHorizontal())
					{
						using(new WellFired.Shared.GUIBeginVertical())
						{
							GUILayout.FlexibleSpace();
							GUILayout.Label("Welcome to uSequencer!", Heading1Style, GUILayout.MinWidth(position.width));
							GUILayout.Space(6);
							GUILayout.Label("You have in your hands a powerful sequencer", SubHeading1Style, GUILayout.MinWidth(position.width));
							GUILayout.Label("we cannot wait to see what you produce.", SubHeading1Style, GUILayout.MinWidth(position.width));
					
							GUILayout.FlexibleSpace();
						}
						GUILayout.FlexibleSpace();
					}
				}
			
				distanceFromEdge = 150;

				area.y = 100;
				area.height = 80;
				using(new WellFired.Shared.GUIBeginArea(area, ""))
				{
					using(new WellFired.Shared.GUIBeginHorizontal())
					{
						GUILayout.FlexibleSpace();
						if(GUILayout.Button(TwitterLogo, GUILayout.Width(60.0f), GUILayout.Height(60.0f)))
							OpenTwitterWindow();
						if(GUILayout.Button(FacebookLogo, GUILayout.Width(60.0f), GUILayout.Height(60.0f)))
							OpenFacebookWindow();
						if(GUILayout.Button(GoogleLogo, GUILayout.Width(60.0f), GUILayout.Height(60.0f)))
							OpenGoogleWindow();
						if(GUILayout.Button(YoutubeLogo, GUILayout.Width(60.0f), GUILayout.Height(60.0f)))
							OpenYoutubeWindow();
						GUILayout.FlexibleSpace();
					}
				}
			
				area.y = 180;
				area.height = 80;
				using(new WellFired.Shared.GUIBeginArea(area, ""))
				{
					using(new WellFired.Shared.GUIBeginHorizontal())
					{
						GUILayout.Space(10);
						if(GUILayout.Button(WebsiteLogo, GUILayout.Width(75.0f), GUILayout.Height(75.0f)))
							OpenWebsiteWindow();
						GUILayout.Space(10);
						using(new WellFired.Shared.GUIBeginVertical())
						{
							GUILayout.Space(10);
							GUILayout.Label("Website", SubHeading2Style, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Space(6);
							GUILayout.Label("The official Well Fired website is packed with ", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Label("information.", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.FlexibleSpace();
						}
						GUILayout.FlexibleSpace();
					}
				}
				
				area.y += area.height;
				using(new WellFired.Shared.GUIBeginArea(area, ""))
				{
					using(new WellFired.Shared.GUIBeginHorizontal())
					{
						GUILayout.Space(10);
						if(GUILayout.Button(BlogLogo, GUILayout.Width(75.0f), GUILayout.Height(75.0f)))
							OpenBlogWindow();
						GUILayout.Space(10);
						using(new WellFired.Shared.GUIBeginVertical())
						{
							GUILayout.Space(4);
							GUILayout.Label("Blog", SubHeading2Style, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Space(6);
							GUILayout.Label("For lots of cool tips and tricks, including technical", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Label("write ups, be sure to check out our development blog!", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.FlexibleSpace();
						}
						GUILayout.FlexibleSpace();
					}
				}
				
				area.y += area.height;
				using(new WellFired.Shared.GUIBeginArea(area, ""))
				{
					using(new WellFired.Shared.GUIBeginHorizontal())
					{
						GUILayout.Space(10);
						if(GUILayout.Button(DocumentationLogo, GUILayout.Width(75.0f), GUILayout.Height(75.0f)))
							OpenDocumentationWindow();
						GUILayout.Space(10);
						using(new WellFired.Shared.GUIBeginVertical())
						{
							GUILayout.Space(4);
							GUILayout.Label("Documentation", SubHeading2Style, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Space(6);
							GUILayout.Label("For a full overview of the uSequencer, including a", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Label("reference for adding your own events, check out our", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Label("documentation", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.FlexibleSpace();
						}
						GUILayout.FlexibleSpace();
					}
				}
				
				area.y += area.height;
				using(new WellFired.Shared.GUIBeginArea(area, ""))
				{
					using(new WellFired.Shared.GUIBeginHorizontal())
					{
						GUILayout.Space(10);
						if(GUILayout.Button(CommunityLogo, GUILayout.Width(75.0f), GUILayout.Height(75.0f)))
							OpenCommunityWindow();
						GUILayout.Space(10);
						using(new WellFired.Shared.GUIBeginVertical())
						{
							GUILayout.Space(10);
							GUILayout.Label("Community", SubHeading2Style, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Space(6);
							GUILayout.Label("The uSequencer community is gorwing daily, visit the", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Label("forum and join in with the conversation", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.FlexibleSpace();
						}
						GUILayout.FlexibleSpace();
					}
				}
			
				area.y += area.height;
				using(new WellFired.Shared.GUIBeginArea(area, ""))
				{
					using(new WellFired.Shared.GUIBeginHorizontal())
					{
						GUILayout.Space(10);
						if(GUILayout.Button(FeedbackLogo, GUILayout.Width(75.0f), GUILayout.Height(75.0f)))
							OpenFeedbackWindow();
						GUILayout.Space(10);
						using(new WellFired.Shared.GUIBeginVertical())
						{
							GUILayout.Space(10);
							GUILayout.Label("Feedback", SubHeading2Style, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Space(6);
							GUILayout.Label("Your feedback is very important to use. If you want a", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.Label("new feature for uSequencer, you can request it here.", NormalStyle, GUILayout.MinWidth(position.width - distanceFromEdge));
							GUILayout.FlexibleSpace();
						}
						GUILayout.FlexibleSpace();
					}
				}
			
				GUILayout.FlexibleSpace();
				bool show = EditorGUILayout.Toggle("Show Welcome Screen?", EditorPrefs.GetBool(editorPrefsShowWelcome), GUILayout.MinWidth(minSize.x));
				EditorPrefs.SetBool(editorPrefsShowWelcome, show);
			}
		}
		
		private bool LoadAssets()
		{
			// Already Loaded.
			if(USeqLogo && DocumentationLogo && VideosLogo && FeedbackLogo && WebsiteLogo)
				return true;
			
			USeqLogo 			= Resources.Load("IconAlt2", typeof(Texture)) as Texture;
			DocumentationLogo 	= Resources.Load("IconDocumentation", typeof(Texture)) as Texture;
			VideosLogo 			= Resources.Load("IconVideos", typeof(Texture)) as Texture;
			FeedbackLogo 		= Resources.Load("IconFeedback", typeof(Texture)) as Texture;
			CommunityLogo 		= Resources.Load("IconCommunity", typeof(Texture)) as Texture;
			WebsiteLogo 		= Resources.Load("IconWebsite", typeof(Texture)) as Texture;
			BlogLogo			= Resources.Load("IconBlog", typeof(Texture)) as Texture;
			TwitterLogo			= Resources.Load("IconTwitter", typeof(Texture)) as Texture;
			FacebookLogo		= Resources.Load("IconFacebook", typeof(Texture)) as Texture;
			GoogleLogo			= Resources.Load("IconGoogle", typeof(Texture)) as Texture;
			YoutubeLogo			= Resources.Load("IconYoutube", typeof(Texture)) as Texture;
			USeqSkin 			= USEditorUtility.USeqSkin;
			
			if(!USeqLogo || !DocumentationLogo || !VideosLogo || !FeedbackLogo || !WebsiteLogo || !USeqSkin || !TwitterLogo || !FacebookLogo || !GoogleLogo || ! YoutubeLogo)
				return false;
			
			Heading1Style = USeqSkin.GetStyle("Heading1");
			if(Heading1Style == null)
				return false;
			
			SubHeading1Style = USeqSkin.GetStyle("SubHeading1");
			if(SubHeading1Style == null)
				return false;
			
			SubHeading2Style = USeqSkin.GetStyle("SubHeading2");
			if(SubHeading2Style == null)
				return false;
			
			NormalStyle = USeqSkin.GetStyle("Normal");
			if(NormalStyle == null)
				return false;
			
			return true;
		}
		
		private void OpenDocumentationWindow()
		{
			Application.OpenURL("http://www.wellfired.com/usequencer.html#documentation_tab");
		}
		
		private void OpenCommunityWindow()
		{
			Application.OpenURL("http://www.wellfired.com/blog/forum");
		}

		private void OpenBlogWindow()
		{
			Application.OpenURL("http://www.wellfired.com/blog");
		}
		
		private void OpenFeedbackWindow()
		{
			Application.OpenURL("http://usequencer.uservoice.com/");
		}
		
		private void OpenWebsiteWindow()
		{
			Application.OpenURL("http://www.wellfired.com/usequencer.html");
		}

		private void OpenTwitterWindow()
		{
			Application.OpenURL("https://www.twitter.com/wellfired");
		}

		private void OpenFacebookWindow()
		{
			Application.OpenURL("https://www.facebook.com/wellfired");
		}

		private void OpenGoogleWindow()
		{
			Application.OpenURL("https://plus.google.com/+WellfiredDevelopment/posts");
		}

		private void OpenYoutubeWindow()
		{
			Application.OpenURL("https://www.youtube.com/channel/UC94MXAjuJT-dSfS03wrZW9Q");
		}
	}
}