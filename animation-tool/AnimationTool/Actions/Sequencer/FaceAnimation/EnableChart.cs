using System.Collections.Generic;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;

namespace AnimationTool.FaceAnimation
{
    public class EnableChart : Sequencer.EnableChart
    {
        public EnableChart(ChartForm form)
        {
            chart = form.chart;
            checkBox = form.checkBox;
            removeDataPoints = new List<Sequencer.RemoveDataPoint>();
            unselectAllDataPoints = new UnselectAllDataPoints(chart);
        }

        public override bool Do()
        {
            if (!base.Do())
            {
                return false;
            }

            bool addPreview = true;

            foreach(DataPoint dp in chart.Series[0].Points)
            {
                if(dp.Color == Color.Red)
                {
                    addPreview = false;
                    break;
                }
            }

            if (addPreview)
            {
                Sequencer.ExtraData previewBar = new Sequencer.ExtraData();
                previewBar.Length = 0.04D;
                AddPreviewBar addPreviewBar = new AddPreviewBar(chart, previewBar);
                addPreviewBar.Do();
            }

            chart.Refresh();

            return true;
        }
    }
}
