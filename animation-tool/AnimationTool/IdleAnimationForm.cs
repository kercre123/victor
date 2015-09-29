using System;
using System.Windows.Forms;
using System.Drawing;
using System.Net;

namespace AnimationTool
{
    public class IdleAnimationForm : Form
    {
        private Label label;
        private TextBox textBox;

        private Button button;

        public IdleAnimationForm()
        {
            FormBorderStyle = FormBorderStyle.FixedToolWindow;
            MaximizeBox = false;
            MinimizeBox = false;

            InitializeComponent();
        }

        public DialogResult Open(Point location)
        {
            StartPosition = FormStartPosition.Manual;
            Location = location;
            button.Enabled = false;

            if (!string.IsNullOrEmpty(Properties.Settings.Default.idleAnimation))
            {
                this.textBox.Text = Properties.Settings.Default.idleAnimation;
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
            this.textBox.Location = new System.Drawing.Point(95, 12);
            this.textBox.Name = "textBox";
            this.textBox.Size = new System.Drawing.Size(96, 20);
            this.textBox.TabIndex = 0;
            this.textBox.Text = "_LIVE_NO_TWITCH_";
            this.textBox.TextChanged += new System.EventHandler(this.textBox_TextChanged);
            // 
            // label
            // 
            this.label.AutoSize = true;
            this.label.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label.Location = new System.Drawing.Point(2, 15);
            this.label.Name = "label";
            this.label.Size = new System.Drawing.Size(87, 13);
            this.label.TabIndex = 1;
            this.label.Text = "Idle Animation";
            // 
            // button
            // 
            this.button.Location = new System.Drawing.Point(116, 38);
            this.button.Name = "button";
            this.button.Size = new System.Drawing.Size(75, 23);
            this.button.TabIndex = 3;
            this.button.Text = "OK";
            this.button.UseVisualStyleBackColor = true;
            this.button.Click += new System.EventHandler(this.Button_Click);
            // 
            // IdleAnimationForm
            // 
            this.ClientSize = new System.Drawing.Size(204, 67);
            this.Controls.Add(this.button);
            this.Controls.Add(this.label);
            this.Controls.Add(this.textBox);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "IdleAnimationForm";
            this.ShowIcon = false;
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void textBox_TextChanged(object o, EventArgs e)
        {
            if (!string.IsNullOrEmpty(this.textBox.Text) && this.textBox.Text != Properties.Settings.Default.idleAnimation)
            {
                button.Enabled = true;
            }
        }

        private void Button_Click(object o, EventArgs e)
        {
            Properties.Settings.Default["idleAnimation"] = this.textBox.Text;
            RobotEngineMessenger.instance.SendIdleAnimation(this.textBox.Text);
            Close();
        }
    }
}
