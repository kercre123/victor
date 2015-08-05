using System.Windows.Forms.DataVisualization.Charting;
using System;

namespace AnimationTool.Sequencer
{
    public class ChangeChartDuration : AnimationTool.ChangeChartDuration
    {
        protected MoveDataPoint moveDataPoint;

        public ChangeChartDuration(Chart chart, double xValueNew)
        {
            if (chart == null) return;

            this.chart = chart;
            points = chart.Series[0].Points;
            this.xValueNew = xValueNew;

            xValueOld = chart.ChartAreas[0].AxisY.Maximum;
        }

        public override bool Do()
        {
            if (chart == null || xValueOld == xValueNew || points == null || points.Count < 2) return false;

            chart.ChartAreas[0].AxisY.Maximum = xValueNew;
            chart.ChartAreas[0].AxisY.LabelStyle.Interval = Clamp(Math.Round(xValueNew * 0.1, 1), 0.1, 0.5);

            DataPoint last = points[points.Count - 1];

            moveDataPoint = new MoveDataPoint(last, xValueNew);
            moveDataPoint.Do();

            chart.Refresh();

            return true;
        }

        public override void Undo()
        {
            if (chart == null || moveDataPoint == null) return;

            chart.ChartAreas[0].AxisY.Maximum = Math.Round(xValueOld, 1);
            chart.ChartAreas[0].AxisY.LabelStyle.Interval = Clamp(Math.Round(xValueOld * 0.1, 1), 0.1, 0.5);
            moveDataPoint.Undo();

            chart.Refresh();
        }
    }
}
