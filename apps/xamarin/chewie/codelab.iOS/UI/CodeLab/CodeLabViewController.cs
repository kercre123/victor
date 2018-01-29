using Foundation;
using System;
using System.IO;
using UIKit;

namespace codelab.iOS.UI.CodeLab
{
    public partial class CodeLabViewController : UIViewController {

        public CodeLabViewController (IntPtr handle) : base (handle) {
        }

        public override void ViewDidLoad() {
            string filePath = "Content/Scratch/index_vertical.html?locale=en-US";
            string localHtmlUrl = Path.Combine(NSBundle.MainBundle.BundlePath, filePath);
            CodeLabWebView.LoadRequest(new NSUrlRequest(new NSUrl(localHtmlUrl)));
        }
    }
}