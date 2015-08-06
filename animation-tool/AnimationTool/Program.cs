using System;
using System.Windows.Forms;
using System.Threading;
using Anki.Cozmo.ExternalInterface;

namespace AnimationTool
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.ThreadException += LogException;
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }

        static void LogException(object unused, ThreadExceptionEventArgs args)
        {
            Console.Out.WriteLine(args.Exception);
            MessageBox.Show(args.Exception.ToString(), "Fatal Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            Application.Exit();
        }
    }
}
