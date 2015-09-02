using System;
using System.Windows.Forms;
using System.Drawing;
using System.Net;

namespace AnimationTool
{
    public class FaceTrackBarForm : ContainerControl
    {
        private TextBox leftTextBox;
        private TrackBar leftTrackBar;
        private Label leftLabel;
        private Label rightLabel;
        private TrackBar rightTrackBar;
        private CheckBox checkBox;
        private TextBox rightTextBox;

        public string LeftLabel
        {
            get
            {
                return leftLabel.Text;
            }

            set
            {
                leftLabel.Text = value;
            }
        }

        public int LeftMinimum
        {
            get
            {
                return leftTrackBar.Minimum;
            }

            set
            {
                leftTrackBar.Minimum = value;
            }
        }

        public int LeftMaximum
        {
            get
            {
                return leftTrackBar.Maximum;
            }

            set
            {
                leftTrackBar.Maximum = value;
            }
        }

        public string RightLabel
        {
            get
            {
                return rightLabel.Text;
            }

            set
            {
                rightLabel.Text = value;
            }
        }

        public int RightMinimum
        {
            get
            {
                return rightTrackBar.Minimum;
            }

            set
            {
                rightTrackBar.Minimum = value;
            }
        }

        public int RightMaximum
        {
            get
            {
                return rightTrackBar.Maximum;
            }

            set
            {
                rightTrackBar.Maximum = value;
            }
        }

        public bool CheckBoxVisible
        {
            get
            {
                return checkBox.Visible && checkBox.Enabled;
            }

            set
            {
                checkBox.Visible = value;
                checkBox.Enabled = value;
                checkBox.Checked = value;
            }
        }

        public FaceTrackBarForm()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            this.leftTextBox = new System.Windows.Forms.TextBox();
            this.leftTrackBar = new System.Windows.Forms.TrackBar();
            this.leftLabel = new System.Windows.Forms.Label();
            this.rightLabel = new System.Windows.Forms.Label();
            this.rightTrackBar = new System.Windows.Forms.TrackBar();
            this.rightTextBox = new System.Windows.Forms.TextBox();
            this.checkBox = new System.Windows.Forms.CheckBox();
            ((System.ComponentModel.ISupportInitialize)(this.leftTrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.rightTrackBar)).BeginInit();
            this.SuspendLayout();
            // 
            // leftTextBox
            // 
            this.leftTextBox.Location = new System.Drawing.Point(200, 20);
            this.leftTextBox.Name = "leftTextBox";
            this.leftTextBox.Size = new System.Drawing.Size(30, 20);
            this.leftTextBox.TabIndex = 0;
            this.leftTextBox.Text = "000";
            this.leftTextBox.TextChanged += new System.EventHandler(this.textBox_TextChanged);
            // 
            // leftTrackBar
            // 
            this.leftTrackBar.LargeChange = 10;
            this.leftTrackBar.Location = new System.Drawing.Point(-5, 25);
            this.leftTrackBar.Margin = new System.Windows.Forms.Padding(0);
            this.leftTrackBar.Maximum = 90;
            this.leftTrackBar.Name = "leftTrackBar";
            this.leftTrackBar.Size = new System.Drawing.Size(200, 45);
            this.leftTrackBar.TabIndex = 0;
            this.leftTrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // leftLabel
            // 
            this.leftLabel.AutoSize = true;
            this.leftLabel.Location = new System.Drawing.Point(0, 0);
            this.leftLabel.Margin = new System.Windows.Forms.Padding(0);
            this.leftLabel.Name = "leftLabel";
            this.leftLabel.Size = new System.Drawing.Size(33, 13);
            this.leftLabel.TabIndex = 3;
            this.leftLabel.Text = "Label";
            // 
            // rightLabel
            // 
            this.rightLabel.AutoSize = true;
            this.rightLabel.Location = new System.Drawing.Point(251, 0);
            this.rightLabel.Name = "rightLabel";
            this.rightLabel.Size = new System.Drawing.Size(33, 13);
            this.rightLabel.TabIndex = 6;
            this.rightLabel.Text = "Label";
            // 
            // rightTrackBar
            // 
            this.rightTrackBar.LargeChange = 10;
            this.rightTrackBar.Location = new System.Drawing.Point(285, 25);
            this.rightTrackBar.Margin = new System.Windows.Forms.Padding(0);
            this.rightTrackBar.Maximum = 90;
            this.rightTrackBar.Name = "rightTrackBar";
            this.rightTrackBar.Size = new System.Drawing.Size(200, 45);
            this.rightTrackBar.TabIndex = 0;
            this.rightTrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // rightTextBox
            // 
            this.rightTextBox.Location = new System.Drawing.Point(254, 20);
            this.rightTextBox.Name = "rightTextBox";
            this.rightTextBox.Size = new System.Drawing.Size(30, 20);
            this.rightTextBox.TabIndex = 4;
            this.rightTextBox.Text = "000";
            // 
            // checkBox
            // 
            this.checkBox.AutoSize = true;
            this.checkBox.Checked = true;
            this.checkBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox.Location = new System.Drawing.Point(235, 20);
            this.checkBox.Name = "checkBox";
            this.checkBox.Size = new System.Drawing.Size(15, 14);
            this.checkBox.TabIndex = 7;
            this.checkBox.UseVisualStyleBackColor = true;
            // 
            // FaceTrackBarForm
            // 
            this.BackColor = System.Drawing.SystemColors.Control;
            this.Controls.Add(this.checkBox);
            this.Controls.Add(this.rightLabel);
            this.Controls.Add(this.rightTrackBar);
            this.Controls.Add(this.rightTextBox);
            this.Controls.Add(this.leftLabel);
            this.Controls.Add(this.leftTrackBar);
            this.Controls.Add(this.leftTextBox);
            this.Name = "FacePanel";
            this.Size = new System.Drawing.Size(480, 50);
            ((System.ComponentModel.ISupportInitialize)(this.leftTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.rightTrackBar)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void textBox_TextChanged(object sender, EventArgs e)
        {
            try
            {
                IPAddress.Parse(this.leftTextBox.Text);
            }
            catch (Exception) { }
        }
    }
}
