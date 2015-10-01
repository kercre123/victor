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

        public string LeftLabel { get { return leftLabel.Text; } set { leftLabel.Text = value; } }
        public float LeftValue { get { return leftTrackBar.Value * 0.01f; } set { leftTrackBar.Value = (int)(value * 100); } }
        public int LeftMinimum { get { return leftTrackBar.Minimum; } set { leftTrackBar.Minimum = value; } }
        public int LeftMaximum { get { return leftTrackBar.Maximum; } set { leftTrackBar.Maximum = value; } }

        public string RightLabel { get { return rightLabel.Text; } set { rightLabel.Text = value; } }
        public float RightValue { get { return rightTrackBar.Value * 0.01f; } set { rightTrackBar.Value = (int)(value * 100); } }
        public int RightMinimum { get { return rightTrackBar.Minimum; } set { rightTrackBar.Minimum = value; } }
        public int RightMaximum { get { return rightTrackBar.Maximum; } set { rightTrackBar.Maximum = value; } }

        public bool CheckBoxVisible { get { return checkBox.Visible; } set { checkBox.Visible = value; } }

        public EventHandler OnChanged;

        public FaceTrackBarForm()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FaceTrackBarForm));
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
            this.leftTextBox.Text = "0";
            this.leftTextBox.TextChanged += new System.EventHandler(this.LeftTextBox_Changed);
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
            this.leftTrackBar.ValueChanged += new System.EventHandler(this.LeftTrackBar_Changed);
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
            this.rightTrackBar.ValueChanged += new System.EventHandler(this.RightTrackBar_Changed);
            // 
            // rightTextBox
            // 
            this.rightTextBox.Location = new System.Drawing.Point(254, 20);
            this.rightTextBox.Name = "rightTextBox";
            this.rightTextBox.Size = new System.Drawing.Size(30, 20);
            this.rightTextBox.TabIndex = 4;
            this.rightTextBox.Text = "0";
            this.rightTextBox.TextChanged += new System.EventHandler(this.RightTextBox_Changed);
            // 
            // checkBox
            // 
            this.checkBox.AutoSize = true;
            this.checkBox.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("checkBox.BackgroundImage")));
            this.checkBox.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
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
            this.ClientSize = new System.Drawing.Size(464, 61);
            this.Controls.Add(this.checkBox);
            this.Controls.Add(this.rightLabel);
            this.Controls.Add(this.rightTrackBar);
            this.Controls.Add(this.rightTextBox);
            this.Controls.Add(this.leftLabel);
            this.Controls.Add(this.leftTrackBar);
            this.Controls.Add(this.leftTextBox);
            this.Name = "FaceTrackBarForm";
            ((System.ComponentModel.ISupportInitialize)(this.leftTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.rightTrackBar)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        private void LeftTrackBar_Changed(object o, EventArgs e)
        {
            if (leftTextBox.Text != leftTrackBar.Value.ToString())
            {
                leftTextBox.Text = leftTrackBar.Value.ToString();

                if (checkBox.Visible && checkBox.Checked)
                {
                    rightTextBox.Text = leftTextBox.Text;
                }

                if (OnChanged != null)
                {
                    OnChanged(null, null);
                }
            }
        }

        private void RightTrackBar_Changed(object o, EventArgs e)
        {
            if (rightTextBox.Text != rightTrackBar.Value.ToString())
            {
                rightTextBox.Text = rightTrackBar.Value.ToString();

                if (checkBox.Visible && checkBox.Checked)
                {
                    leftTextBox.Text = rightTextBox.Text;
                }

                if (OnChanged != null)
                {
                    OnChanged(null, null);
                }
            }
        }

        private void LeftTextBox_Changed(object o, EventArgs e)
        {
            try
            {
                if (leftTrackBar.Value != Convert.ToInt32(leftTextBox.Text))
                {
                    leftTrackBar.Value = Convert.ToInt32(leftTextBox.Text);

                    if (checkBox.Visible && checkBox.Checked)
                    {
                        rightTrackBar.Value = leftTrackBar.Value;
                    }

                    if (OnChanged != null)
                    {
                        OnChanged(null, null);
                    }
                }
            }
            catch (Exception) { }
        }

        private void RightTextBox_Changed(object o, EventArgs e)
        {
            try
            {
                if (rightTrackBar.Value != Convert.ToInt32(rightTextBox.Text))
                {
                    rightTrackBar.Value = Convert.ToInt32(rightTextBox.Text);

                    if (checkBox.Visible && checkBox.Checked)
                    {
                        leftTrackBar.Value = rightTrackBar.Value;
                    }

                    if (OnChanged != null)
                    {
                        OnChanged(null, null);
                    }
                }
            }
            catch (Exception) { }
        }
    }
}
