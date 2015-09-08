using System;
using System.Windows.Forms;

namespace AnimationTool
{
    public class StraightForm : Form
    {
        private TextBox timeTextBox;
        private TextBox speedTextBox;
        private Label timeLabel;
        private Label speedLabel;
        private Button button;

        private double maxTime;

        private double previousTime;
        public double Time { get; private set; }

        private double previousSpeed;
        public int Speed { get; private set; }

        public StraightForm()
        {
            FormBorderStyle = FormBorderStyle.FixedToolWindow;
            MaximizeBox = false;
            MinimizeBox = false;

            InitializeComponent();

            Time = MoveSelectedDataPoints.DELTA_X;
            Speed = 10;
            button.Enabled = false;
        }

        private void InitializeComponent()
        {
            this.timeTextBox = new System.Windows.Forms.TextBox();
            this.speedTextBox = new System.Windows.Forms.TextBox();
            this.timeLabel = new System.Windows.Forms.Label();
            this.speedLabel = new System.Windows.Forms.Label();
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
            // speedTextBox
            // 
            this.speedTextBox.Location = new System.Drawing.Point(91, 38);
            this.speedTextBox.Name = "speedTextBox";
            this.speedTextBox.Size = new System.Drawing.Size(100, 20);
            this.speedTextBox.TabIndex = 1;
            this.speedTextBox.Text = "10";
            this.speedTextBox.TextChanged += OnSpeedTextChange;
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
            // speedLabel
            // 
            this.speedLabel.AutoSize = true;
            this.speedLabel.Location = new System.Drawing.Point(12, 38);
            this.speedLabel.Name = "speedLabel";
            this.speedLabel.Size = new System.Drawing.Size(73, 13);
            this.speedLabel.TabIndex = 3;
            this.speedLabel.Text = "Speed (mm/s)";
            this.speedLabel.TextAlign = System.Drawing.ContentAlignment.BottomLeft;
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
            // StraightForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(206, 94);
            this.Controls.Add(this.button);
            this.Controls.Add(this.speedLabel);
            this.Controls.Add(this.timeLabel);
            this.Controls.Add(this.speedTextBox);
            this.Controls.Add(this.timeTextBox);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "StraightForm";
            this.ShowIcon = false;
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        public DialogResult Open(double maxTime, int speed, double time, IWin32Window parent)
        {
            this.maxTime = maxTime;
            speedTextBox.Text = speed.ToString();
            Speed = speed;
            timeTextBox.Text = time.ToString();
            Time = time;
            previousSpeed = speed;
            previousTime = time;

            StartPosition = FormStartPosition.CenterParent;

            return ShowDialog(parent);
        }

        private void OnTimeTextChange(object o, EventArgs e)
        {
            button.Enabled = false;

            try
            {
                double newTime = Math.Round(Convert.ToDouble(timeTextBox.Text), 1);

                if (newTime >= MoveSelectedDataPoints.DELTA_X && newTime <= maxTime && newTime != previousTime)
                {
                    Time = newTime;
                    button.Enabled = true;
                }
            }
            catch (Exception) { }
        }

        private void OnSpeedTextChange(object o, EventArgs e)
        {
            button.Enabled = false;

            if (speedTextBox.Text == null || speedTextBox.Text == string.Empty) return;

            try
            {
                int newSpeed = Convert.ToInt32(speedTextBox.Text);

                if (Math.Abs(newSpeed) >= 10 && Math.Abs(newSpeed) <= 160 && newSpeed != previousSpeed)
                {
                    Speed = newSpeed;
                    button.Enabled = true;
                }
            }
            catch (Exception) { }
        }
    }
}
