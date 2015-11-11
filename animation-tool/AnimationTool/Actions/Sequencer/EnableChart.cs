using System.Collections.Generic;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool.Sequencer
{
    public class EnableChart : AnimationTool.EnableChart
    {
        protected new List<RemoveDataPoint> removeDataPoints;

        protected EnableChart() { }

        public EnableChart(ChartForm form)
        {
            chart = form.chart;
            checkBox = form.CheckBox;
            removeDataPoints = new List<RemoveDataPoint>();
            unselectAllDataPoints = new UnselectAllDataPoints(chart);
        }

        public override bool Do()
        {
            if (chart == null || chart.Enabled || checkBox == null) return false;

            chart.Enabled = true;

            if (checkBox.Checked)
            {
                checkBox.Checked = false;
            }

            if (chart.Series[0].Points.Count == 0)
            {
                ExtraData firstData = new ExtraData();
                AddDataPoint addFirst = new AddDataPoint(chart, firstData, chart.ChartAreas[0].AxisY.Minimum - MoveSelectedDataPoints.DELTA_X, false);

                if (addFirst.Do())
                {
                    addFirst.dataPoint.IsEmpty = true;
                }
            }

            if (chart.Series[0].Points.Count == 1)
            {
                ExtraData lastData = new ExtraData();
                AddDataPoint addLast = new AddDataPoint(chart, lastData, chart.ChartAreas[0].AxisY.Maximum, false);

                if (addLast.Do())
                {
                    addLast.dataPoint.IsEmpty = true;
                }
            }

            if (!unselectAllDataPoints.Do())
            {
                unselectAllDataPoints = null;
            }

            for (int i = 1; i < chart.Series[0].Points.Count-2; ++i) // don't change first, last, or preview bar
            {
                chart.Series[0].Points[i].IsEmpty = false;
            }

            chart.Refresh();

            return true;
        }

        public override void Undo()
        {
            for (int i = 0; i < chart.Series[0].Points.Count; ++i)
            {
                chart.Series[0].Points[i].IsEmpty = true;
            }

            if (unselectAllDataPoints != null)
            {
                unselectAllDataPoints.Undo();
            }

            chart.Enabled = false;

            if (checkBox != null && !checkBox.Checked)
            {
                checkBox.Checked = true;
            }

            chart.Refresh();
        }
    }
}
