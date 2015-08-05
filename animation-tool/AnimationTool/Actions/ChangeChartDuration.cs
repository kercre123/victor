using System.Windows.Forms.DataVisualization.Charting;
using System;

namespace AnimationTool
{
    public class ChangeChartDuration : Action
    {
        protected double xValueOld;
        protected double xValueNew;

        protected Chart chart;
        protected DataPointCollection points;

        protected RemoveDataPoint removeDataPoint;

        protected ChangeChartDuration() { }

        public ChangeChartDuration(Chart chart, double xValueNew)
        {
            this.chart = chart;
            points = chart.Series[0].Points;
            this.xValueNew = xValueNew;

            xValueOld = chart.ChartAreas[0].AxisX.Maximum;
        }

        public virtual bool Do()
        {
            if (chart == null || xValueOld == xValueNew || points == null || points.Count < 2) return false;

            chart.ChartAreas[0].AxisX.Maximum = xValueNew;
            chart.ChartAreas[0].AxisX.LabelStyle.Interval = Clamp(Math.Round(xValueNew * 0.1, 1), 0.1, 0.5);

            DataPoint last = points[points.Count - 1];

            removeDataPoint = new RemoveDataPoint(last, chart);
            removeDataPoint.Do();

            chart.Refresh();

            return true;
        }

        public virtual void Undo()
        {
            if (chart == null || removeDataPoint == null) return;

            chart.ChartAreas[0].AxisX.Maximum = xValueOld;
            chart.ChartAreas[0].AxisX.LabelStyle.Interval = Clamp(Math.Round(xValueOld * 0.1, 1), 0.1, 0.5);
            removeDataPoint.Undo();

            chart.Refresh();
        }

        public static double Clamp(double value, double min, double max)
        {
            return value < min ? min : value > max ? max : value;
        }
    }
}
