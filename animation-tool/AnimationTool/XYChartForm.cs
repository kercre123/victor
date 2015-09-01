using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class XYChartForm : Form
    {
        public Chart chart;
        public Panel sidePanel;
        public CheckBox checkBox;

        public XYChartForm()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea1 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series1 = new System.Windows.Forms.DataVisualization.Charting.Series();
            this.chart = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.sidePanel = new System.Windows.Forms.Panel();
            this.checkBox = new System.Windows.Forms.CheckBox();
            ((System.ComponentModel.ISupportInitialize)(this.chart)).BeginInit();
            this.sidePanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // chart
            // 
            this.chart.BorderlineColor = System.Drawing.Color.Transparent;
            this.chart.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea1.AxisX.Interval = 0.1D;
            chartArea1.AxisX.IsLabelAutoFit = false;
            chartArea1.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea1.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea1.AxisX.LabelStyle.Interval = 0.5D;
            chartArea1.AxisX.LabelStyle.IntervalOffset = 0D;
            chartArea1.AxisX.MajorGrid.Interval = 1D;
            chartArea1.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea1.AxisX.MajorGrid.LineWidth = 2;
            chartArea1.AxisX.Maximum = 6D;
            chartArea1.AxisX.Minimum = 0D;
            chartArea1.AxisX.MinorGrid.Enabled = true;
            chartArea1.AxisX.MinorGrid.Interval = 0.1D;
            chartArea1.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea1.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea1.AxisX2.Maximum = 6D;
            chartArea1.AxisX2.Minimum = 0D;
            chartArea1.AxisY.Interval = 10D;
            chartArea1.AxisY.IntervalOffset = 5D;
            chartArea1.AxisY.IsLabelAutoFit = false;
            chartArea1.AxisY.LabelStyle.Interval = 10D;
            chartArea1.AxisY.LabelStyle.IntervalOffset = 5D;
            chartArea1.AxisY.MajorGrid.Interval = 5D;
            chartArea1.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea1.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea1.AxisY.MajorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea1.AxisY.MajorTickMark.Interval = 10D;
            chartArea1.AxisY.Maximum = 35D;
            chartArea1.AxisY.Minimum = -25D;
            chartArea1.AxisY.Title = "Head Angle\\n[deg]";
            chartArea1.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea1.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea1.AxisY2.Maximum = 35D;
            chartArea1.AxisY2.Minimum = -25D;
            chartArea1.Name = "ChartArea1";
            chartArea1.Position.Auto = false;
            chartArea1.Position.Height = 94F;
            chartArea1.Position.Width = 100F;
            chartArea1.Position.Y = 3F;
            this.chart.ChartAreas.Add(chartArea1);
            this.chart.Dock = System.Windows.Forms.DockStyle.Fill;
            this.chart.Location = new System.Drawing.Point(0, 0);
            this.chart.Margin = new System.Windows.Forms.Padding(2);
            this.chart.Name = "chart";
            series1.BorderDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            series1.BorderWidth = 3;
            series1.ChartArea = "ChartArea1";
            series1.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series1.Color = System.Drawing.Color.LightSkyBlue;
            series1.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series1.MarkerBorderWidth = 0;
            series1.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series1.MarkerSize = 8;
            series1.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Square;
            series1.Name = "SeriesKeyframes";
            this.chart.Series.Add(series1);
            this.chart.Size = new System.Drawing.Size(1154, 134);
            // 
            // sidePanel
            // 
            this.sidePanel.BackColor = System.Drawing.SystemColors.AppWorkspace;
            this.sidePanel.Controls.Add(this.checkBox);
            this.sidePanel.Dock = System.Windows.Forms.DockStyle.Right;
            this.sidePanel.Location = new System.Drawing.Point(1154, 0);
            this.sidePanel.Margin = new System.Windows.Forms.Padding(2);
            this.sidePanel.Name = "sidePanel";
            this.sidePanel.Size = new System.Drawing.Size(110, 134);
            // 
            // checkBox
            // 
            this.checkBox.AutoSize = true;
            this.checkBox.Location = new System.Drawing.Point(11, 11);
            this.checkBox.Margin = new System.Windows.Forms.Padding(2);
            this.checkBox.Name = "checkBox";
            this.checkBox.Size = new System.Drawing.Size(103, 17);
            this.checkBox.Text = "Disable Channel";
            this.checkBox.UseVisualStyleBackColor = true;
            // 
            // XYChartForm
            // 
            this.ClientSize = new System.Drawing.Size(1264, 134);
            this.ControlBox = false;
            this.Controls.Add(this.chart);
            this.Controls.Add(this.sidePanel);
            this.Name = "XYChartForm";
            this.TopLevel = false;
            this.ShowInTaskbar = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            ((System.ComponentModel.ISupportInitialize)(this.chart)).EndInit();
            this.sidePanel.ResumeLayout(false);
            this.sidePanel.PerformLayout();
            this.ResumeLayout(false);

        }
    }
}
