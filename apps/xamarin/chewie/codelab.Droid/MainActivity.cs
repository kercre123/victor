using Android.App;
using Android.Content;
using Android.Widget;
using Android.OS;

using codelab.Droid.UI.CodeLab;

using System;

namespace codelab
{
  [Activity(Label = "codelab", MainLauncher = true, Icon = "@mipmap/icon")]
  public class MainActivity : Activity {

    protected override void OnCreate(Bundle savedInstanceState) {
      base.OnCreate(savedInstanceState);

      // Set our view from the "main" layout resource
      SetContentView(Resource.Layout.Main);

      Button codeLabButton = FindViewById<Button>(Resource.Id.NewProjectButton);
      codeLabButton.Click += NavigateToCodeLab;
    }

    private void NavigateToCodeLab(object sender, EventArgs ea) {
      StartActivity(new Intent(Application.Context, typeof(CodeLabViewController)));
    }
  }
}

