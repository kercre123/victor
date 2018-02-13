// WARNING
//
// This file has been generated automatically by Visual Studio from the outlets and
// actions declared in your storyboard file.
// Manual changes to this file will not be maintained.
//
using Foundation;
using System;
using System.CodeDom.Compiler;
using UIKit;

namespace codelab.iOS.UI.CodeLab
{
    [Register ("CodeLabViewController")]
    partial class CodeLabViewController
    {
        [Outlet]
        [GeneratedCode ("iOS Designer", "1.0")]
        UIKit.UIWebView CodeLabWebView { get; set; }

        void ReleaseDesignerOutlets ()
        {
            if (CodeLabWebView != null) {
                CodeLabWebView.Dispose ();
                CodeLabWebView = null;
            }
        }
    }
}