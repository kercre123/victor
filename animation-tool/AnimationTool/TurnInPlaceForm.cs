using System;
using System.Windows.Forms;

namespace AnimationTool
{
    public class TurnInPlaceForm : Form
    {
        private TextBox timeTextBox;
        private TextBox angleTextBox;
        private Label timeLabel;
        private Label angleLabel;
        private Button button;

        private double maxTime;

        private double previousTime;
        public double Time { get; private set; }

        private int previousAngle;
        public int Angle { get; private set; }

        public TurnInPlaceForm()
        {
            FormBorderStyle = FormBorderStyle.FixedToolWindow;
            MaximizeBox = false;
            MinimizeBox = false;

            InitializeComponent();

            Time = MoveSelectedDataPoints.DELTA_X;
            Angle = 1;
            button.Enabled = false;
        }

        private void InitializeComponent()
        {
            this.timeTextBox = new System.Windows.Forms.TextBox();
            this.angleTextBox = new System.Windows.Forms.TextBox();
            this.timeLabel = new System.Windows.Forms.Label();
            this.angleLabel = new System.Windows.Forms.Label();
            this.button = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // timeTextBox
            // 
            this.timeTextBox.Location = new System.Drawing.Point(91, 12);
            this.timeTextBox.Name = "timeTextBox";
            this.timeTextBox.Size = new System.Drawing.Size(100, 20);
            this.timeTextBox.TabIndex = 0;
            this.timeTextBox.Text = "0.1";
            this.timeTextBox.TextChanged += OnTimeTextChange;
            // 
            // angleTextBox
            // 
            this.angleTextBox.Location = new System.Drawing.Point(91, 38);
            this.angleTextBox.Name = "angleTextBox";
            this.angleTextBox.Size = new System.Drawing.Size(100, 20);
            this.angleTextBox.TabIndex = 1;
            this.angleTextBox.Text = "1";
            this.angleTextBox.TextChanged += OnAngleTextChange;
            // 
            // timeLabel
            // 
            this.timeLabel.AutoSize = true;
            this.timeLabel.Location = new System.Drawing.Point(12, 12);
            this.timeLabel.Name = "timeLabel";
            this.timeLabel.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.timeLabel.Size = new System.Drawing.Size(30, 13);
            this.timeLabel.TabIndex = 2;
            this.timeLabel.Text = "Time (s)";
            this.timeLabel.TextAlign = System.Drawing.ContentAlignment.BottomLeft;
            // 
            // angleLabel
            // 
            this.angleLabel.AutoSize = true;
            this.angleLabel.Location = new System.Drawing.Point(12, 38);
            this.angleLabel.Name = "angleLabel";
            this.angleLabel.Size = new System.Drawing.Size(73, 13);
            this.angleLabel.TabIndex = 3;
            this.angleLabel.Text = "Angle (deg)";
            this.angleLabel.TextAlign = System.Drawing.ContentAlignment.BottomLeft;
            // 
            // button
            // 
            this.button.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.button.Enabled = false;
            this.button.Location = new System.Drawing.Point(91, 64);
            this.button.Name = "button";
            this.button.Size = new System.Drawing.Size(100, 23);
            this.button.TabIndex = 4;
            this.button.Text = "OK";
            this.button.UseVisualStyleBackColor = true;
            // 
            // TurnInPlaceForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(206, 94);
            this.Controls.Add(this.button);
            this.Controls.Add(this.angleLabel);
            this.Controls.Add(this.timeLabel);
            this.Controls.Add(this.angleTextBox);
            this.Controls.Add(this.timeTextBox);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "TurnInPlaceForm";
            this.ShowIcon = false;
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        public DialogResult Open(double maxTime, int angle, double time, IWin32Window parent)
        {
            this.maxTime = maxTime;
            angleTextBox.Text = angle.ToString();
            Angle = angle;
            timeTextBox.Text = time.ToString();
            Time = time;
            previousTime = time;
            previousAngle = angle;

            StartPosition = FormStartPosition.CenterParent;

            return ShowDialog(parent);
        }

        private void OnTimeTextChange(object o, EventArgs e)
        {
            button.Enabled = false;

            try
            {
                double newTime = Math.Round(Convert.ToDouble(timeTextBox.Text), 1);

                if (newTime >= MoveSelectedDataPoints.DELTA_X && newTime <= maxTime && previousTime != newTime)
                {
                    Time = newTime;
                    button.Enabled = true;
                }
            }
            catch (Exception) { }
        }

        private void OnAngleTextChange(object o, EventArgs e)
        {
            button.Enabled = false;

            if (angleTextBox.Text == null || angleTextBox.Text == string.Empty) return;

            try
            {
                int newAngle = Convert.ToInt32(angleTextBox.Text);

                if (newAngle != 0 && previousAngle != newAngle)
                {
                    Angle = newAngle;
                    button.Enabled = true;
                }
            }
            catch (Exception) { }
        }
    }
}
