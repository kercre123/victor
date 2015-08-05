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
            this.SuspendLayout();
            // 
            // textBox
            // 
            this.textBox.Location = new System.Drawing.Point(77, 12);
            this.textBox.Name = "textBox";
            this.textBox.Size = new System.Drawing.Size(96, 20);
            this.textBox.TabIndex = 0;
            this.textBox.Text = Properties.Settings.Default.maxTime.ToString();
            this.textBox.TextChanged += OnTextChange;
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
            this.scaleButton.Location = new System.Drawing.Point(12, 43);
            this.scaleButton.Name = "scaleButton";
            this.scaleButton.Size = new System.Drawing.Size(75, 23);
            this.scaleButton.TabIndex = 2;
            this.scaleButton.Text = "Animation";
            this.scaleButton.UseVisualStyleBackColor = true;
            this.scaleButton.Enabled = false;
            this.scaleButton.DialogResult = DialogResult.Yes;
            // 
            // truncateButton
            // 
            this.truncateButton.Location = new System.Drawing.Point(98, 43);
            this.truncateButton.Name = "truncateButton";
            this.truncateButton.Size = new System.Drawing.Size(75, 23);
            this.truncateButton.TabIndex = 3;
            this.truncateButton.Text = "Timeline";
            this.truncateButton.UseVisualStyleBackColor = true;
            this.truncateButton.Enabled = false;
            this.truncateButton.DialogResult = DialogResult.OK;
            // 
            // ChangeDurationForm
            // 
            this.ClientSize = new System.Drawing.Size(185, 78);
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
    }
}
