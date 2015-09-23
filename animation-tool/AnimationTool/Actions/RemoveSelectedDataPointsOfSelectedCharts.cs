using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class RemoveSelectedDataPointsOfSelectedCharts : Action
    {
        protected List<RemoveSelectedDataPoints> removeSelectedDataPoints;

        public RemoveSelectedDataPointsOfSelectedCharts(List<ChartForm> chartForms)
        {
            if (chartForms == null) return;

            removeSelectedDataPoints = new List<RemoveSelectedDataPoints>();

            foreach (ChartForm chartForm in chartForms)
            {
                if (chartForm.chart.BorderlineColor == SelectChart.borderlineColor)
                {
                    DataPoint dp = chartForm.chart.Series[0].Points[0]; // if chart has sequencer point
                    bool sequencer = dp.IsCustomPropertySet(Sequencer.ExtraData.Key) && Sequencer.ExtraData.Entries.ContainsKey(dp.GetCustomProperty(Sequencer.ExtraData.Key));

                    if (sequencer)
                    {
                        removeSelectedDataPoints.Add(new Sequencer.RemoveSelectedDataPoints(chartForm.chart));
                    }
                    else // else if XYchart
                    {
                        removeSelectedDataPoints.Add(new RemoveSelectedDataPoints(chartForm.chart));
                    }
                }
            }
        }

        public bool Do()
        {
            if (removeSelectedDataPoints == null) return false;

            for (int i = 0; i < removeSelectedDataPoints.Count; ++i)
            {
                if (!removeSelectedDataPoints[i].Do())
                {
                    removeSelectedDataPoints.RemoveAt(i--);
                }
            }

            return removeSelectedDataPoints.Count > 0;
        }

        public void Undo()
        {
            foreach (RemoveSelectedDataPoints action in removeSelectedDataPoints)
            {
                action.Undo();
            }
        }
    }
}
