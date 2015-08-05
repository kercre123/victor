using System;
using System.Windows.Forms;
using System.Drawing;

namespace AnimationTool
{
    public class IPForm : Form
    {
        private Label durationLabel;
        private TextBox textBox;

        private Button button;

        private RobotEngineManager robotEngineManager;

        public IPForm()
        {
            FormBorderStyle = FormBorderStyle.FixedToolWindow;
            MaximizeBox = false;
            MinimizeBox = false;

            InitializeComponent();
        }

        public DialogResult Open(Point location, RobotEngineManager robotEngineManager)
        {
            StartPosition = FormStartPosition.Manual;
            Location = location;
            this.robotEngineManager = robotEngineManager;

            if (Properties.Settings.Default.IPAddress != null)
            {
                this.textBox.Text = Properties.Settings.Default.IPAddress;
            }

            return ShowDialog();
        }

        private void InitializeComponent()
        {
            this.textBox = new System.Windows.Forms.TextBox();
            this.durationLabel = new System.Windows.Forms.Label();
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
            // durationLabel
            // 
            this.durationLabel.AutoSize = true;
            this.durationLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationLabel.Location = new System.Drawing.Point(12, 15);
            this.durationLabel.Name = "durationLabel";
            this.durationLabel.Size = new System.Drawing.Size(62, 13);
            this.durationLabel.TabIndex = 1;
            this.durationLabel.Text = "Engine IP";
            // 
            // button
            // 
            this.button.Location = new System.Drawing.Point(86, 38);
            this.button.Name = "button";
            this.button.Size = new System.Drawing.Size(75, 23);
            this.button.TabIndex = 3;
            this.button.Text = "Connect";
            this.button.UseVisualStyleBackColor = true;
            this.button.Click += new System.EventHandler(this.Button_Click);
            // 
            // IPForm
            // 
            this.ClientSize = new System.Drawing.Size(185, 67);
            this.Controls.Add(this.button);
            this.Controls.Add(this.durationLabel);
            this.Controls.Add(this.textBox);
            this.Name = "IPForm";
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        private void textBox_TextChanged(object sender, EventArgs e)
        {
            robotEngineManager.SetEngineIP(this.textBox.Text);
        }

        private void Button_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default["IPAddress"] = this.textBox.Text;
            Properties.Settings.Default.Save();
            robotEngineManager.Connect();
            Close();
        }
    }
}
