using System;
using Android.App;
using Android.Content;
using Android.OS;
using Android.Views;
using Android.Webkit;
using Android.Widget;

namespace codelab.Droid.UI.CodeLab {
  [Activity(Label = "CodeLabViewController")]
  public class CodeLabViewController : Activity {

    protected override void OnCreate(Bundle savedInstanceState) {
      base.OnCreate(savedInstanceState);

      SetContentView(Resource.Layout.CodeLabView);
    }

    protected override void OnResume() {
      base.OnResume();

      WebView codeLabWebView = FindViewById<WebView>(Resource.Id.CodeLabWebView);
      codeLabWebView.SetWebViewClient(new WebViewClient());
      codeLabWebView.Settings.JavaScriptEnabled = true;
      codeLabWebView.LoadUrl("file:///android_asset/Scratch/index_vertical.html?locale=en-US");
    }
  }
}
