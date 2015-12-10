using System;
using System.Windows.Forms;
using System.Drawing;

namespace AnimationTool
{
    public class ChangeDurationForm : Form
    {
        private Label durationLabel;
        private Button scaleButton;
        private Button truncateButton;
        private TextBox textBox;

        public double Duration { get; private set; }

        private double minTime;
        private TextBox textBox1;
        private TextBox textBox2;
        private Button zoomButton;
        private const double MAX_DURATION = 9;

        public ChangeDurationForm()
        {
            FormBorderStyle = FormBorderStyle.FixedToolWindow;
            MaximizeBox = false;
            MinimizeBox = false;
            Duration = Properties.Settings.Default.maxTime;

            InitializeComponent();

            scaleButton.Enabled = false;
            truncateButton.Enabled = false;
        }

        public DialogResult Open(Point location, double maxTime, double minTime)
        {
            Duration = maxTime;
            textBox.Text = Duration.ToString();
            this.minTime = minTime;

            StartPosition = FormStartPosition.Manual;
            Location = location;

            return ShowDialog();
        }

        private void InitializeComponent()
        {
            this.textBox = new System.Windows.Forms.TextBox();
            this.durationLabel = new System.Windows.Forms.Label();
            this.scaleButton = new System.Windows.Forms.Button();
            this.truncateButton = new System.Windows.Forms.Button();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.textBox2 = new System.Windows.Forms.TextBox();
            this.zoomButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // textBox
            // 
            this.textBox.Location = new System.Drawing.Point(77, 12);
            this.textBox.Name = "textBox";
            this.textBox.Size = new System.Drawing.Size(96, 20);
            this.textBox.TabIndex = 0;
            this.textBox.Text = "6";
            this.textBox.TextChanged += new System.EventHandler(this.OnTextChange);
            // 
            // durationLabel
            // 
            this.durationLabel.AutoSize = true;
            this.durationLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationLabel.Location = new System.Drawing.Point(12, 15);
            this.durationLabel.Name = "durationLabel";
            this.durationLabel.Size = new System.Drawing.Size(55, 13);
            this.durationLabel.TabIndex = 1;
            this.durationLabel.Text = "Duration";
            // 
            // scaleButton
            // 
            this.scaleButton.DialogResult = System.Windows.Forms.DialogResult.Yes;
            this.scaleButton.Enabled = false;
            this.scaleButton.Location = new System.Drawing.Point(12, 43);
            this.scaleButton.Name = "scaleButton";
            this.scaleButton.Size = new System.Drawing.Size(75, 23);
            this.scaleButton.TabIndex = 2;
            this.scaleButton.Text = "Animation";
            this.scaleButton.UseVisualStyleBackColor = true;
            // 
            // truncateButton
            // 
            this.truncateButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.truncateButton.Enabled = false;
            this.truncateButton.Location = new System.Drawing.Point(98, 43);
            this.truncateButton.Name = "truncateButton";
            this.truncateButton.Size = new System.Drawing.Size(75, 23);
            this.truncateButton.TabIndex = 3;
            this.truncateButton.Text = "Timeline";
            this.truncateButton.UseVisualStyleBackColor = true;
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(44, 72);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(44, 20);
            this.textBox1.TabIndex = 4;
            // 
            // textBox2
            // 
            this.textBox2.Location = new System.Drawing.Point(98, 72);
            this.textBox2.Name = "textBox2";
            this.textBox2.Size = new System.Drawing.Size(47, 20);
            this.textBox2.TabIndex = 5;
            // 
            // zoomButton
            // 
            this.zoomButton.Location = new System.Drawing.Point(58, 98);
            this.zoomButton.Name = "zoomButton";
            this.zoomButton.Size = new System.Drawing.Size(75, 23);
            this.zoomButton.TabIndex = 6;
            this.zoomButton.Text = "ReZoom";
            this.zoomButton.UseVisualStyleBackColor = true;
            // 
            // ChangeDurationForm
            // 
            this.ClientSize = new System.Drawing.Size(187, 135);
            this.Controls.Add(this.zoomButton);
            this.Controls.Add(this.textBox2);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.truncateButton);
            this.Controls.Add(this.scaleButton);
            this.Controls.Add(this.durationLabel);
            this.Controls.Add(this.textBox);
            this.Name = "ChangeDurationForm";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void OnTextChange(object o, EventArgs e)
        {
            truncateButton.Enabled = false;
            scaleButton.Enabled = false;

            try
            {
                double newDuration = Math.Round(Convert.ToDouble(textBox.Text), 1);

                if(newDuration >= minTime && newDuration <= MAX_DURATION && Properties.Settings.Default.maxTime != newDuration)
                {
                    Duration = newDuration;
                    truncateButton.Enabled = true;
                    scaleButton.Enabled = true;
                }
            }
            catch (Exception) { }
        }

        private void OnZoom(object o, EventArgs e)
        {
            double timeStart = double.Parse(textBox1.Text);
            double timeEnd = double.Parse(textBox2.Text);
        }
    }
}
