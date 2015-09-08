using System;
using System.Windows.Forms;
using System.Drawing;
using System.Net;

namespace AnimationTool
{
    public class IPForm : Form
    {
        private Label label;
        private TextBox textBox;

        private Button button;

        private RobotEngineMessenger robotEngineMessenger;

        public IPForm()
        {
            FormBorderStyle = FormBorderStyle.FixedToolWindow;
            MaximizeBox = false;
            MinimizeBox = false;

            InitializeComponent();
        }

        public DialogResult Open(Point location, RobotEngineMessenger robotEngineManager)
        {
            StartPosition = FormStartPosition.Manual;
            Location = location;
            this.robotEngineMessenger = robotEngineManager;
            button.Enabled = false;

            if (Properties.Settings.Default.IPAddress != null)
            {
                this.textBox.Text = Properties.Settings.Default.IPAddress;
            }

            return ShowDialog();
        }

        private void InitializeComponent()
        {
            this.textBox = new System.Windows.Forms.TextBox();
            this.label = new System.Windows.Forms.Label();
            this.button = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // textBox
            // 
            this.textBox.Location = new System.Drawing.Point(77, 12);
            this.textBox.Name = "textBox";
            this.textBox.Size = new System.Drawing.Size(96, 20);
            this.textBox.TabIndex = 0;
            this.textBox.Text = "127.0.0.1";
            this.textBox.TextChanged += new System.EventHandler(this.textBox_TextChanged);
            // 
            // label
            // 
            this.label.AutoSize = true;
            this.label.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label.Location = new System.Drawing.Point(12, 15);
            this.label.Name = "label";
            this.label.Size = new System.Drawing.Size(62, 13);
            this.label.TabIndex = 1;
            this.label.Text = "Engine IP";
            // 
            // button
            // 
            this.button.Location = new System.Drawing.Point(86, 38);
            this.button.Name = "button";
            this.button.Size = new System.Drawing.Size(75, 23);
            this.button.TabIndex = 3;
            this.button.Text = "OK";
            this.button.UseVisualStyleBackColor = true;
            this.button.Click += new System.EventHandler(this.Button_Click);
            // 
            // IPForm
            // 
            this.ClientSize = new System.Drawing.Size(185, 67);
            this.Controls.Add(this.button);
            this.Controls.Add(this.label);
            this.Controls.Add(this.textBox);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "IPForm";
            this.ShowIcon = false;
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void textBox_TextChanged(object sender, EventArgs e)
        {
            button.Enabled = false;

            try
            {
                IPAddress.Parse(this.textBox.Text);
                button.Enabled = true;
            }
            catch (Exception) { }
        }

        private void Button_Click(object sender, EventArgs e)
        {
			RobotSettings.RobotIPAddress = this.textBox.Text;
            robotEngineMessenger.Reset();
            Close();
        }
    }
}
